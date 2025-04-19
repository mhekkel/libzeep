// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <cassert>

#include <zeep/http/controller.hpp>
#include <zeep/http/uri.hpp>

namespace zeep::http
{

thread_local request *controller::s_request = nullptr;

controller::controller(const std::string &prefix_path)
	: m_prefix_path(prefix_path)
{
}

controller::~controller()
{
	for (auto mp : m_mountpoints)
		delete mp;
}

bool controller::dispatch_request(asio_ns::ip::tcp::socket & /*socket*/, request &req, reply &rep)
{
	bool result = false;

	try
	{
		s_request = &req;
		result = handle_request(req, rep);
		s_request = nullptr;
	}
	catch (...)
	{
		s_request = nullptr;
		throw;
	}

	return result;
}

bool controller::path_matches_prefix(const uri &path) const
{
	bool result = m_prefix_path.empty();

	if (not result)
	{
		auto ab = m_prefix_path.get_segments().begin(), ae = m_prefix_path.get_segments().end();
		auto bb = path.get_segments().begin(), be = path.get_segments().end();

		do
		{
			if (ab->empty() and ab + 1 == ae)
			{
				result = true;
				break;
			}

			result = ab != ae and bb != be and *ab == *bb;
			++ab;
			++bb;
		} while (result and ab != ae);
	}

	return result;
}

uri controller::get_prefixless_path(const request &req) const
{
	auto path = req.get_uri().get_path();

	auto ab = m_prefix_path.get_segments().begin(), ae = m_prefix_path.get_segments().end();
	auto bb = path.get_segments().begin(), be = path.get_segments().end();

	while (ab != ae and bb != be)
	{
		if (ab->empty() and ab + 1 == ae)
			break;

		if (*ab != *bb)
			throw zeep::exception("Controller does not have the same prefix as the request");

		++ab;
		++bb;
	}

	return { bb, be };
}

el::object controller::get_credentials() const
{
	el::object credentials;
	if (s_request != nullptr)
		credentials = s_request->get_credentials();
	return credentials;
}

std::string controller::get_remote_address() const
{
	std::string result;
	if (s_request != nullptr)
		result = s_request->get_remote_address();
	return result;
}

bool controller::has_role(const std::string &role) const
{
	auto credentials = get_credentials();
	return credentials.is_object() and credentials["role"].is_array() and credentials["role"].contains(role);
}

std::string controller::get_header(const char *name) const
{
	return s_request ? s_request->get_header(name) : "";
}

void controller::get_options(const request &req, reply &rep)
{
	if (m_server)
		m_server->get_options_for_request(req, rep);
}

// --------------------------------------------------------------------

void controller::init_scope(request &req, scope &)
{

}

bool controller::handle_request(http::request &req, http::reply &rep)
{
	auto p = get_prefixless_path(req).string();

	bool result = false;
	for (auto &mp : m_mountpoints)
	{
		if (req.get_method() != mp->m_method)
			continue;

		parameter_pack params(*get_server(), req);
		init_scope(req, params.get_scope());

		if (mp->m_path_params.empty())
		{
			if (mp->m_path != p)
				continue;
		}
		else
		{
			std::smatch m;
			if (not std::regex_match(p, m, mp->m_rx))
				continue;

			for (size_t i = 0; i < mp->m_path_params.size(); ++i)
			{
				std::string v = m[i + 1].str();
				v = decode_url(v);
				params.m_path_parameters.push_back({ mp->m_path_params[i], v });
			}
		}

		try
		{
			if (req.get_method() == "OPTIONS")
				get_options(req, rep);
			else
				call_mount_point(mp, params, rep);
		}
		catch (status_type s)
		{
			rep = http::reply::stock_reply(s);

			object error({ { "error", get_status_description(s) } });
			rep.set_content(error);
			rep.set_status(s);
		}
		catch (const std::exception &e)
		{
			rep = http::reply::stock_reply(http::internal_server_error);

			object error({ { "error", e.what() } });
			rep.set_content(error);
			rep.set_status(http::internal_server_error);
		}

		result = true;
		break;
	}

	return result;
}

void controller::call_mount_point(mount_point_base *mp, const parameter_pack &params, reply &rep)
{
	try
	{
		object message("ok");
		rep.set_content(message);
		rep.set_status(ok);

		mp->call(params, rep);
	}
	catch (const std::exception &e)
	{
		object message;
		message["error"] = e.what();

		rep.set_content(message);
		rep.set_status(internal_server_error);
	}
}

} // namespace zeep::http
