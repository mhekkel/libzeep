//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ZEEP_XML_UNICODE_SUPPORT_HPP
#define ZEEP_XML_UNICODE_SUPPORT_HPP

#include <string>

namespace zeep { namespace xml {

// the supported encodings. Perhaps we should extend this list a bit?
enum encoding_type
{
	enc_UTF8,
	enc_UTF16BE,
	enc_UTF16LE,
//	enc_ISO88591
};

// some character classification routines
bool is_name_start_char(wchar_t uc);
bool is_name_char(wchar_t uc);
bool is_char(wchar_t uc);
bool is_valid_system_literal_char(wchar_t uc);
bool is_valid_system_literal(const std::string& s);
bool is_valid_public_id_char(wchar_t uc);
bool is_valid_public_id(const std::string& s);

// Convert a string from UCS4 to UTF-8
std::string wstring_to_string(const std::wstring& s);

void append(std::string& s, wchar_t ch);
wchar_t pop_last_char(std::string& s);

// inlines

inline bool is_char(wchar_t uc)
{
	return
		uc == 0x09 or
		uc == 0x0A or
		uc == 0x0D or
		(uc >= 0x020 and uc <= 0x0D7FF) or
		(uc >= 0x0E000 and uc <= 0x0FFFD) or
		(uc >= 0x010000 and uc <= 0x010FFFF);
}

}
}

#endif
