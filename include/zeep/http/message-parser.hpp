// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the zeep::http::{request,reply}_parser classes that parse HTTP input/output

#include "zeep/http/header.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace zeep::http
{

// --------------------------------------------------------------------

/// \brief A simple reimplementation of boost::tribool for HTTP parsing

class parse_result
{
  public:
	/// \brief The possible parse states
	enum value_type
	{
		true_value,         ///< Parsing succeeded
		false_value,        ///< Parsing failed
		indeterminate_value ///< More data needed
	} m_value;

	/// \brief Default constructor — initializes to false
	constexpr parse_result() noexcept
		: m_value(false_value)
	{
	}
	/// \brief Construct from a boolean
	/// \param init  The initial value
	constexpr parse_result(bool init) noexcept
		: m_value(init ? true_value : false_value)
	{
	}
	/// \brief Construct from a value_type enumerator
	/// \param init  The initial value
	constexpr parse_result(value_type init) noexcept
		: m_value(init)
	{
	}

	/// \brief Check if the result is true (success)
	constexpr explicit operator bool() const noexcept { return m_value == true_value; }
	/// \brief Check if the result is false (failure)
	constexpr bool operator not() const noexcept { return m_value == false_value; }
};

/// \brief Sentinel value indicating that parsing is incomplete and more data is needed
constexpr parse_result::value_type indeterminate = parse_result::indeterminate_value;

/// \brief Logical AND of two parse_results
constexpr parse_result operator and(parse_result lhs, parse_result rhs) noexcept
{
	return (static_cast<bool>(not lhs) or static_cast<bool>(not rhs))
	           ? parse_result(false)
	           : ((static_cast<bool>(lhs) and static_cast<bool>(rhs)) ? parse_result(true) : indeterminate);
}

/// \brief Logical AND of a parse_result and a bool
constexpr parse_result operator and(parse_result lhs, bool rhs) noexcept
{
	return rhs ? lhs : parse_result(false);
}

/// \brief Logical AND of a bool and a parse_result
constexpr parse_result operator and(bool lhs, parse_result rhs) noexcept
{
	return lhs ? rhs : parse_result(false);
}

/// \brief Logical OR of two parse_results
constexpr parse_result operator or(parse_result lhs, parse_result rhs) noexcept
{
	return (static_cast<bool>(not lhs) and static_cast<bool>(not rhs))
	           ? parse_result(false)
	           : ((static_cast<bool>(lhs) or static_cast<bool>(rhs)) ? parse_result(true) : indeterminate);
}

/// \brief Logical OR of a parse_result and a bool
constexpr parse_result operator or(parse_result lhs, bool rhs) noexcept
{
	return rhs ? parse_result(true) : lhs;
}

/// \brief Logical OR of a bool and a parse_result
constexpr parse_result operator or(bool lhs, parse_result rhs) noexcept
{
	return lhs ? parse_result(true) : rhs;
}

/// \brief Compare a parse_result with a value_type enumerator
constexpr parse_result operator==(parse_result lhs, parse_result::value_type rhs) noexcept
{
	return lhs.m_value == rhs;
}

/// \brief Equality comparison of two parse_results
constexpr parse_result operator==(parse_result lhs, parse_result rhs) noexcept
{
	return (lhs == indeterminate or rhs == indeterminate) ? indeterminate : ((lhs and rhs) or (not lhs and not rhs));
}

// --------------------------------------------------------------------

/// \brief Base class for message parsers.
class parser
{
  public:
	virtual ~parser() = default;

	/// \brief Reset the parser to its initial state
	virtual void reset() noexcept;

	/// \brief Parse a single character of header lines
	/// \param ch  A character from the HTTP header
	/// \return    The parse result
	[[nodiscard]] parse_result parse_header_lines(char ch);

	/// \brief Parse a single character of a chunked transfer encoding chunk
	/// \param ch  A character from the chunk data
	/// \return    The parse result
	[[nodiscard]] parse_result parse_chunk(char ch);

	/// \brief Parse a single character of a chunked transfer encoding footer
	/// \param ch  A character from the footer
	/// \return    The parse result
	[[nodiscard]] parse_result parse_footer(char ch);

	/// \brief Parse a single character of the message body content
	/// \param ch  A character from the content
	/// \return    The parse result
	[[nodiscard]] parse_result parse_content(char ch);

	/// \brief Set the maximum allowed total size of the header section
	///        (request/reply line plus all header lines) in bytes.
	/// \param size  The maximum size in bytes
	void set_max_header_size(size_t size) noexcept { m_max_header_size = size; }

	/// \brief Set the maximum allowed size of the message body in bytes.
	/// \param size  The maximum size in bytes
	void set_max_payload_size(size_t size) noexcept { m_max_payload_size = size; }

  protected:
	/// @cond
	using state_parser = parse_result (parser::*)(char ch);

	parser() = default;

	/// \brief Account for \a n more bytes of header data; returns false when the
	///        configured maximum header size has been exceeded.
	[[nodiscard]] bool grow_header(size_t n) noexcept
	{
		m_header_size += n;
		return m_header_size <= m_max_header_size;
	}

	parse_result post_process_headers();

	[[nodiscard]] bool find_last_token(const header &h, std::string_view t) const;

	state_parser m_parser = nullptr;
	int m_state = 0;
	unsigned int m_chunk_size = 0;
	std::string m_data;
	std::string m_uri;
	std::string m_method;

	bool m_parsing_content = false;
	bool m_collect_payload = true;
	int m_http_version_major = 1, m_http_version_minor = 0;

	size_t m_header_size = 0;
	size_t m_max_header_size = 64 * 1024;
	size_t m_max_payload_size = 100 * 1024 * 1024;

	std::vector<header> m_headers;
	std::string m_payload;
	/// @endcond
};

/// \brief Parser for request messages
class request_parser : public parser
{
  public:
	request_parser() = default;

	/// \brief Parse an HTTP request from a stream buffer
	/// \param text  The input stream buffer
	/// \return      The parse result
	parse_result parse(std::streambuf &text);

	/// \brief Retrieve the parsed request
	/// \return The parsed HTTP request
	[[nodiscard]] request get_request();

  private:
	/// @cond
	parse_result parse_initial_line(char ch);

	// parse_result post_process_headers() override;
	/// @endcond
};

/// \brief Parser for reply messages
class reply_parser : public parser
{
  public:
	reply_parser() = default;

	/// \brief Parse an HTTP reply from a stream buffer
	/// \param text  The input stream buffer
	/// \return      The parse result
	parse_result parse(std::streambuf &text);

	/// \brief Retrieve the parsed reply
	/// \return The parsed HTTP reply
	[[nodiscard]] reply get_reply();

	/// \brief Reset the parser to its initial state
	void reset() noexcept override;

  private:
	/// @cond
	parse_result parse_initial_line(char ch);

	int m_status = 0;
	std::string m_status_line;
	/// @endcond
};

} // namespace zeep::http
