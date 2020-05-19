// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <locale>
#include <sstream>

#include <zeep/unicode-support.hpp>

namespace zeep
{

bool iequals(const std::string& a, const std::string& b)
{
	auto loc = std::locale();

	bool equal = a.length() == b.length();
	for (std::string::size_type i = 0; equal and i < a.length(); ++i)
		equal = std::toupper<char>(a[i], loc) == std::toupper<char>(b[i], loc);
	
	return equal;
}

std::string to_hex(int i)
{
	std::ostringstream s;
	s << std::hex << "0x" << i;
	return s.str();
}

// --------------------------------------------------------------------
// decode_url function

std::string decode_url(const std::string& s)
{
	std::string result;
	
	for (std::string::const_iterator c = s.begin(); c != s.end(); ++c)
	{
		if (*c == '%')
		{
			if (s.end() - c >= 3)
			{
				int value;
				std::string s2(c + 1, c + 3);
				std::istringstream is(s2);
				if (is >> std::hex >> value)
				{
					result += static_cast<char>(value);
					c += 2;
				}
			}
		}
		else if (*c == '+')
			result += ' ';
		else
			result += *c;
	}
	return result;
}

// --------------------------------------------------------------------
// encode_url function

std::string encode_url(const std::string& s)
{
	const unsigned char kURLAcceptable[96] =
	{/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	    0,0,0,0,0,0,0,0,0,0,7,6,0,7,7,4,		/* 2x   !"#$%&'()*+,-./	 */
	    7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,		/* 3x  0123456789:;<=>?	 */
	    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 4x  @ABCDEFGHIJKLMNO  */
	    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,		/* 5X  PQRSTUVWXYZ[\]^_	 */
	    0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 6x  `abcdefghijklmno	 */
	    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0			/* 7X  pqrstuvwxyz{\}~	DEL */
	};

	const char kHex[] = "0123456789abcdef";

	std::string result;
	
	for (std::string::const_iterator c = s.begin(); c != s.end(); ++c)
	{
		unsigned char a = (unsigned char)*c;
		if (not (a >= 32 and a < 128 and (kURLAcceptable[a - 32] & 4)))
		{
			result += '%';
			result += kHex[a >> 4];
			result += kHex[a & 15];
		}
		else
			result += *c;
	}

	return result;
}

}
