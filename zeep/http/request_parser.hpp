// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_REQUEST_PARSER_HPP
#define SOAP_HTTP_REQUEST_PARSER_HPP

#include <boost/logic/tribool.hpp>

#include <zeep/http/request.hpp>

namespace zeep { namespace http {

// An HTTP request parser with support for Transfer-Encoding: Chunked

class request_parser
{
  public:
						request_parser();
	
	void				reset();
	
	boost::tribool		parse(request& req, const char* text, size_t length);
	
  private:
	typedef boost::tribool (request_parser::*state_parser)(request& req, char ch);
	
	boost::tribool		parse_initial_line(request& req, char ch);
	boost::tribool		parse_header(request& req, char ch);
	boost::tribool		parse_empty_line(request& req, char ch);
	boost::tribool		parse_chunk(request& req, char ch);
	boost::tribool		parse_footer(request& req, char ch);
	boost::tribool		parse_content(request& req, char ch);

	state_parser		m_parser;
	int					m_state;
	unsigned int		m_chunk_size;
	std::string			m_data;
};

}
}

#endif
