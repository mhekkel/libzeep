// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2025
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/config.hpp"

#include "glob.hpp"

#include <cassert>

#include "zeep/http/controller.hpp"
#include "zeep/http/uri.hpp"

namespace zeep::http
{

controller::controller(std::string prefix_path)
	: m_prefix_path(std::move(prefix_path))
{
}

controller::~controller()
{
	for (auto mp : m_mountpoints)
		delete mp;
}

bool controller::dispatch_request(asio_ns::ip::tcp::socket & /*socket*/, request &req, reply &rep)
{
	return handle_request(req, rep);
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

void controller::get_options(const request &req, reply &rep)
{
	if (m_server)
		m_server->get_options_for_request(req, rep);
}

// --------------------------------------------------------------------

void controller::init_scope(scope &)
{
}

bool controller::handle_request(http::request &req, http::reply &rep)
{
	auto uri = get_prefixless_path(req);
	auto path = get_prefixless_path(req).string();

	bool result = false;
	for (auto &mp : m_mountpoints)
	{
		if (req.get_method() != mp->m_method)
			continue;

		scope scope(get_server(), req);

		if (mp->m_path_params.empty())
		{
			if (not glob_match(std::filesystem::path(path), mp->m_path))
				continue;
		}
		else
		{
			std::smatch m;
			if (not std::regex_match(path, m, mp->m_rx))
				continue;

			for (size_t i = 0; i < mp->m_path_params.size(); ++i)
				scope.add_path_param(mp->m_path_params[i], decode_url(m[i + 1].str()));
		}

		scope.put("baseuri", path);
		init_scope(scope);

		if (req.get_method() == "OPTIONS")
			get_options(req, rep);
		else
			rep = call_mount_point(mp, scope);

		result = true;
		break;
	}

	return result;
}

reply controller::call_mount_point(mount_point_base *mp, const scope &scope)
{
	return mp->call(scope);
}

} // namespace zeep::http
