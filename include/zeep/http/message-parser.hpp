// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::{request,reply}_parser classes that parse HTTP input/output

#include <zeep/config.hpp>

#include <tuple>

#include <zeep/http/reply.hpp>
#include <zeep/http/request.hpp>

namespace zeep::http
{

/// An HTTP message parser with support for Transfer-Encoding: Chunked

// --------------------------------------------------------------------
// A simple reimplementation of boost::tribool

class parse_result
{
  public:
	enum value_type
	{
		true_value,
		false_value,
		indeterminate_value
	} m_value;

	constexpr parse_result() noexcept
		: m_value(false_value)
	{
	}
	constexpr parse_result(bool init) noexcept
		: m_value(init ? true_value : false_value)
	{
	}
	constexpr parse_result(value_type init) noexcept
		: m_value(init)
	{
	}

	constexpr explicit operator bool() const noexcept { return m_value == true_value; }
};

constexpr parse_result indeterminate = parse_result(parse_result::indeterminate_value);

constexpr parse_result operator and(parse_result lhs, parse_result rhs)
{
	return (static_cast<bool>(not lhs) or static_cast<bool>(not rhs))
	           ? parse_result(false)
	           : ((static_cast<bool>(lhs) and static_cast<bool>(rhs)) ? parse_result(true) : indeterminate);
}

constexpr parse_result operator and(parse_result lhs, bool rhs)
{
	return rhs ? lhs : parse_result(false);
}

constexpr parse_result operator and(bool lhs, parse_result rhs)
{
	return lhs ? rhs : parse_result(false);
}

constexpr parse_result operator or(parse_result lhs, parse_result rhs)
{
	return (static_cast<bool>(not lhs) and static_cast<bool>(not rhs))
	           ? parse_result(false)
	           : ((static_cast<bool>(lhs) or static_cast<bool>(rhs)) ? parse_result(true) : indeterminate);
}

constexpr parse_result operator or(parse_result lhs, bool rhs)
{
	return rhs ? parse_result(true) : lhs;
}

constexpr parse_result operator or(bool lhs, parse_result rhs)
{
	return lhs ? parse_result(true) : rhs;
}

constexpr parse_result operator==(parse_result lhs, parse_result rhs)
{
	return (lhs.m_value == parse_result::indeterminate_value or rhs.m_value == parse_result::indeterminate_value) ? indeterminate : ((lhs and rhs) or (not lhs and not rhs));
}

// --------------------------------------------------------------------
class parser
{
  public:
	virtual ~parser() {}

	virtual void reset();

	parse_result parse_header_lines(char ch);
	parse_result parse_chunk(char ch);
	parse_result parse_footer(char ch);
	parse_result parse_content(char ch);

  protected:
	typedef parse_result (parser::*state_parser)(char ch);

	parser();

	state_parser m_parser;
	int m_state;
	unsigned int m_chunk_size;
	std::string m_data;
	std::string m_uri;
	std::string m_method;

	bool m_parsing_content;
	bool m_collect_payload;
	int m_http_version_major, m_http_version_minor;

	std::vector<header> m_headers;
	std::string m_payload;
};

class request_parser : public parser
{
  public:
	request_parser();

	parse_result parse(std::streambuf &text);

	request get_request();

  private:
	parse_result parse_initial_line(char ch);
};

class reply_parser : public parser
{
  public:
	reply_parser();

	parse_result parse(std::streambuf &text);

	reply get_reply();

	virtual void reset();

  private:
	parse_result parse_initial_line(char ch);

	int m_status = 0;
	std::string m_status_line;
};

} // namespace zeep::http
