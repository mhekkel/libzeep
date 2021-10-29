// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <cassert>

#include <zeep/http/controller.hpp>
#include <zeep/http/uri.hpp>

namespace zeep::http
{

thread_local request* controller::s_request = nullptr;

controller::controller(const std::string& prefix_path)
	: m_prefix_path(prefix_path)
{
	while (not m_prefix_path.empty())
	{
		// strip leading slashes
		if (m_prefix_path.front() == '/')
		{
			m_prefix_path.erase(m_prefix_path.begin());
			continue;
		}

		// and trailing slashes
		if (m_prefix_path.back() == '/')
		{
			m_prefix_path.pop_back();
			continue;
		}

		break;
	}
}

controller::~controller()
{
}

bool controller::dispatch_request(boost::asio::ip::tcp::socket& socket, request& req, reply& rep)
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
		std::string::size_type offset = 0;
		while (offset < path.length() and path[offset] == '/')
			++offset;

		result = path.compare(offset, m_prefix_path.length(), m_prefix_path) == 0;

		if (result)
			result = path.length() == m_prefix_path.length() + offset or path[offset + m_prefix_path.length()] == '/';
	}

	return result;
}

std::string controller::get_prefixless_path(const request& req) const
{
	uri uri(req.get_uri());

	auto result = uri.is_absolute()
		? uri.get_path().lexically_relative("/")
		: uri.get_path();

	if (not m_prefix_path.empty())
	{
		result = result.lexically_relative(m_prefix_path);

		if (not result.empty() and result.begin()->string() == "..")
		{
			assert(false);
			throw std::logic_error("Controller does not have the prefix path for this request");
		}
	}

	return result == "." ? "" : result.string();
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

bool controller::has_role(const std::string& role) const
{
	auto credentials = get_credentials();
	return credentials.is_object() and credentials["role"].is_array() and credentials["role"].contains(role);
}

std::string controller::get_header(const char *name) const
{
	return s_request ? s_request->get_header(name) : "";
}

}
