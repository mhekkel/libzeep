//           Copyright Maarten L. Hekkelman, 2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/access-control.hpp>

namespace zeep::http
{

void access_control::get_access_control_headers(reply &rep) const
{
	if (not m_allow_origin.empty())
		rep.set_header("Access-Control-Allow-Origin", m_allow_origin);
	if (m_allow_credentials)
		rep.set_header("Access-Control-Allow-Credentials", "true");
	if (not m_allowed_headers.empty())
		rep.set_header("Access-Control-Allow-Headers", join(m_allowed_headers, ","));
}

} // namespace zeep::http