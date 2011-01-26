//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>

#include <zeep/http/request_parser.hpp>

using namespace std;

namespace zeep { namespace http {
	
namespace detail {

bool is_tspecial(int c)
{
	switch (c)
	{
		case '(': case ')': case '<': case '>': case '@':
		case ',': case ';': case ':': case '\\': case '"':
		case '/': case '[': case ']': case '?': case '=':
		case '{': case '}': case ' ': case '\t':
			return true;
		default:
			return false;
	}
}

} // detail

request_parser::request_parser()
	: m_parser(NULL)
{
}

void request_parser::reset()
{
	m_parser = NULL;
}

boost::tribool request_parser::parse(request& req,
	const char* text, unsigned int length)
{
	if (m_parser == NULL)
	{
		m_state = 0;
		m_data.clear();
		req.version = http_version_1_0;
		req.close = false;
		m_parser = &request_parser::parse_initial_line;
	}

	boost::tribool result = boost::indeterminate;
	
	while (length-- > 0 and boost::indeterminate(result))
		result = (this->*m_parser)(req, *text++);

	return result;
}

boost::tribool request_parser::parse_initial_line(request& req, char ch)
{
	boost::tribool result = boost::indeterminate;
	
	// a state machine to parse the initial request line
	// which consists of:
	// METHOD URI HTTP/1.0 (or 1.1)
	
	switch (m_state)
	{
		// we're parsing the method here
		case 0:
			if (isalpha(ch))
				req.method += ch;
			else if (ch == ' ')
				++m_state;
			else
				result = false;
			break;

		// we're parsing the URI here
		case 1:
			if (ch == ' ')
				++m_state;
			else if (iscntrl(ch))
				result = false;
			else
				req.uri += ch;
			break;
		
		// we're parsing the trailing HTTP/1.x here
		case 2:	if (ch == 'H') ++m_state; else result = false;	break;
		case 3:	if (ch == 'T') ++m_state; else result = false;	break;
		case 4:	if (ch == 'T') ++m_state; else result = false;	break;
		case 5:	if (ch == 'P') ++m_state; else result = false;	break;
		case 6:	if (ch == '/') ++m_state; else result = false;	break;
		case 7:	if (ch == '1') ++m_state; else result = false;	break;
		case 8:	if (ch == '.') ++m_state; else if (ch == '\r') m_state = 11; else result = false;	break;
		case 9:
			if (ch == '1' or ch == '0')
			{
				if (ch == '1')
					req.version = http_version_1_1;
				++m_state;
			}
			else
				result = false;
			break;
		case 10: if (ch == '\r') ++m_state; else result = false;	break;
		case 11:
			if (ch == '\n')
			{
				m_state = 0;
				m_parser = &request_parser::parse_header;
			}
			else
				result = false;
			break;
	}
	
	return result;
}

boost::tribool request_parser::parse_header(request& req, char ch)
{
	boost::tribool result = boost::indeterminate;
	
	// parse the header lines, consisting of
	// NAME: VALUE
	// optionally followed by more VALUE prefixed by white space on the next lines
	
	switch (m_state)
	{
		case 0:
			// If the header starts with \r, it is the start of an empty line
			// which indicates the end of the header section
			if (ch == '\r')
			{
				m_parser = &request_parser::parse_empty_line;

				for (vector<header>::iterator h = req.headers.begin(); h != req.headers.end(); ++h)
				{
					if (h->name == "Transfer-Encoding" and h->value == "chunked")
					{
						m_parser = &request_parser::parse_chunk;
						break;
					}
					else if (h->name == "Content-Length")
					{
						stringstream s(h->value);
						s >> m_chunk_size;
						req.payload.reserve(m_chunk_size);
						m_state = 0;
						m_parser = &request_parser::parse_content;
						break;
					}
				}
				
				result = (this->*m_parser)(req, ch);
			}
			else if ((ch == ' ' or ch == '\t') and not req.headers.empty())
				m_state = 10;
			else if (iscntrl(ch) or detail::is_tspecial(ch))
				result = false;
			else
			{
				req.headers.push_back(header());
				req.headers.back().name += ch;
				m_state = 1;
			}
			break;
		
		case 1:
			if (ch == ':')
				++m_state;
			else if (iscntrl(ch) or detail::is_tspecial(ch))
				result = false;
			else
				req.headers.back().name += ch;
			break;
		
		case 2:
			if (ch == ' ')
				++m_state;
			else
				result = false;
			break;
		
		case 3:
			if (ch == '\r')
				++m_state;
			else if (iscntrl(ch))
				result = false;
			else
				req.headers.back().value += ch;
			break;
		
		case 4:
			if (ch == '\n')
			{
				if (req.headers.back().name == "Connection" and
					req.headers.back().value == "close")
				{
					req.close = true;
				}	
				
				m_state = 0;
			}
			else
				result = false;
			break;
		
		case 10:
			if (ch == '\r')
				m_state = 4;
			else if (iscntrl(ch))
				result = false;
			else if (not (ch == ' ' or ch == '\t'))
			{
				req.headers.back().value += ch;
				m_state = 3;
			}
			break;
		
	}
	
	return result;
}

boost::tribool request_parser::parse_empty_line(request& req, char ch)
{
	boost::tribool result = boost::indeterminate;
	
	switch (m_state)
	{
		case 0: if (ch == '\r') ++m_state; else result = false; break;
		case 1: if (ch == '\n') result = true; else result = false; break;
	}
	
	return result;
}

boost::tribool request_parser::parse_chunk(request& req, char ch)
{
	boost::tribool result = boost::indeterminate;
	
	switch (m_state)
	{
		// Transfer-Encoding: Chunked
		// lines starting with hex encoded length, optionally followed by text
		// then a newline (\r\n) and the actual length bytes.
		// This repeats until length is zero
		
			// start with empty line...
		case 0:	if (ch == '\r')	++m_state; else result = false; break;
		case 1: if (ch == '\n') ++m_state; else result = false; break;
		
			// new chunk?
		case 2:
			if (isxdigit(ch))
			{
				m_data = ch;
				++m_state;
			}
			else if (ch == '\r')
				m_state = 10;
			else
				result = false;
			break;
		
		case 3:
			if (isxdigit(ch))
				m_data += ch;
			else if (ch == ';')
				++m_state;
			else if (ch == '\r')
				m_state = 5;
			else
				result = false;
			break;
		
		case 4:
			if (ch == '\r')
				++m_state;
			else if (detail::is_tspecial(ch) or iscntrl(ch))
				result = false;
			break;
		
		case 5:
			if (ch == '\n')
			{
				stringstream s(m_data);
				s >> hex >> m_chunk_size;

				if (m_chunk_size > 0)
				{
					req.payload.reserve(req.payload.size() + m_chunk_size);
					++m_state;
				}
				else
					m_state = 2;
			}
			else
				result = false;
			break;
		
		case 6:
			req.payload += ch;
			if (--m_chunk_size == 0)
				m_state = 0;		// restart
			break;
		
			// trailing \r\n		
		case 10:
			if (ch == '\n')
				result = true;
			else
				result = false;
			break;
	}
	
	return result;
}

boost::tribool request_parser::parse_content(request& req, char ch)
{
	boost::tribool result = boost::indeterminate;
	
	// here we simply read m_chunk_size of bytes and finish
	
	switch (m_state)
	{
		case 0:	if (ch == '\r')	++m_state; else result = false; break;
		case 1: if (ch == '\n') ++m_state; else result = false; break;
		
		case 2:
			req.payload += ch;
			if (--m_chunk_size == 0)
				result = true;
			break;
	}
	
	return result;
}

} // http
} // zeep
