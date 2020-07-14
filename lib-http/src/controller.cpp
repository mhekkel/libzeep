// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <cassert>

#include <zeep/http/controller.hpp>

namespace zeep::http
{

thread_local request* controller::s_request = nullptr;

controller::controller(const std::string& prefix_path)
	: m_prefix_path(prefix_path)
{
	// strip leading slashes
	while (m_prefix_path[0] == '/')
		m_prefix_path.erase(m_prefix_path.begin());
}

controller::~controller()
{
}

bool controller::dispatch_request(request& req, reply& rep)
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

bool controller::path_matches_prefix(const std::string& path) const
{
	bool result = m_prefix_path.empty();
	
	if (not result)
	{
		int offset = 0;
		while (path[offset] == '/')
			++offset;
		result = path.compare(offset, m_prefix_path.length(), m_prefix_path) == 0;
	}

	return result;
}

std::string controller::get_prefix_less_path(request& req) const
{
	auto p = req.get_path();

	if (not m_prefix_path.empty())
	{
		if (p.compare(0, m_prefix_path.length(), m_prefix_path) != 0)
		{
			assert(false);
			throw std::logic_error("Controller does not have the prefix path for this request");
		}
		
		p.erase(0, m_prefix_path.length());		
	}

	while (p.front() == '/')
		p.erase(0, 1);
	
	return p;
}

json::element controller::get_credentials() const
{
	json::element credentials;
	if (m_server != nullptr and s_request != nullptr)
		credentials = m_server->get_credentials(*s_request);
	return credentials;
}

bool controller::has_role(const std::string& role) const
{
	auto credentials = get_credentials();
	return credentials.is_object() and credentials["role"].is_array() and credentials["role"].contains(role);
}

}