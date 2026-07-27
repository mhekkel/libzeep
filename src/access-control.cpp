// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2025
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/access-control.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/unicode-support.hpp"

#include <string>
#include <vector>

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