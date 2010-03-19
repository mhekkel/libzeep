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
bool is_valid_system_literal(const std::wstring& s);
bool is_valid_public_id_char(wchar_t uc);
bool is_valid_public_id(const std::wstring& s);

// Convert a string from UCS4 to UTF-8
std::string wstring_to_string(const std::wstring& s);

}
}

#endif
