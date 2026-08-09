// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the HTTP status codes and related helper types

#include "zeep/exception.hpp"

#include <cassert>
#include <string>
#include <system_error>

namespace zeep::http
{

/// \brief Enumeration of standard HTTP status codes, covering all codes from
/// 1xx (Informational), 2xx (Successful), 3xx (Redirection), 4xx (Client Error),
/// to 5xx (Server Error) as defined in RFC 7231 and related RFCs.

enum class status_type
{
	cont = 100,
	switching_protocols = 101,
	processing = 102,
	early_hints = 103,
	upload_resumption_supported = 104,
	ok = 200,
	created = 201,
	accepted = 202,
	non_authoritative_information = 203,
	no_content = 204,
	reset_content = 205,
	partial_content = 206,
	multi_status = 207,
	already_reported = 208,
	im_used = 226,
	multiple_choices = 300,
	moved_permanently = 301,
	found = 302,
	moved_temporarily = found,
	see_other = 303,
	not_modified = 304,
	use_proxy = 305,
	temporary_redirect = 307,
	permanent_redirect = 308,
	bad_request = 400,
	unauthorized = 401,
	payment_required = 402,
	forbidden = 403,
	not_found = 404,
	method_not_allowed = 405,
	not_acceptable = 406,
	proxy_authentication_required = 407,
	request_timeout = 408,
	conflict = 409,
	gone = 410,
	length_required = 411,
	precondition_failed = 412,
	content_too_large = 413,
	uri_too_long = 414,
	unsupported_media_type = 415,
	range_not_satisfiable = 416,
	expectation_failed = 417,
	misdirected_request = 421,
	unprocessable_content = 422,
	unprocessable_entity = unprocessable_content,
	locked = 423,
	failed_dependency = 424,
	too_early = 425,
	upgrade_required = 426,
	precondition_required = 428,
	too_many_requests = 429,
	request_header_fields_too_large = 431,
	unavailable_for_legal_reasons = 451,
	internal_server_error = 500,
	not_implemented = 501,
	bad_gateway = 502,
	service_unavailable = 503,
	gateway_timeout = 504,
	http_version_not_supported = 505,
	variant_also_negotiates = 506,
	insufficient_storage = 507,
	loop_detected = 508,
	not_extended_obsoleted = 510,
	network_authentication_required = 511
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
			using enum status_type;

			case cont: return "Continue";
			case switching_protocols: return "Switching Protocols";
			case processing: return "Processing";
			case early_hints: return "Early Hints";
			case upload_resumption_supported: return "Upload Resumption Supported";
			case ok: return "OK";
			case created: return "Created";
			case accepted: return "Accepted";
			case non_authoritative_information: return "Non-Authoritative Information";
			case no_content: return "No Content";
			case reset_content: return "Reset Content";
			case partial_content: return "Partial Content";
			case multi_status: return "Multi-Status";
			case already_reported: return "Already Reported";
			case im_used: return "IM Used";
			case multiple_choices: return "Multiple Choices";
			case moved_permanently: return "Moved Permanently";
			case found: return "Found";
			case see_other: return "See Other";
			case not_modified: return "Not Modified";
			case use_proxy: return "Use Proxy";
			case temporary_redirect: return "Temporary Redirect";
			case permanent_redirect: return "Permanent Redirect";
			case bad_request: return "Bad Request";
			case unauthorized: return "Unauthorized";
			case payment_required: return "Payment Required";
			case forbidden: return "Forbidden";
			case not_found: return "Not Found";
			case method_not_allowed: return "Method Not Allowed";
			case not_acceptable: return "Not Acceptable";
			case proxy_authentication_required: return "Proxy Authentication Required";
			case request_timeout: return "Request Timeout";
			case conflict: return "Conflict";
			case gone: return "Gone";
			case length_required: return "Length Required";
			case precondition_failed: return "Precondition Failed";
			case content_too_large: return "Content Too Large";
			case uri_too_long: return "URI Too Long";
			case unsupported_media_type: return "Unsupported Media Type";
			case range_not_satisfiable: return "Range Not Satisfiable";
			case expectation_failed: return "Expectation Failed";
			case misdirected_request: return "Misdirected Request";
			case unprocessable_content: return "Unprocessable Content";
			case locked: return "Locked";
			case failed_dependency: return "Failed Dependency";
			case too_early: return "Too Early";
			case upgrade_required: return "Upgrade Required";
			case precondition_required: return "Precondition Required";
			case too_many_requests: return "Too Many Requests";
			case request_header_fields_too_large: return "Request Header Fields Too Large";
			case unavailable_for_legal_reasons: return "Unavailable For Legal Reasons";
			case internal_server_error: return "Internal Server Error";
			case not_implemented: return "Not Implemented";
			case bad_gateway: return "Bad Gateway";
			case service_unavailable: return "Service Unavailable";
			case gateway_timeout: return "Gateway Timeout";
			case http_version_not_supported: return "HTTP Version Not Supported";
			case variant_also_negotiates: return "Variant Also Negotiates";
			case insufficient_storage: return "Insufficient Storage";
			case loop_detected: return "Loop Detected";
			case not_extended_obsoleted: return "obsoleted Not Extended";
			case network_authentication_required: return "Network Authentication Required";
			default: return "unknown status code";
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

/// \brief Create an std::error_code from a status_type
/// \param e  The HTTP status code
/// \return   An error_code representing the given status
inline std::error_code make_error_code(status_type e)
{
	return { static_cast<int>(e), status_type_category() };
}

/// \brief Create an std::error_condition from a status_type
/// \param e  The HTTP status code
/// \return   An error_condition representing the given status
inline std::error_condition make_error_condition(status_type e)
{
	return { static_cast<int>(e), status_type_category() };
}

/// \brief Return a human-readable description string for a given HTTP status code
/// \param status  The HTTP status code
/// \return        The descriptive string (e.g., "Not Found" for 404)
std::string get_status_description(status_type status);

/// \brief Exception class that carries an HTTP status code
///
/// This exception can be thrown by handlers to signal a specific HTTP error
/// response. The status code is available via \a status() and the human-readable
/// message via \a what().
class http_status_exception : public exception
{
  public:
	/// \brief Construct from an error_code
	/// \param ec  The error code representing the HTTP status
	http_status_exception(std::error_code ec) noexcept
		: exception(ec.message())
		, m_code(ec)
	{
	}

	/// \brief Construct directly from a status_type enumerator
	/// \param status  The HTTP status code
	http_status_exception(status_type status) noexcept
		: zeep::http::http_status_exception(make_error_code(status))
	{
	}

	/// \brief Return the underlying error_code
	[[nodiscard]] const std::error_code &code() const noexcept { return m_code; }
	/// \brief Return the HTTP status code
	[[nodiscard]] status_type status() const noexcept { return static_cast<status_type>(m_code.value()); }

  private:
	std::error_code m_code;
};

} // namespace zeep::http