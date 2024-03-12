//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// This file contains definitions of various utility routines

#include <zeep/config.hpp>

#include <string>
#include <locale>

namespace zeep::http
{

/// \brief A locale dependent formatting of a decimal number
///
/// Returns a formatted number with the specified number of digits
/// using separators taken from std::locale \a loc
std::string FormatDecimal(double d, int integerDigits, int decimalDigits, std::locale loc);

}
