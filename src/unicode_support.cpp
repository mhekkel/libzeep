//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>

#include <boost/bind.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "zeep/xml/unicode_support.hpp"

using namespace std;

namespace zeep { namespace xml {

// some very basic code to check the class of scanned characters

bool is_name_start_char(wchar_t uc)
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

bool is_name_char(wchar_t uc)
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

bool is_char(wchar_t uc)
{
	return
		uc == 0x09 or
		uc == 0x0A or
		uc == 0x0D or
		(uc >= 0x020 and uc <= 0x0D7FF) or
		(uc >= 0x0E000 and uc <= 0x0FFFD) or
		(uc >= 0x010000 and uc <= 0x010FFFF);
}

bool is_valid_system_literal_char(wchar_t uc)
{
	return
		not (uc >= 0x0 and uc <= 0x1f) and
		uc != ' ' and
		uc != '<' and
		uc != '>' and
		uc != '"' and
		uc != '#';
}

bool is_valid_system_literal(const wstring& s)
{
	bool result = true;
	for (wstring::const_iterator ch = s.begin(); result == true and ch != s.end(); ++ch)
		result = is_valid_system_literal_char(*ch);
	return result;
}

bool is_valid_public_id_char(wchar_t uc)
{
	return
		(uc >= 'a' and uc <= 'z') or
		(uc >= 'A' and uc <= 'Z') or
		(uc >= '0' and uc <= '9') or
		boost::is_any_of(L" \r\n-'()+,./:=?;!*#@$_%")(uc);
}

bool is_valid_public_id(const wstring& s)
{
	bool result = true;
	for (wstring::const_iterator ch = s.begin(); result == true and ch != s.end(); ++ch)
		result = is_valid_public_id_char(*ch);
	return result;
}

// wstring_to_string is a very simplistic UCS4 to UTF-8 converter
string wstring_to_string(const wstring& s)
{
	string result;
	result.reserve(s.length());
	
	for (wstring::const_iterator ch = s.begin(); ch != s.end(); ++ch)
	{
		unsigned long cv = static_cast<unsigned long>(*ch);
		
		if (cv < 0x080)
			result += (static_cast<const char> (cv));
		else if (cv < 0x0800)
		{
			result += (static_cast<const char> (0x0c0 | (cv >> 6)));
			result += (static_cast<const char> (0x080 | (cv & 0x3f)));
		}
		else if (cv < 0x00010000)
		{
			result += (static_cast<const char> (0x0e0 | (cv >> 12)));
			result += (static_cast<const char> (0x080 | ((cv >> 6) & 0x3f)));
			result += (static_cast<const char> (0x080 | (cv & 0x3f)));
		}
		else
		{
			result += (static_cast<const char> (0x0f0 | (cv >> 18)));
			result += (static_cast<const char> (0x080 | ((cv >> 12) & 0x3f)));
			result += (static_cast<const char> (0x080 | ((cv >> 6) & 0x3f)));
			result += (static_cast<const char> (0x080 | (cv & 0x3f)));
		}
	}
	
	return result;
}

}
}
