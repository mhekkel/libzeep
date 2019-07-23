//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <experimental/type_traits>

#include <zeep/el/element.hpp>

#include "parser-internals.hpp"

namespace zeep
{
namespace detail
{

bool is_absolute_path(const std::string& s)
{
	bool result = false;

	if (not s.empty())
	{
		if (s[0] == '/')
			result = true;
		else if (isalpha(s[0]))
		{
			std::string::const_iterator ch = s.begin() + 1;
			while (ch != s.end() and isalpha(*ch))
				++ch;
			result = ch != s.end() and *ch == ':';
		}
	}

	return result;
}

void istream_data_source::guess_encoding()
{
	// see if there is a BOM
	// if there isn't, we assume the data is UTF-8

	char ch1 = m_data.rdbuf()->sgetc();

	if (ch1 == char(0xfe))
	{
		char ch2 = m_data.rdbuf()->snextc();

		if (ch2 == char(0xff))
		{
			m_data.rdbuf()->snextc();
			m_encoding = encoding_type::enc_UTF16BE;
			m_has_bom = true;
		}
		else
			m_data.rdbuf()->sungetc();
	}
	else if (ch1 == char(0xff))
	{
		char ch2 = m_data.rdbuf()->snextc();

		if (ch2 == char(0xfe))
		{
			m_data.rdbuf()->snextc();
			m_encoding = encoding_type::enc_UTF16LE;
			m_has_bom = true;
		}
		else
			m_data.rdbuf()->sungetc();
	}
	else if (ch1 == char(0xef))
	{
		char ch2 = m_data.rdbuf()->snextc();
		char ch3 = m_data.rdbuf()->snextc();

		if (ch2 == char(0xbb) and ch3 == char(0xbf))
		{
			m_data.rdbuf()->snextc();
			m_encoding = encoding_type::enc_UTF8;
			m_has_bom = true;
		}
		else
		{
			m_data.rdbuf()->sungetc();
			m_data.rdbuf()->sputbackc(ch1);
		}
	}

	switch (m_encoding)
	{
        case encoding_type::enc_UTF8:
            m_next = &istream_data_source::next_utf8_char;
            break;
        case encoding_type::enc_UTF16LE:
            m_next = &istream_data_source::next_utf16le_char;
            break;
        case encoding_type::enc_UTF16BE:
            m_next = &istream_data_source::next_utf16be_char;
            break;
        case encoding_type::enc_ISO88591:
            m_next = &istream_data_source::next_iso88591_char;
            break;
        default: break;
	}
}

unicode istream_data_source::next_utf8_char()
{
	unicode result = next_byte();

	if (result & 0x080)
	{
		unsigned char ch[3];

		if ((result & 0x0E0) == 0x0C0)
		{
			ch[0] = next_byte();
			if ((ch[0] & 0x0c0) != 0x080)
				throw source_exception("Invalid utf-8");
			result = ((result & 0x01F) << 6) | (ch[0] & 0x03F);
		}
		else if ((result & 0x0F0) == 0x0E0)
		{
			ch[0] = next_byte();
			ch[1] = next_byte();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080)
				throw source_exception("Invalid utf-8");
			result = ((result & 0x00F) << 12) | ((ch[0] & 0x03F) << 6) | (ch[1] & 0x03F);
		}
		else if ((result & 0x0F8) == 0x0F0)
		{
			ch[0] = next_byte();
			ch[1] = next_byte();
			ch[2] = next_byte();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
				throw source_exception("Invalid utf-8");
			result = ((result & 0x007) << 18) | ((ch[0] & 0x03F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);

			if (result > 0x10ffff)
				throw source_exception("invalid utf-8 character (out of range)");
		}
	}

	return result;
}

unicode istream_data_source::next_utf16le_char()
{
	unsigned char c1 = next_byte(), c2 = next_byte();

	unicode result = (static_cast<unicode>(c2) << 8) | c1;

	return result;
}

unicode istream_data_source::next_utf16be_char()
{
	unsigned char c1 = next_byte(), c2 = next_byte();

	unicode result = (static_cast<unicode>(c1) << 8) | c2;

	return result;
}

unicode istream_data_source::next_iso88591_char()
{
	return (unicode)next_byte();
}

unicode istream_data_source::get_next_char()
{
	unicode ch = m_char_buffer;

	if (ch == 0)
		ch = (this->*m_next)();
	else
		m_char_buffer = 0;

	if (ch == '\r')
	{
		ch = (this->*m_next)();
		if (ch != '\n')
			m_char_buffer = ch;
		ch = '\n';
	}

	if (ch == '\n')
		++m_line_nr;

	return ch;
}


unicode string_data_source::get_next_char()
{
	unicode result = 0;

	if (m_ptr != m_data.end())
	{
		result = static_cast<unsigned char>(*m_ptr);
		++m_ptr;

		if (result > 0x07f)
		{
			unsigned char ch[3];

			if ((result & 0x0E0) == 0x0C0)
			{
				ch[0] = static_cast<unsigned char>(*m_ptr);
				++m_ptr;
				result = ((result & 0x01F) << 6) | (ch[0] & 0x03F);
			}
			else if ((result & 0x0F0) == 0x0E0)
			{
				ch[0] = static_cast<unsigned char>(*m_ptr);
				++m_ptr;
				ch[1] = static_cast<unsigned char>(*m_ptr);
				++m_ptr;
				result = ((result & 0x00F) << 12) | ((ch[0] & 0x03F) << 6) | (ch[1] & 0x03F);
			}
			else if ((result & 0x0F8) == 0x0F0)
			{
				ch[0] = static_cast<unsigned char>(*m_ptr);
				++m_ptr;
				ch[1] = static_cast<unsigned char>(*m_ptr);
				++m_ptr;
				ch[2] = static_cast<unsigned char>(*m_ptr);
				++m_ptr;
				result = ((result & 0x007) << 18) | ((ch[0] & 0x03F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
			}
		}
	}

	if (result == '\n')
		++m_line_nr;

	return result;
}

} // namespace detail
}