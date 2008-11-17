#ifndef SOAP_HTTP_REQUEST_PARSER_HPP
#define SOAP_HTTP_REQUEST_PARSER_HPP

#include <boost/logic/tribool.hpp>

#include "soap/http/request.hpp"

namespace soap { namespace http {

// An HTTP request parser with support for Transfer-Encoding: Chunked

class request_parser
{
  public:
						request_parser();
	
	void				reset();
	
	boost::tribool		parse(request& req, const char* text, unsigned int length);

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
