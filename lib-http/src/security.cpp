// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include <zeep/crypto.hpp>
#include <zeep/http/security.hpp>
#include <zeep/json/parser.hpp>

#include "glob.hpp"

namespace pt = boost::posix_time;

namespace zeep::http
{

namespace
{
#define BASE64URL "(?:[-_A-Za-z0-9]{4})*(?:[-_A-Za-z0-9]{2,3})?"
std::regex kJWTRx("^(" BASE64URL R"()\.()" BASE64URL R"()\.()" BASE64URL ")$" );
}

void security_context::validate_request(const request& req) const
{
	bool allow = m_default_allow;

	for (;;)
	{
		std::string path = req.get_path();

		// first check if this page is allowed without any credentials
		// that means, the first rule that matches this uri should allow
		// access.
		for (auto& rule: m_rules)
		{
			if (not glob_match(path, rule.m_pattern))
				continue;
			
			allow = rule.m_roles.empty();
			break;
		}

		if (allow)
			break;

		std::set<std::string> roles;

		auto access_token = req.get_cookie("access_token");
		if (not access_token.empty())
		{
			std::smatch m;
			if (not std::regex_match(access_token, m, kJWTRx))
				break;
			
			json::element JOSEHeader;
			json::parse_json(decode_base64url(m[1].str()), JOSEHeader);

			const json::element kJOSEHeader{ { "typ", "JWT" }, { "alg", "HS256" } };

			if (JOSEHeader != kJOSEHeader)
				break;

			// check signature
			auto sig = encode_base64url(hmac_sha256(m[1].str() + '.' + m[2].str(), m_secret));
			if (sig != m[3].str())
				break;

			json::element credentials;
			json::parse_json(decode_base64url(m[2].str()), credentials);

			if (not credentials.is_object() or not credentials["role"].is_array())
				break;

			for (auto role: credentials["role"])
				roles.insert(role.as<std::string>());
		}

		for (auto& rule: m_rules)
		{
			if (not glob_match(path, rule.m_pattern))
				continue;
			
			if (rule.m_roles.empty())
				allow = true;
			else
			{
				std::set<std::string> common;
				std::set_intersection(roles.begin(), roles.end(), rule.m_roles.begin(), rule.m_roles.end(), std::inserter(common, common.begin()));

				allow = not common.empty();
			}
			break;
		}

		break;
	}

	if (not allow)
		throw unauthorized_exception("no access");
}

// --------------------------------------------------------------------

json::element security_context::get_credentials(const request& req) const
{
	json::element result;

	for (;;)
	{
		auto access_token = req.get_cookie("access_token");
		if (access_token.empty())
			break;

		std::smatch m;
		if (not std::regex_match(access_token, m, kJWTRx))
			break;
			
		json::element JOSEHeader;
		json::parse_json(decode_base64url(m[1].str()), JOSEHeader);

		const json::element kJOSEHeader{ { "typ", "JWT" }, { "alg", "HS256" } };

		if (JOSEHeader != kJOSEHeader)
			break;

		// check signature
		auto sig = encode_base64url(hmac_sha256(m[1].str() + '.' + m[2].str(), m_secret));
		if (sig != m[3].str())
			break;

		json::element credentials;
		json::parse_json(decode_base64url(m[2].str()), result);
		break;
	}

	return result;
}

// --------------------------------------------------------------------

void security_context::add_authorization_headers(reply &rep, const user_details user)
{
	using namespace json::literals;

	auto JOSEHeader = R"({
		"typ": "JWT",
		"alg": "HS256"
	})"_json;

	json::element credentials{
		{ "username", user.username }
	};

	for (auto& role: user.roles)
		credentials["role"].push_back(role);

	auto h1 = encode_base64url(JOSEHeader.as<std::string>());
	auto h2 = encode_base64url(credentials.as<std::string>());
	auto h3 = encode_base64url(hmac_sha256(h1 + '.' + h2, m_secret));

	rep.set_cookie("access_token", h1 + '.' + h2 + '.' + h3, {
		{ "HttpOnly", "" },
		{ "SameSite", "Lax" }
	});
}

// --------------------------------------------------------------------

void security_context::verify_username_password(const std::string& username, const std::string& raw_password, reply &rep)
{
	try
	{
		auto user = m_users.load_user(username);
		
		bool match = false;
		for (auto&& [name, pwenc]: m_known_password_encoders)
		{
			if (user.password.compare(0, name.length(), name) != 0)
				continue;
			
			match = pwenc->matches(raw_password, user.password);
			break;
		}

		if (not match)
			throw invalid_password_exception();

		add_authorization_headers(rep, user);
	}
	catch (const std::exception& ex)
	{
		throw invalid_password_exception();
	}
}

// --------------------------------------------------------------------

std::pair<std::string,bool> security_context::get_csrf_token(request& req)
{
	// See if we need to add a new csrf token
	bool csrf_is_new = false;
	std::string csrf = req.get_cookie("csrf-token");
	if (csrf.empty())
	{
		csrf_is_new = true;
		csrf = encode_base64url(random_hash());
		req.set_cookie("csrf-token", csrf);
	}
	return { csrf, csrf_is_new };
}

}