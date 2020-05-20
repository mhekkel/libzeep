// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

/// \file
/// The definition of the JSON parser in libzeep

#include <zeep/json/element.hpp>

namespace zeep::json
{

void parse_json(const std::string& json, json::element& object);
void parse_json(std::istream& os, json::element& object);

namespace literals
{
zeep::json::element operator""_json(const char* s, size_t len);
}

} // namespace zeep::json
