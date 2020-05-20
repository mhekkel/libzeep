// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/security.hpp>
#include <zeep/http/error-handler.hpp>

namespace zeep::http
{

error_handler::error_handler()
{
}

error_handler::~error_handler()
{
}

bool error_handler::create_error_reply(const request& req, std::exception_ptr eptr, reply& reply)
{
	bool result = false;

	try
	{
		if (eptr)
			std::rethrow_exception(eptr);
	}
	catch (const http::status_type& s)
	{
		result = create_error_reply(req, s, get_status_description(s), reply);
	}
	catch (const unauthorized_exception& ex)
	{
		result = create_unauth_reply(req, ex.m_realm, reply);
	}
	catch (const std::exception& ex)
	{
		result = create_error_reply(req, http::internal_server_error, ex.what(), reply);
	}
	catch (...)
	{
		result = create_error_reply(req, http::internal_server_error, "unhandled exception", reply);
	}

	return result;
}

bool error_handler::create_unauth_reply(const request& req, const std::string& realm, reply& rep)
{
	return create_error_reply(req, http::unauthorized, "You don't have access to " + realm, rep);
}

bool error_handler::create_error_reply(const request& req, status_type status, reply& rep)
{
	return create_error_reply(req, status, get_status_description(status), rep);
}

bool error_handler::create_error_reply(const request& req, status_type status, const std::string& message, reply& rep)
{
	rep = reply::stock_reply(status, message);
	return true;
}

}