//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <sstream>

#include <zeep/http/message_parser.hpp>

#include <boost/algorithm/string.hpp>

using namespace std;
namespace ba = boost::algorithm;

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

boost::tribool parser::parse_header_lines(vector<header>& headers, string& payload, char ch)
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
				m_parser = &parser::parse_empty_line;

				for (vector<header>::iterator h = headers.begin(); h != headers.end(); ++h)
				{
					if (ba::iequals(h->name, "Transfer-Encoding") and ba::iequals(h->value, "chunked"))
					{
						m_parser = &parser::parse_chunk;
						m_parsing_content = true;
						break;
					}
					else if (ba::iequals(h->name, "Content-Length"))
					{
						stringstream s(h->value);
						s >> m_chunk_size;
						payload.reserve(m_chunk_size);
						m_state = 0;
						m_parser = &parser::parse_content;
						break;
					}
				}
				
				result = (this->*m_parser)(headers, payload, ch);
			}
			else if ((ch == ' ' or ch == '\t') and not headers.empty())
				m_state = 10;
			else if (iscntrl(ch) or detail::is_tspecial(ch))
				result = false;
			else
			{
				headers.push_back(header());
				headers.back().name += ch;
				m_state = 1;
			}
			break;
		
		case 1:
			if (ch == ':')
				++m_state;
			else if (iscntrl(ch) or detail::is_tspecial(ch))
				result = false;
			else
				headers.back().name += ch;
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
			else
				headers.back().value += ch;
			break;
		
		case 4:
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
				headers.back().value += ch;
				m_state = 3;
			}
			break;
		
	}
	
	return result;
}

boost::tribool parser::parse_empty_line(vector<header>& headers, string& payload, char ch)
{
	boost::tribool result = boost::indeterminate;
	
	switch (m_state)
	{
		case 0: if (ch == '\r') ++m_state; else result = false; break;
		case 1:
			if (ch == '\n')
			{
				result = true;
				if (m_chunk_size > 0)
					m_parsing_content = true;
			}
			else
				result = false;
			break;
	}
	
	return result;
}

boost::tribool parser::parse_chunk(vector<header>& headers, string& payload, char ch)
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
					payload.reserve(payload.size() + m_chunk_size);
					++m_state;
				}
				else
					m_state = 2;
			}
			else
				result = false;
			break;
		
		case 6:
			if (m_collect_payload)
				payload += ch;
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

boost::tribool parser::parse_content(vector<header>& headers, string& payload, char ch)
{
	boost::tribool result = boost::indeterminate;
	
	// here we simply read m_chunk_size of bytes and finish
	
	switch (m_state)
	{
		case 0:	if (ch == '\r')	++m_state; else result = false; break;
		case 1: if (ch == '\n') ++m_state; else result = false; break;
		
		case 2:
			if (m_collect_payload)
				payload += ch;
			if (--m_chunk_size == 0)
			{
				result = true;
				m_parsing_content = false;
			}
			break;
	}
	
	return result;
}

// --------------------------------------------------------------------
// 

request_parser::request_parser()
{
}

parser::result_type request_parser::parse(request& req, const char* text, size_t length)
{
	if (m_parser == NULL)
	{
		// clear the request
		req.clear();
		m_parser = static_cast<state_parser>(&request_parser::parse_initial_line);
		m_parsing_content = false;
	}

	boost::tribool result = boost::indeterminate;
	size_t used = 0;
	
	while (used < length and boost::indeterminate(result))
		result = (this->*m_parser)(req.headers, req.payload, text[used++]);

	if (result)
	{
		req.uri = m_uri;
		req.method = m_method;
		req.http_version_major = m_http_version_major;
		req.http_version_minor = m_http_version_minor;
	}

	return tie(result, used);
}

parser::result_type request_parser::parse_header(request& req, const char* text, size_t length)
{
	if (m_parser == NULL)
	{
		// clear the reply
		req.clear();
		m_parser = static_cast<state_parser>(&request_parser::parse_initial_line);
	}

	boost::tribool result = boost::indeterminate;
	size_t used = 0;
	
	while (used < length and boost::indeterminate(result))
	{
		result = (this->*m_parser)(req.headers, req.payload, text[used++]);
		
		if (boost::indeterminate(result) and m_parsing_content)
			result = true;
	}

	if (result)
	{
		req.http_version_major = m_http_version_major;
		req.http_version_minor = m_http_version_minor;
	}

	return tie(result, used);
}

parser::result_type request_parser::parse_content(request& req, const char* text, size_t length)
{
	boost::tribool result = boost::indeterminate;
	size_t used = 0;

	if (not m_parsing_content)
		result = false;
	else
	{
		m_collect_payload = false;
		
		while (used < length and boost::indeterminate(result))
		{
			result = (this->*m_parser)(req.headers, req.payload, text[used++]);
			
			if (boost::indeterminate(result) and m_parsing_content)
				result = true;
		}
	}
	
	return tie(result, used);
}

boost::tribool request_parser::parse(request& req, streambuf& text)
{
	if (m_parser == NULL)
	{
		// clear the request
		req.clear();
		m_parser = static_cast<state_parser>(&request_parser::parse_initial_line);
		m_parsing_content = false;
	}

	boost::tribool result = boost::indeterminate;
	
	while (text.in_avail() > 0 and boost::indeterminate(result))
		result = (this->*m_parser)(req.headers, req.payload, text.sbumpc());

	if (result)
	{
		req.uri = m_uri;
		req.method = m_method;
		req.http_version_major = m_http_version_major;
		req.http_version_minor = m_http_version_minor;
	}

	return result;
}

