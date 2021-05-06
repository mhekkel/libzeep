//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <regex>

#include <boost/algorithm/string.hpp>

#include "glob.hpp"

namespace ba = boost::algorithm;

// --------------------------------------------------------------------

namespace zeep::http
{
namespace
{

bool Match(const char* pattern, const char* name)
{
	for (;;)
	{
		char op = *pattern;

		switch (op)
		{
			case 0:
				return *name == 0;
			case '*':
			{
				// separate shortcut
				if (pattern[1] == '*' and pattern[2] == '/' and Match(pattern + 3, name))
					return true;

				// ends with **
				if (pattern[1] == '*' and pattern[2] == 0)
					return true;
				
				// ends with *
				if (pattern[1] == 0)
				{
					while (*name)
					{
						if (*name == '/' or *name == '\\')
							return false;
						++name;
					}
					return true;
				}

				// contains **
				if (pattern[1] == '*')
				{
					while (*name)
					{
						if (Match(pattern + 2, name))
							return true;
						++name;
					}
					return false;
				}

				// contains just *
				while (*name)
				{
					if (Match(pattern + 1, name))
						return true;
					if (*name == '/')
						return false;
					++name;
				}

				return false;
			}
			case '?':
				if (*name)
					return Match(pattern + 1, name + 1);
				else
					return false;
			default:
				if ((*name == '/' and op == '\\') or
					(*name == '\\' and op == '/') or
					tolower(*name) == tolower(op))
				{
					++name;
					++pattern;
				}
				else
					return false;
				break;
		}
	}
}

void expand_group(const std::string& pattern, std::vector<std::string>& expanded)
{
	static std::regex rx(R"(\{([^{},]*,[^{}]*)\})");

	std::smatch m;
	if (std::regex_search(pattern, m, rx))
	{
		std::vector<std::string> options;

		std::string group = m[1].str();
		ba::split(options, group, ba::is_any_of(","));

		for (std::string& option : options)
			expand_group(m.prefix().str() + option + m.suffix().str(), expanded);
	}
	else
		expanded.push_back(pattern);
}

}

// --------------------------------------------------------------------

bool glob_match(const std::filesystem::path& path, std::string glob_pattern)
{
	bool result = path.empty() and glob_pattern.empty();

	if (not glob_pattern.empty() and glob_pattern.back() == '/')
		glob_pattern += "**";

	std::vector<std::string> patterns;
	ba::split(patterns, glob_pattern, ba::is_any_of(";"));

	std::vector<std::string> expandedpatterns;
	std::for_each(patterns.begin(), patterns.end(), [&expandedpatterns](std::string& pattern)
	{
		expand_group(pattern, expandedpatterns);
	});

	for (std::string& pat : expandedpatterns)
	{
		if ((path.empty() and pat.empty()) or
			Match(pat.c_str(), path.generic_string().c_str()))
		{
			result = true;
			break;
		}
	}

	return result;
}

}

