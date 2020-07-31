// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::{request,reply}_parser classes that parse HTTP input/output

#include <zeep/config.hpp>

#include <tuple>

#include <boost/logic/tribool.hpp>

#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>

namespace zeep::http
{

/// An HTTP message parser with support for Transfer-Encoding: Chunked

class parser
{
  public:
	virtual ~parser() {}

	virtual void reset();

	boost::tribool parse_header_lines(char ch);
	boost::tribool parse_chunk(char ch);
	boost::tribool parse_footer(char ch);
	boost::tribool parse_content(char ch);

  protected:

	typedef boost::tribool (parser::*state_parser)(char ch);

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

	boost::tribool parse(std::streambuf& text);

	request get_request();

  private:
	boost::tribool parse_initial_line(char ch);
};

class reply_parser : public parser
{
  public:
	reply_parser();

	boost::tribool parse(std::streambuf& text);

	reply get_reply();

	virtual void reset();

  private:
	boost::tribool parse_initial_line(char ch);

	int m_status = 0;
	std::string m_status_line;
};

} // namespace zeep::http
