//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <locale>
#include <filesystem>

namespace zeep::http
{

std::string FormatDecimal(double d, int integerDigits, int decimalDigits, std::locale loc);

// this might be useful outside, but we leave it here for now
/// \brief compare an fs::path with a glob pattern
bool glob_match(const std::filesystem::path& p, std::string pattern);

}
