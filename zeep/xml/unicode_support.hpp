//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ZEEP_XML_UNICODE_SUPPORT_HPP
#define ZEEP_XML_UNICODE_SUPPORT_HPP

#include <string>

namespace zeep { namespace xml {

bool is_name_start_char(wchar_t uc);
bool is_name_char(wchar_t uc);
bool is_char(wchar_t uc);
bool is_valid_system_literal_char(wchar_t uc);
bool is_valid_system_literal(const std::wstring& s);
bool is_valid_public_id_char(wchar_t uc);
bool is_valid_public_id(const std::wstring& s);
std::string wstring_to_string(const std::wstring& s);

}
}

#endif
