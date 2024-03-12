// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/message-parser.hpp>
#include <zeep/unicode-support.hpp>

#include <charconv>

namespace zeep::http
{

namespace
{
	bool is_tspecial_or_cntrl(int c)
	{
		switch (c)
		{
			case '(':
			case ')':
			case '<':
			case '>':
			case '@':
			case ',':
			case ';':
			case ':':
			case '\\':
			case '"':
			case '/':
			case '[':
			case ']':
			case '?':
			case '=':
			case '{':
			case '}':
			case ' ':
			case 0x7f:
				return true;

			default:
				return c <= 0x1f;
		}
	}

} // namespace

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

parse_result parser::parse_header_lines(char ch)
{
	parse_result result = indeterminate;

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
			else if (is_tspecial_or_cntrl(ch))
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
			else if (is_tspecial_or_cntrl(ch))
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
			else if (not(ch == ' ' or ch == '\t'))
			{
				m_headers.back().value += ch;
				m_state = 3;
			}
			break;

		case 20:
			if (ch == '\n')
				result = post_process_headers();
			else
				result = false;
			break;
	}

	return result;
}

bool parser::find_last_token(const header &h, const std::string &t) const
{
	bool result = false;
	if (h.value.length() >= t.length())
	{
		auto ix = h.value.length() - t.length();

		result = iequals(h.value.substr(ix), t);
		if (result)
			result = ix == 0 or h.value[ix] == ' ' or h.value[ix] == ',';
	}

	return result;
}

parse_result parser::post_process_headers()
{
	parse_result result = true;

	auto i = find_if(m_headers.begin(), m_headers.end(), [](const header &h)
		{ return iequals(h.name, "transfer-encoding"); });
	if (i != m_headers.end())
	{
		if (find_last_token(*i, "chunked"))
		{
			m_parser = &parser::parse_chunk;
			m_state = 0;
			m_parsing_content = true;
		}
		else
			result = false;
	}
	else
	{
		i = find_if(m_headers.begin(), m_headers.end(), [](const header &h)
			{ return iequals(h.name, "content-length"); });
		if (i != m_headers.end())
		{
			auto r = std::from_chars(i->value.data(), i->value.data() + i->value.length(), m_chunk_size);
			if (r.ec != std::errc())
				result = false;
			else if (m_chunk_size)
			{
				m_parser = &parser::parse_content;
				m_parsing_content = true;
				m_payload.reserve(m_chunk_size);
			}
			else
				m_parsing_content = false;
		}
	}

	return result;
}

parse_result parser::parse_chunk(char ch)
{
	parse_result result = indeterminate;

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
			else if (is_tspecial_or_cntrl(ch))
				result = false;
			break;

		case 3:
			if (ch == '\n')
			{
				auto r = std::from_chars(m_data.data(), m_data.data() + m_data.length(), m_chunk_size, 16);

				if (r.ec != std::errc{})
					result = false;
				else if (m_chunk_size > 0)
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
				m_state = 5; // parse trailing \r\n
			break;

		case 5:
			if (ch == '\r')
				++m_state;
			else
				result = false;
			break;
		case 6:
			if (ch == '\n')
				m_state = 0;
			else
				result = false;
			break;

			// trailing \r\n
		case 10:
			if (ch == '\r')
				++m_state;
			else
				result = false;
			break;
		case 11:
			if (ch == '\n')
				result = true;
			else
				result = false;
			break;
	}

	return result;
}

