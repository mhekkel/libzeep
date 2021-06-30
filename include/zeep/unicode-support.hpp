// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// various definitions of data types and routines used to work with Unicode encoded text

#include <zeep/config.hpp>
#include <zeep/exception.hpp>

#include <string>
#include <tuple>

namespace zeep
{

/// \brief typedef of our own unicode type
/// 
/// We use our own unicode type since wchar_t might be too small.
/// This type should be able to contain a UCS4 encoded character.
using unicode = char32_t;

/// \brief the (admittedly limited) set of supported text encodings in libzeep
/// 
/// these are the supported encodings. Perhaps we should extend this list a bit?
enum class encoding_type
{
	ASCII,			///< 7-bit ascii 
	UTF8,			///< UTF-8
	UTF16BE,		///< UTF-16 Big Endian
	UTF16LE,		///< UTF 16 Little Endian
	ISO88591		///< Default single byte encoding, is a subset of utf-8
};

/// \brief utf-8 is not single byte e.g.
constexpr bool is_single_byte_encoding(encoding_type enc)
{
	return enc == encoding_type::ASCII or enc == encoding_type::ISO88591 or enc == encoding_type::UTF8;
}

/// Convert a string from UCS4 to UTF-8
std::string wstring_to_string(const std::wstring& s);

/// manipulate UTF-8 encoded strings
void append(std::string& s, unicode ch);
unicode pop_last_char(std::string& s);
template<typename Iter>
std::tuple<unicode,Iter> get_first_char(Iter ptr);

/// \brief our own implementation of iequals: compares \a a with \a b case-insensitive
///
/// This is a limited use function, works only reliably with ASCII. But that's OK.
inline bool iequals(const std::string& a, const std::string& b)
{
	bool equal = a.length() == b.length();

	for (std::string::size_type i = 0; equal and i < a.length(); ++i)
		equal = std::toupper(a[i]) == std::toupper(b[i]);

	return equal;
}

// inlines

/// \brief Append a single unicode character to an utf-8 string
inline void append(std::string& s, unicode uc)
{
	if (uc < 0x080)
		s += (static_cast<char>(uc));
	else if (uc < 0x0800)
	{
		char ch[2] = {
			static_cast<char>(0x0c0 | (uc >> 6)),
			static_cast<char>(0x080 | (uc & 0x3f))
		};
		s.append(ch, 2);
	}
	else if (uc < 0x00010000)
	{
		char ch[3] = {
			static_cast<char>(0x0e0 | (uc >> 12)),
			static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
			static_cast<char>(0x080 | (uc & 0x3f))
		};
		s.append(ch, 3);
	}
	else
	{
		char ch[4] = {
			static_cast<char>(0x0f0 | (uc >> 18)),
			static_cast<char>(0x080 | ((uc >> 12) & 0x3f)),
			static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
			static_cast<char>(0x080 | (uc & 0x3f))
		};
		s.append(ch, 4);
	}
}

/// \brief remove the last unicode character from an utf-8 string
inline unicode pop_last_char(std::string& s)
{
	unicode result = 0;

	if (not s.empty())
	{
		std::string::iterator ch = s.end() - 1;
		
		if ((*ch & 0x0080) == 0)
		{
			result = *ch;
			s.erase(ch);
		}
		else
		{
			int o = 0;
			
			do
			{
				result |= (*ch & 0x03F) << o;
				o += 6;
				--ch;
			}
			while (ch != s.begin() and (*ch & 0x0C0) == 0x080);
			
			switch (o)
			{
				case  6: result |= (*ch & 0x01F) <<  6; break;
				case 12: result |= (*ch & 0x00F) << 12; break;
				case 18: result |= (*ch & 0x007) << 18; break;
			}
			
			s.erase(ch, s.end());
		}
	}
	
	return result;
}

// I used to have this comment here:
//
//    this code only works if the input is valid utf-8
//
// That was a bad idea....
//
/// \brief return the first unicode and the advanced pointer from a string
template<typename Iter>
std::tuple<unicode,Iter> get_first_char(Iter ptr)
{
	unicode result = static_cast<unsigned char>(*ptr);
	++ptr;

	if (result > 0x07f)
	{
		unsigned char ch[3];
		
		if ((result & 0x0E0) == 0x0C0)
		{
			ch[0] = static_cast<unsigned char>(*ptr); ++ptr;

			if ((ch[0] & 0x0c0) != 0x080)
				throw zeep::exception("Invalid utf-8");

			result = ((result & 0x01F) << 6) | (ch[0] & 0x03F);
		}
		else if ((result & 0x0F0) == 0x0E0)
		{
			ch[0] = static_cast<unsigned char>(*ptr); ++ptr;
			ch[1] = static_cast<unsigned char>(*ptr); ++ptr;

			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080)
				throw zeep::exception("Invalid utf-8");

			result = ((result & 0x00F) << 12) | ((ch[0] & 0x03F) << 6) | (ch[1] & 0x03F);
		}
		else if ((result & 0x0F8) == 0x0F0)
		{
			ch[0] = static_cast<unsigned char>(*ptr); ++ptr;
			ch[1] = static_cast<unsigned char>(*ptr); ++ptr;
			ch[2] = static_cast<unsigned char>(*ptr); ++ptr;

			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
				throw zeep::exception("Invalid utf-8");

			result = ((result & 0x007) << 18) | ((ch[0] & 0x03F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
		}
	}

	return std::make_tuple(result, ptr);
}

// --------------------------------------------------------------------

inline std::string to_hex(uint32_t i)
{
	char s[sizeof(i) * 2 + 3];
	char* p = s + sizeof(s);
	*--p = 0;

	const char kHexChars[] = "0123456789abcdef";

	while (i)
	{
		*--p = kHexChars[i & 0x0F];
		i >>= 4;
	}

	*--p = 'x';
	*--p = '0';

	return p;
}

// --------------------------------------------------------------------

/// \brief A simple implementation of trim, removing white space from start and end of \a s
inline void trim(std::string& s)
{
	std::string::iterator b = s.begin();
	while (b != s.end() and std::isspace(*b))
		++b;
	
	std::string::iterator e = s.end();
	while (e > b and std::isspace(*(e - 1)))
		--e;
	
	if (b != s.begin() or e != s.end())
		s = { b, e };
}

} // namespace xml
