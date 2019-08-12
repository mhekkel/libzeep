//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <locale>

namespace zeep
{
namespace el
{

std::string FormatDecimal(double d, int integerDigits, int decimalDigits, std::locale loc);

}
}
