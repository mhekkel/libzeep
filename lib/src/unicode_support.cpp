// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <locale>
#include <sstream>

#include <zeep/xml/unicode_support.hpp>

namespace zeep { namespace xml {

// some very basic code to check the class of scanned characters

bool is_name_start_char(unicode uc)
{
	return
		uc == L':' or
		(uc >= L'A' and uc <= L'Z') or
		uc == L'_' or
		(uc >= L'a' and uc <= L'z') or
		(uc >= 0x0C0 and uc <= 0x0D6) or
		(uc >= 0x0D8 and uc <= 0x0F6) or
		(uc >= 0x0F8 and uc <= 0x02FF) or
		(uc >= 0x0370 and uc <= 0x037D) or
		(uc >= 0x037F and uc <= 0x01FFF) or
		(uc >= 0x0200C and uc <= 0x0200D) or
		(uc >= 0x02070 and uc <= 0x0218F) or
		(uc >= 0x02C00 and uc <= 0x02FEF) or
		(uc >= 0x03001 and uc <= 0x0D7FF) or
		(uc >= 0x0F900 and uc <= 0x0FDCF) or
		(uc >= 0x0FDF0 and uc <= 0x0FFFD) or
		(uc >= 0x010000 and uc <= 0x0EFFFF);	
}

bool is_name_char(unicode uc)
{
	return
		uc == '-' or
		uc == '.' or
		(uc >= '0' and uc <= '9') or
		uc == 0x0B7 or
		is_name_start_char(uc) or
		(uc >= 0x00300 and uc <= 0x0036F) or
		(uc >= 0x0203F and uc <= 0x02040);
}

bool is_valid_system_literal_char(unicode uc)
{
	return
		uc > 0x1f and
		uc != ' ' and
		uc != '<' and
		uc != '>' and
		uc != '"' and
		uc != '#';
}

bool is_valid_system_literal(const std::string& s)
{
	bool result = true;
	for (std::string::const_iterator ch = s.begin(); result == true and ch != s.end(); ++ch)
		result = is_valid_system_literal_char(*ch);
	return result;
}

bool is_valid_public_id_char(unicode uc)
{
	static const std::string kPubChars(" \r\n-'()+,./:=?;!*#@$_%");
	
	return
		(uc >= 'a' and uc <= 'z') or
		(uc >= 'A' and uc <= 'Z') or
		(uc >= '0' and uc <= '9') or
		(uc < 128 and kPubChars.find(static_cast<char>(uc)) != std::string::npos);
}

bool is_valid_public_id(const std::string& s)
{
	bool result = true;
	for (std::string::const_iterator ch = s.begin(); result == true and ch != s.end(); ++ch)
		result = is_valid_public_id_char(*ch);
	return result;
}

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

}
}
