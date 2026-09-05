// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2019
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// This file contains definitions of various utility routines

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include "zeep/uri.hpp"

# include <filesystem>
# include <string>
#endif

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
bool glob_match(const std::filesystem::path &p, std::string pattern);

/// \brief compare the path part of a uri with a glob pattern
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
/// \param u			The uri whose path to match
/// \param pattern		The pattern to match against
/// \return				True in case of a match
bool glob_match(const uri &u, std::string pattern);

} // namespace zeep::http