// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <tuple>

#include <boost/logic/tribool.hpp>

#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>

namespace zeep { namespace http {

/// An HTTP message parser with support for Transfer-Encoding: Chunked

class parser
{
  public:
	typedef std::tuple<boost::tribool,size_t> result_type;

	virtual				~parser() {}
	
	virtual void		reset();

  protected:
						parser();
	
	typedef boost::tribool (parser::*state_parser)(std::vector<header>& headers, std::string& payload, char ch);
	
	boost::tribool		parse_header(std::vector<header>& headers, std::string& payload, char ch);
	boost::tribool		parse_empty_line(std::vector<header>& headers, std::string& payload, char ch);
	boost::tribool		parse_chunk(std::vector<header>& headers, std::string& payload, char ch);
	boost::tribool		parse_footer(std::vector<header>& headers, std::string& payload, char ch);
	boost::tribool		parse_content(std::vector<header>& headers, std::string& payload, char ch);

	state_parser		m_parser;
	int					m_state;
	unsigned int		m_chunk_size;
	std::string			m_data;
	std::string			m_uri;
	std::string			m_method;
	bool				m_close;
	int					m_http_version_major, m_http_version_minor;
};

class request_parser : public parser
{
  public:
						request_parser();
	result_type			parse(request& req, const char* text, size_t length);

  private:
	boost::tribool		parse_initial_line(std::vector<header>& headers, std::string& payload, char ch);
};

class reply_parser : public parser
{
  public:
						reply_parser();
	result_type			parse(reply& req, const char* text, size_t length);

	virtual void		reset();

  private:
	boost::tribool		parse_initial_line(std::vector<header>& headers, std::string& payload, char ch);

	int					m_status;
	std::string			m_status_line;
};

}
}