parse_result parser::parse_content(char ch)
{
	parse_result result = indeterminate;

	// here we simply read m_chunk_size of bytes and finish
	if (m_collect_payload)
		m_payload += ch;

	if (m_payload.length() == m_chunk_size)
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

parse_result request_parser::parse(std::streambuf &text)
{
	if (m_parser == NULL)
	{
		m_parser = static_cast<state_parser>(&request_parser::parse_initial_line);
		m_parsing_content = false;
	}

	parse_result result = indeterminate;

	if (m_http_version_major == 0 and m_http_version_minor == 9)
		result = true;
	else
	{
		bool is_parsing_content = m_parsing_content;
		while (text.in_avail() > 0 and result == indeterminate)
		{
			result = (this->*m_parser)(static_cast<char>(text.sbumpc()));

			if (result and is_parsing_content == false and m_parsing_content == true)
			{
				is_parsing_content = true;
				result = indeterminate;
			}
		}
	}

	return result;
}

request request_parser::get_request()
{
	if (iequals(m_method, "CONNECT"))
	{
		// Special case, uri should be of the form HOST:PORT

		if (not is_valid_connect_host(m_uri))
			throw zeep::exception("Invalid host for CONNECT");

		m_uri = "http://" + m_uri;
	}

	return request(m_method, std::move(m_uri), { m_http_version_major, m_http_version_minor },
		std::move(m_headers), std::move(m_payload));
}

parse_result request_parser::parse_initial_line(char ch)
{
	parse_result result = indeterminate;

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
			else if (ch == '\r' or ch == '\n')
			{
				m_http_version_major = 0;
				m_http_version_minor = 9;
				result = true;
			}
			else if (iscntrl(ch))
				result = false;
			else
				m_uri += ch;
			break;

		// we're parsing the trailing HTTP/1.x here
		case 2:
			if (ch == 'H')
				++m_state;
			else
				result = false;
			break;
		case 3:
			if (ch == 'T')
				++m_state;
			else
				result = false;
			break;
		case 4:
			if (ch == 'T')
				++m_state;
			else
				result = false;
			break;
		case 5:
			if (ch == 'P')
				++m_state;
			else
				result = false;
			break;
		case 6:
			if (ch == '/')
				++m_state;
			else
				result = false;
			break;
		case 7:
			if (ch == '1')
				++m_state;
			else
				result = false;
			break;
		case 8:
			if (ch == '.')
				++m_state;
			else if (ch == '\r')
				m_state = 11;
			else
				result = false;
			break;
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
		case 10:
			if (ch == '\r')
				++m_state;
			else
				result = false;
			break;
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

parse_result reply_parser::parse(std::streambuf &text)
{
	if (m_parser == NULL)
	{
		m_parser = static_cast<state_parser>(&reply_parser::parse_initial_line);
		m_parsing_content = false;
	}

	parse_result result = indeterminate;

	bool is_parsing_content = m_parsing_content;
	while (text.in_avail() and result == indeterminate)
	{
		result = (this->*m_parser)(static_cast<char>(text.sbumpc()));

		if (result and is_parsing_content == false and m_parsing_content == true)
		{
			is_parsing_content = true;
			result = indeterminate;
		}
	}

	return result;
}

reply reply_parser::get_reply()
{
	return { static_cast<status_type>(m_status), { m_http_version_major, m_http_version_minor }, std::move(m_headers), std::move(m_payload) };
}

parse_result reply_parser::parse_initial_line(char ch)
{
	parse_result result = indeterminate;

	// a state machine to parse the initial reply line
	// which consists of:
	// HTTP/1.{0,1} XXX status-message

	switch (m_state)
	{
		// we're parsing the initial HTTP/1.x here
		case 0:
			if (ch == 'H')
				++m_state;
			else
				result = false;
			break;
		case 1:
			if (ch == 'T')
				++m_state;
			else
				result = false;
			break;
		case 2:
			if (ch == 'T')
				++m_state;
			else
				result = false;
			break;
		case 3:
			if (ch == 'P')
				++m_state;
			else
				result = false;
			break;
		case 4:
			if (ch == '/')
				++m_state;
			else
				result = false;
			break;
		case 5:
			if (ch == '1')
				++m_state;
			else
				result = false;
			break;
		case 6:
			if (ch == '.')
				++m_state;
			else if (ch == '\r')
				m_state = 11;
			else
				result = false;
			break;
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

		case 8:
			if (isspace(ch))
				++m_state;
			else
				result = false;
			break;

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

		case 12:
			if (isspace(ch))
				++m_state;
			else
				result = false;
			break;

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

} // namespace zeep::http
