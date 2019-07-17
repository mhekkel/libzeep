// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

#include <zeep/el/element.hpp>

namespace zeep
{
namespace el
{

void parse_json(const std::string& json, el::element& object);
void parse_json(std::istream& os, el::element& object);

} // namespace el
} // namespace zeep

zeep::el::element operator""_json(const char* s, size_t len);
