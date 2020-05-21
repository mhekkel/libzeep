//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// This file contains definitions of various utility routines

#include <zeep/config.hpp>

#include <cassert>

#include <string>
#include <locale>
#include <filesystem>

#include <zeep/unicode-support.hpp>

namespace zeep
{

/// \brief A locale dependent formatting of a decimal number
///
/// Returns a formatted number with the specified number of digits
/// using separators taken from std::locale \a loc
std::string FormatDecimal(double d, int integerDigits, int decimalDigits, std::locale loc);

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

// --------------------------------------------------------------------

/// \brief Simple class to save the state of a variable in a scope
template <typename T>
struct value_saver
{
	T& m_ref;
	T m_value;

	value_saver(T& value, const T& new_value) : m_ref(value), m_value(value) { m_ref = new_value; }
	~value_saver() { m_ref = m_value; }
};

/// \brief Simple class used as a replacement for std::stack
///
/// The overhead of the full blown std::stack is a bit too much sometimes
class mini_stack
{
  public:
	mini_stack() : m_ix(-1) {}

	unicode top()
	{
		assert(m_ix >= 0 and m_ix < int(sizeof(m_data) / sizeof(unicode)));
		return m_data[m_ix];
	}

	void pop()
	{
		--m_ix;
	}

	void push(unicode uc)
	{
		++m_ix;
		assert(m_ix < int(sizeof(m_data) / sizeof(unicode)));
		m_data[m_ix] = uc;
	}

	bool empty() const
	{
		return m_ix == -1;
	}

  private:
	unicode m_data[2];
	int m_ix;
};

bool is_absolute_path(const std::string& s);

}