//          Copyright Maarten L. Hekkelman 2026
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "zeep/exception.hpp"

#include <cassert>
#include <string>
#include <system_error>

namespace zeep::http
{

/// Various predefined HTTP status codes

enum class status_type
{
	cont = 100,
	ok = 200,
	created = 201,
	accepted = 202,
	no_content = 204,
	multiple_choices = 300,
	moved_permanently = 301,
	moved_temporarily = 302,
	see_other = 303,
	not_modified = 304,
	bad_request = 400,
	unauthorized = 401,
	forbidden = 403,
	not_found = 404,
	method_not_allowed = 405,
	unprocessable_entity = 422,
	proxy_authentication_required = 407,
	internal_server_error = 500,
	not_implemented = 501,
	bad_gateway = 502,
	service_unavailable = 503
};

/**
 * @brief The implementation for @ref config_category error messages
 *
 */
class status_type_impl : public std::error_category
{
  public:
	/**
	 * @brief User friendly name
	 *
	 * @return const char*
	 */

	[[nodiscard]] const char *name() const noexcept override
	{
		return "http status";
	}

	/**
	 * @brief Provide the error message as a string for the error code @a ev
	 *
	 * @param ev The error code
	 * @return std::string
	 */

	[[nodiscard]] std::string message(int ev) const override
	{
		switch (static_cast<status_type>(ev))
		{
			case status_type::cont: return "Continue";
			case status_type::ok: return "OK";
			case status_type::created: return "Created";
			case status_type::accepted: return "Accepted";
			case status_type::no_content: return "No Content";
			case status_type::multiple_choices: return "Multiple Choices";
			case status_type::moved_permanently: return "Moved Permanently";
			case status_type::moved_temporarily: return "Found";
			case status_type::see_other: return "See Other";
			case status_type::not_modified: return "Not Modified";
			case status_type::bad_request: return "Bad Request";
			case status_type::unauthorized: return "Unauthorized";
			case status_type::proxy_authentication_required: return "Proxy Authentication Required";
			case status_type::forbidden: return "Forbidden";
			case status_type::not_found: return "Not Found";
			case status_type::method_not_allowed: return "Method not allowed";
			case status_type::unprocessable_entity: return "Unprocessable Entity";
			case status_type::internal_server_error: return "Internal Server Error";
			case status_type::not_implemented: return "Not Implemented";
			case status_type::bad_gateway: return "Bad Gateway";
			case status_type::service_unavailable: return "Service Unavailable";
		}
	}

	/**
	 * @brief Return whether two error codes are equivalent, always false in this case
	 *
	 */

	[[nodiscard]] bool equivalent(const std::error_code & /*code*/, int /*condition*/) const noexcept override
	{
		return false;
	}
};

/**
 * @brief Return the implementation for the config_category
 *
 * @return std::error_category&
 */
inline std::error_category &status_type_category()
{
	static status_type_impl instance;
	return instance;
}

inline std::error_code make_error_code(status_type e)
{
	return { static_cast<int>(e), status_type_category() };
}

inline std::error_condition make_error_condition(status_type e)
{
	return { static_cast<int>(e), status_type_category() };
}

/// Return the string describing the status_type in more detail
std::string get_status_description(status_type status);

// http exception

class http_status_exception : public exception
{
  public:
	http_status_exception(std::error_code ec) noexcept
		: exception(ec.message())
		, m_code(ec)
	{
	}

	http_status_exception(status_type status) noexcept
		: zeep::http::http_status_exception(make_error_code(status))
	{
	}

	[[nodiscard]] const std::error_code &code() const noexcept { return m_code; }
	[[nodiscard]] status_type status() const noexcept { return static_cast<status_type>(m_code.value()); }

  private:
	std::error_code m_code;
};

} // namespace zeep::http