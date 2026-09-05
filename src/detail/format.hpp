// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2019-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// This file contains definitions of various utility routines

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include <locale>
# include <string>
#endif

namespace zeep
{

/// \brief A locale dependent formatting of a decimal number
///
/// Returns a formatted number with the specified number of digits
/// using separators taken from std::locale \a loc
std::string format_decimal(double d, int integerDigits, int decimalDigits, const std::locale &loc);

} // namespace zeep
