// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <sstream>

#include <zeep/http/message-parser.hpp>
#include <zeep/unicode-support.hpp>

namespace zeep::http 
{
	
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

parser::parser()
{
	reset();
}

void parser::reset()
{
	m_parser = NULL;
	m_state = 0;
	m_chunk_size = 0;
	m_data.clear();
	m_uri.clear();
	m_method.clear();
	m_parsing_content = false;
	m_collect_payload = true;
	m_http_version_major = 1;
	m_http_version_minor = 0;
}

boost::tribool parser::parse_header_lines(char ch)
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
				m_state = 20;
			else if ((ch == ' ' or ch == '\t') and not m_headers.empty())
				m_state = 10;
			else if (iscntrl(ch) or detail::is_tspecial(ch))
				result = false;
			else
			{
				m_headers.push_back(header());
				m_headers.back().name += ch;
				m_state = 1;
			}
			break;
		
		case 1:
			if (ch == ':')
				++m_state;
			else if (iscntrl(ch) or detail::is_tspecial(ch))
				result = false;
			else
				m_headers.back().name += ch;
			break;
		
		case 2:
			if (ch == ' ')
				++m_state;
			else
				result = false;
			break;
		
		case 3:
			if (ch == '\r')
				m_state += 2;
			else if (ch != ' ')
			{
				m_headers.back().value += ch;
				++m_state;
			}
			break;

		case 4:
			if (ch == '\r')
				++m_state;
			else
				m_headers.back().value += ch;
			break;
		
		case 5:
			if (ch == '\n')
				m_state = 0;
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
				m_headers.back().value += ch;
				m_state = 3;
			}
			break;
		
		case 20:
			if (ch == '\n')
			{
				result = true;

				for (std::vector<header>::iterator h = m_headers.begin(); h != m_headers.end(); ++h)
				{
					if (iequals(h->name, "Transfer-Encoding") and iequals(h->value, "chunked"))
					{
						m_parser = &parser::parse_chunk;
						m_state = 0;
						m_parsing_content = true;
						break;
					}

					if (iequals(h->name, "Content-Length"))
					{
						std::stringstream s(h->value);
						s >> m_chunk_size;

						if (m_chunk_size > 0)
						{
							m_parser = &parser::parse_content;
							m_parsing_content = true;
							m_payload.reserve(m_chunk_size);
						}
						else
							m_parsing_content = false;
					}
					else if (iequals(h->name, "Connection") and iequals(h->value, "Close") and (m_method == "POST" or m_method == "PUT"))
					{
						m_parser = &parser::parse_content;
						m_parsing_content = true;
					}
				}
			}
			else
				result = false;
			break;
	}
	
	return result;
}

boost::tribool parser::parse_chunk(char ch)
{
	boost::tribool result = boost::indeterminate;
	
	switch (m_state)
	{
		// Transfer-Encoding: Chunked
		// lines starting with hex encoded length, optionally followed by text
		// then a newline (\r\n) and the actual length bytes.
		// This repeats until length is zero
		
			// new chunk, starts with hex encoded length
		case 0:
			if (isxdigit(ch))
			{
				m_data = ch;
				++m_state;
			}
			else
				result = false;
			break;
		
		case 1:
			if (isxdigit(ch))
				m_data += ch;
			else if (ch == ';')
				++m_state;
			else if (ch == '\r')
				m_state = 3;
			else
				result = false;
			break;
		
		case 2:
			if (ch == '\r')
				++m_state;
			else if (detail::is_tspecial(ch) or iscntrl(ch))
				result = false;
			break;
		
		case 3:
			if (ch == '\n')
			{
				std::stringstream s(m_data);
				s >> std::hex >> m_chunk_size;

				if (m_chunk_size > 0)
				{
					m_payload.reserve(m_payload.size() + m_chunk_size);
					++m_state;
				}
				else
					m_state = 10;
			}
			else
				result = false;
			break;
		
		case 4:
			if (m_collect_payload)
				m_payload += ch;
			if (--m_chunk_size == 0)
				m_state = 5;		// parse trailing \r\n
			break;
		
		case 5:	if (ch == '\r')	++m_state; else result = false; break;
		case 6: if (ch == '\n') m_state = 0; else result = false; break;
		
			// trailing \r\n		
		case 10: if (ch == '\r') ++m_state; else result = false; break;
		case 11: if (ch == '\n') result = true; else result = false; break;
	}
	
	return result;
}

