//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// This file contains definitions of various utility routines

#include <zeep/config.hpp>

#include <filesystem>

namespace zeep::http
{

/// \brief compare an fs::path with a glob pattern
///
/// Returns true if the path \a p matches \a pattern
/// Matching is done using shell like glob patterns:
///
/// construct     | Matches
/// --------------|--------
/// ?             | single character
/// *             | zero or multiple characters
/// {a,b}         | matching either pattern a or b
///
/// \param p			The path to match
/// \param pattern		The pattern to match against
/// \return				True in case of a match
bool glob_match(const std::filesystem::path& p, std::string pattern);

}