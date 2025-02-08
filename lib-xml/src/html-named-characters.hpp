//        Copyright Maarten L. Hekkelman, 2023-2025
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <zeep/xml/doctype.hpp>

namespace zeep::xml
{

const doctype::general_entity *get_named_character(const char *name);

}