boost::tribool parser::parse_content(char ch)
{
	boost::tribool result = boost::indeterminate;
	
	// here we simply read m_chunk_size of bytes and finish
	if (m_collect_payload)
		m_payload += ch;

	if (--m_chunk_size == 0)
	{
		result = true;
		m_parsing_content = false;
	}
	
	return result;
}

// --------------------------------------------------------------------
// 

request_parser::request_parser()
{
}

boost::tribool request_parser::parse(std::streambuf& text)
{
	if (m_parser == NULL)
	{
		m_parser = static_cast<state_parser>(&request_parser::parse_initial_line);
		m_parsing_content = false;
	}

	boost::tribool result = boost::indeterminate;
	
	bool is_parsing_content = m_parsing_content;
	while (text.in_avail() > 0 and boost::indeterminate(result))
	{
		result = (this->*m_parser)(text.sbumpc());

		if (result and is_parsing_content == false and m_parsing_content == true)
		{
			is_parsing_content = true;
			result = boost::indeterminate;
		}
	}

	return result;
}

request request_parser::get_request()
{
	return request(m_method, std::move(m_uri), { m_http_version_major, m_http_version_minor },
		std::move(m_headers), std::move(m_payload));
}

boost::tribool request_parser::parse_initial_line(char ch)
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
				m_method += ch;
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
				m_uri += ch;
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
					m_http_version_minor = 1;
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
				m_parser = &parser::parse_header_lines;
			}
			else
				result = false;
			break;
	}
	
	return result;
}

// --------------------------------------------------------------------
// 

reply_parser::reply_parser()
{
}

void reply_parser::reset()
{
	parser::reset();
	m_status = 0;
	m_status_line.clear();
}

boost::tribool reply_parser::parse(std::streambuf& text)
{
	if (m_parser == NULL)
	{
		m_parser = static_cast<state_parser>(&reply_parser::parse_initial_line);
		m_parsing_content = false;
	}

	boost::tribool result = boost::indeterminate;

	bool is_parsing_content = m_parsing_content;
	while (text.in_avail() and boost::indeterminate(result))
	{
		result = (this->*m_parser)(text.sbumpc());

		if (result and is_parsing_content == false and m_parsing_content == true)
		{
			is_parsing_content = true;
			result = boost::indeterminate;
		}
	}

	return result;
}

reply reply_parser::get_reply()
{
	return { static_cast<status_type>(m_status), {m_http_version_major, m_http_version_minor }, std::move(m_headers), std::move(m_payload) };
}

boost::tribool reply_parser::parse_initial_line(char ch)
{
	boost::tribool result = boost::indeterminate;
	
	// a state machine to parse the initial reply line
	// which consists of:
	// HTTP/1.{0,1} XXX status-message
	
	switch (m_state)
	{
		// we're parsing the initial HTTP/1.x here
		case 0:	if (ch == 'H') ++m_state; else result = false;	break;
		case 1:	if (ch == 'T') ++m_state; else result = false;	break;
		case 2:	if (ch == 'T') ++m_state; else result = false;	break;
		case 3:	if (ch == 'P') ++m_state; else result = false;	break;
		case 4:	if (ch == '/') ++m_state; else result = false;	break;
		case 5:	if (ch == '1') ++m_state; else result = false;	break;
		case 6:	if (ch == '.') ++m_state; else if (ch == '\r') m_state = 11; else result = false;	break;
		case 7:
			if (ch == '1' or ch == '0')
			{
				if (ch == '1')
					m_http_version_minor = 1;
				++m_state;
			}
			else
				result = false;
			break;
		
		case 8: if (isspace(ch)) ++m_state; else result = false; break;

		// we're parsing the result code here (three digits)
		case 9:
			if (isdigit(ch))
			{
				m_status = 100 * (ch - '0');
				++m_state;
			}
			else
				result = false;
			break;

		case 10:
			if (isdigit(ch))
			{
				m_status += 10 * (ch - '0');
				++m_state;
			}
			else
				result = false;
			break;

		case 11:
			if (isdigit(ch))
			{
				m_status += 1 * (ch - '0');
				++m_state;
			}
			else
				result = false;
			break;

		case 12: if (isspace(ch)) ++m_state; else result = false; break;

		// we're parsing the status message here
		case 13:
			if (ch == '\r')
				++m_state;
			else
				m_status_line += ch;
			break;

		case 14:
			if (ch == '\n')
			{
				m_state = 0;
				m_parser = &parser::parse_header_lines;
			}
			else
				result = false;
			break;
	}
	
	return result;
}

} // zeep::http
