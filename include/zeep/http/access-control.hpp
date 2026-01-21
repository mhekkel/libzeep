//        Copyright Maarten L. Hekkelman, 2025-2026
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

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

/// The zeep::http::access_control class handles CORS for HTTP connections

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

	/// Set the "Access-Control-Allow-Origin" header to \a allow_origin
	void set_allow_origin(std::string allow_origin)
	{
		m_allow_origin = std::move(allow_origin);
	}

	/// Set the "Access-Control-Allow-Credentials" header corresponding to \a allow_credentials
	void set_allow_credentials(bool allow_credentials)
	{
		m_allow_credentials = allow_credentials;
	}

	/// Set the "Access-Control-Allow-Headers" header to \a allowed_headers
	void set_allowed_headers(std::string_view allowed_headers)
	{
		split(m_allowed_headers, allowed_headers, ",");
	}

	/// Add \a allowed_headers to the list in the header "Access-Control-Allow-Headers"
	void add_allowed_header(std::string allowed_header)
	{
		m_allowed_headers.emplace_back(std::move(allowed_header));
	}

	/// Add the specified headers to @ref reply \a rep
	virtual void get_access_control_headers(reply &rep) const;

  private:
	std::string m_allow_origin;
	std::vector<std::string> m_allowed_headers;
	bool m_allow_credentials = false;
};

} // namespace zeep::http