boost::tribool request_parser::parse_header(request& req, streambuf& text)
{
	if (m_parser == NULL)
	{
		// clear the reply
		req.clear();
		m_parser = static_cast<state_parser>(&request_parser::parse_initial_line);
	}

	boost::tribool result = boost::indeterminate;
	size_t used = 0;
	
	while (text.in_avail() > 0 and boost::indeterminate(result))
	{
		result = (this->*m_parser)(req.headers, req.payload, text.sbumpc());
		
		if (boost::indeterminate(result) and m_parsing_content)
			result = true;
	}

	if (result)
	{
		req.http_version_major = m_http_version_major;
		req.http_version_minor = m_http_version_minor;
	}

	return result;
}

boost::tribool request_parser::parse_content(request& req, streambuf& text, streambuf& sink)
{
	boost::tribool result = boost::indeterminate;
	size_t used = 0;

	if (not m_parsing_content)
		result = false;
	else
	{
		m_collect_payload = false;
		
		while (text.in_avail() > 0 and boost::indeterminate(result))
		{
			char ch = text.sbumpc();
			result = (this->*m_parser)(req.headers, req.payload, ch);
			sink.sputc(ch);
			
			if (boost::indeterminate(result) and m_parsing_content)
				result = true;
		}
	}
	
	return result;
}

boost::tribool request_parser::parse_initial_line(vector<header>& headers, string& payload, char ch)
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

parser::result_type reply_parser::parse(reply& rep, const char* text, size_t length)
{
	if (m_parser == NULL)
	{
		// clear the reply
		rep.clear();
		m_parser = static_cast<state_parser>(&reply_parser::parse_initial_line);
		m_parsing_content = false;
	}

	boost::tribool result = boost::indeterminate;
	size_t used = 0;
	
	while (used < length and boost::indeterminate(result))
		result = (this->*m_parser)(rep.m_headers, rep.m_content, text[used++]);

	if (result)
	{
		rep.m_status = static_cast<status_type>(m_status);
		rep.m_status_line = m_status_line;
		rep.m_version_major = m_http_version_major;
		rep.m_version_minor = m_http_version_minor;
	}

	return tie(result, used);
}

parser::result_type reply_parser::parse_header(reply& rep, const char* text, size_t length)
{
	if (m_parser == NULL)
	{
		// clear the reply
		rep.clear();
		m_parser = static_cast<state_parser>(&reply_parser::parse_initial_line);
	}

	boost::tribool result = boost::indeterminate;
	size_t used = 0;
	
	while (used < length and boost::indeterminate(result))
	{
		result = (this->*m_parser)(rep.m_headers, rep.m_content, text[used++]);
		
		if (boost::indeterminate(result) and m_parsing_content)
			result = true;
	}

	if (result)
	{
		rep.m_status = static_cast<status_type>(m_status);
		rep.m_status_line = m_status_line;
		rep.m_version_major = m_http_version_major;
		rep.m_version_minor = m_http_version_minor;
	}

	return tie(result, used);
}

parser::result_type reply_parser::parse_content(reply& rep, const char* text, size_t length)
{
	boost::tribool result = boost::indeterminate;
	size_t used = 0;

	if (not m_parsing_content)
		result = false;
	else
	{
		m_collect_payload = false;
		
		while (used < length and boost::indeterminate(result))
			result = (this->*m_parser)(rep.m_headers, rep.m_content, text[used++]);
	}
	
	return tie(result, used);
}

boost::tribool reply_parser::parse(reply& rep, streambuf& text)
{
	if (m_parser == NULL)
	{
		// clear the reply
		rep.clear();
		m_parser = static_cast<state_parser>(&reply_parser::parse_initial_line);
		m_parsing_content = false;
	}

	boost::tribool result = boost::indeterminate;
	size_t used = 0;

	while (text.in_avail() and boost::indeterminate(result))
		result = (this->*m_parser)(rep.m_headers, rep.m_content, text.sbumpc());

	if (result)
	{
		rep.m_status = static_cast<status_type>(m_status);
		rep.m_status_line = m_status_line;
		rep.m_version_major = m_http_version_major;
		rep.m_version_minor = m_http_version_minor;
	}

	return result;
}

boost::tribool reply_parser::parse_header(reply& rep, streambuf& text)
{
	if (m_parser == NULL)
	{
		// clear the reply
		rep.clear();
		m_parser = static_cast<state_parser>(&reply_parser::parse_initial_line);
	}

	boost::tribool result = boost::indeterminate;
	size_t used = 0;

	while (text.in_avail() and boost::indeterminate(result))
	{
		result = (this->*m_parser)(rep.m_headers, rep.m_content, text.sbumpc());

		if (boost::indeterminate(result) and m_parsing_content)
			result = true;
	}

	if (result)
	{
		rep.m_status = static_cast<status_type>(m_status);
		rep.m_status_line = m_status_line;
		rep.m_version_major = m_http_version_major;
		rep.m_version_minor = m_http_version_minor;
	}

	return result;
}

boost::tribool reply_parser::parse_content(reply& rep, streambuf& text, streambuf& sink)
{
	boost::tribool result = boost::indeterminate;
	size_t used = 0;

	if (not m_parsing_content)
		result = false;
	else
	{
		m_collect_payload = false;

		while (text.in_avail() and boost::indeterminate(result))
		{
			char ch = text.sbumpc();
			result = (this->*m_parser)(rep.m_headers, rep.m_content, ch);
			sink.sputc(ch);
		}
	}

	return result;
}

boost::tribool reply_parser::parse_initial_line(vector<header>& headers, string& payload, char ch)
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

} // http
} // zeep
