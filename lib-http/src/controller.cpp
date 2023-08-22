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
		}
		while (result and ab != ae);
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

json::element controller::get_credentials() const
{
	json::element credentials;
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

} // namespace zeep::http
