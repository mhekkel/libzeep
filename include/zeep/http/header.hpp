// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the zeep::http::header class

#include "zeep/exception.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace zeep::http
{

/// \brief Validate that a header name or value contains no CR or LF character.
///
/// CR/LF in a header field would allow an attacker to inject additional
/// headers or terminate the header block early (response splitting).
/// \param s  The header name or value to check.
/// \throw std::invalid_argument if \a s contains a carriage return or line feed.
inline void check_valid_header_field(std::string_view s)
{
	if (s.find_first_of("\r\n") != std::string_view::npos)
		throw exception("Header name or value contains CR or LF");
}

/// The header object contains the header lines as found in a
/// HTTP Request. The lines are parsed into name / value pairs.

struct header
{
	std::string name;
	std::string value;

	header() = default;
	header(const header &) = default;
	header &operator=(const header &) = default;
	header &operator=(header &&) = default;

	header(std::string name, std::string value)
		: name(std::move(name))
		, value(std::move(value))
	{
	}
};

} // namespace zeep::http
