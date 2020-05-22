// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/controller.hpp>

namespace zeep::http
{

controller::controller(const std::string& prefix_path)
	: m_prefix_path(prefix_path)
{
	// strip leading slashes
	while (m_prefix_path[0] == '/')
		m_prefix_path.erase(m_prefix_path.begin());
}

controller::~controller()
{
}

bool controller::handle_request(request& req, reply& rep)
{
	return false;
}

bool controller::path_matches_prefix(const std::string& path) const
{
	bool result = m_prefix_path.empty();
	
	if (not result)
	{
		int offset = 0;
		while (path[offset] == '/')
			++offset;
		result = path.compare(offset, m_prefix_path.length(), m_prefix_path) == 0;
	}

	return result;
}

}