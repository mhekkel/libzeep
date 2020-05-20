// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// routines for classifying characters in an XML context

#include <zeep/config.hpp>
#include <zeep/unicode-support.hpp>

#include <string>

namespace zeep::xml
{

/// some character classification routines

bool is_name_start_char(unicode uc);
bool is_name_char(unicode uc);
bool is_valid_xml_1_0_char(unicode uc);
bool is_valid_xml_1_1_char(unicode uc);
bool is_valid_system_literal_char(unicode uc);
bool is_valid_system_literal(const std::string& s);
bool is_valid_public_id_char(unicode uc);
bool is_valid_public_id(const std::string& s);

} // namespace zeep::xml
