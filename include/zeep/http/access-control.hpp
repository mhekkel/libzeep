// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2025-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the zeep::http::access_control class, that handles CORS for HTTP connections

#include "zeep/unicode-support.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace zeep::http
{

class reply;

/// \brief Handles CORS (Cross-Origin Resource Sharing) for HTTP connections

class access_control
{
  public:
	access_control() = default;
	virtual ~access_control() = default;

	access_control(access_control &&) = default;
	access_control &operator=(access_control &&) = default;

	/// Constructor with a default \a allow_origin string and a flag \a allow_credentials
	/// that will trigger addition of a "Access-Control-Allow-Credentials" header
	access_control(std::string allow_origin, bool allow_credentials)
		: m_allow_origin(std::move(allow_origin))
		, m_allowed_headers({ "Keep-Alive", "User-Agent", "If-Modified-Since", "Cache-Control", "Content-Type" })
		, m_allow_credentials(allow_credentials)
	{
	}

	/// \brief Set the "Access-Control-Allow-Origin" header value
	/// \param allow_origin  The allowed origin (e.g. "*" or a specific URL)
	void set_allow_origin(std::string allow_origin) noexcept
	{
		m_allow_origin = std::move(allow_origin);
	}

	/// \brief Set the "Access-Control-Allow-Credentials" header value
	/// \param allow_credentials  Whether credentials are allowed
	void set_allow_credentials(bool allow_credentials) noexcept
	{
		m_allow_credentials = allow_credentials;
	}

	/// \brief Set the "Access-Control-Allow-Headers" header value
	/// \param allowed_headers  Comma-separated list of allowed headers
	void set_allowed_headers(std::string_view allowed_headers)
	{
		split(m_allowed_headers, allowed_headers, ",");
	}

	/// \brief Add a single header to the "Access-Control-Allow-Headers" list
	/// \param allowed_header  The header name to add
	void add_allowed_header(std::string allowed_header)
	{
		m_allowed_headers.emplace_back(std::move(allowed_header));
	}

	/// \brief Add the CORS headers to the specified reply
	/// \param rep  The reply to add CORS headers to
	virtual void get_access_control_headers(reply &rep) const;

  private:
	/// @cond
	std::string m_allow_origin;
	std::vector<std::string> m_allowed_headers;
	bool m_allow_credentials = false;
	/// @endcond
};

} // namespace zeep::http
