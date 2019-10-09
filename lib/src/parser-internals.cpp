//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <experimental/type_traits>

#include <zeep/el/element.hpp>

#include "parser-internals.hpp"

namespace zeep
{
namespace detail
{

bool is_absolute_path(const std::string& s)
{
	bool result = false;

	if (not s.empty())
	{
		if (s[0] == '/')
			result = true;
		else if (isalpha(s[0]))
		{
			std::string::const_iterator ch = s.begin() + 1;
			while (ch != s.end() and isalpha(*ch))
				++ch;
			result = ch != s.end() and *ch == ':';
		}
	}

	return result;
}

} // namespace detail
}