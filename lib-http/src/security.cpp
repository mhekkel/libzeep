// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include "glob.hpp"

#include <zeep/crypto.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/uri.hpp>
#include <zeep/json/parser.hpp>

#include <iostream>

namespace zeep::http
{

namespace
{
#define BASE64URL "(?:[-_A-Za-z0-9]{4})*(?:[-_A-Za-z0-9]{2,3})?"
	std::regex kJWTRx("^(" BASE64URL R"()\.()" BASE64URL R"()\.()" BASE64URL ")$");
} // namespace

// --------------------------------------------------------------------

bool user_service::user_is_valid(const json::element &credentials) const
{
	return user_is_valid(credentials["username"].as<std::string>());
}

bool user_service::user_is_valid(const std::string &username) const
{
	bool result = false;

	try
	{
		auto user = load_user(username);
		result = user.username == username;
	}
	catch (...)
	{
	}

	return result;
}

// --------------------------------------------------------------------

security_context::security_context(const std::string &secret, user_service &users, bool defaultAccessAllowed)
	: m_secret(secret)
	, m_users(users)
	, m_default_allow(defaultAccessAllowed)
	, m_default_jwt_exp(date::years{1})
{
	register_password_encoder<pbkdf2_sha256_password_encoder>();
}

void security_context::validate_request(request &req) const
{
	bool allow = m_default_allow;

	for (;;)
	{
		auto path = req.get_uri();

		std::set<std::string> roles;

		auto access_token = req.get_cookie("access_token");
		for (;;)
		{
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
			json::parse_json(decode_base64url(m[2].str()), credentials);

			// check exp
			using namespace std::chrono;

			auto exp = credentials["exp"].as<int64_t>();
			auto exp_t = time_point<system_clock>() + seconds{exp};
			
			if (system_clock::now() > exp_t)
				break; // expired

			if (not credentials.is_object() or not credentials["role"].is_array())
				break;

			// make sure user still exists.
			if (not m_users.user_is_valid(credentials))
				break;

			for (auto role : credentials["role"])
				roles.insert(role.as<std::string>());

			req.set_credentials(std::move(credentials));

			break;
		}

		// first check if this page is allowed without any credentials
		// that means, the first rule that matches this uri should allow
		// access.
		for (auto &rule : m_rules)
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

	if (allow and m_validate_csrf and req.has_parameter("_csrf"))
	{
		auto req_csrf_param = req.get_parameter("_csrf");
		auto req_csrf_cookie = req.get_cookie("csrf-token");

		if (req_csrf_cookie != req_csrf_param)
		{
			allow = false;
			std::cerr << "CSRF validation failed" << std::endl;
		}
	}

	if (not allow)
		throw unauthorized_exception();
}

// --------------------------------------------------------------------

void security_context::add_authorization_headers(reply &rep, const user_details user,
	std::chrono::system_clock::duration exp)
{
	using namespace json::literals;

	using namespace date;
	using namespace std::chrono;

	auto JOSEHeader = R"({
		"typ": "JWT",
		"alg": "HS256"
	})"_json;

	auto exp_t = duration_cast<seconds>(system_clock::now() + exp - system_clock::time_point()).count();

	json::element credentials{
		{ "username", user.username },
		{ "exp", exp_t }
	};

	for (auto &role : user.roles)
		credentials["role"].push_back(role);

	auto h1 = encode_base64url(JOSEHeader.as<std::string>());
	auto h2 = encode_base64url(credentials.as<std::string>());
	auto h3 = encode_base64url(hmac_sha256(h1 + '.' + h2, m_secret));

	rep.set_cookie("access_token", h1 + '.' + h2 + '.' + h3, { { "HttpOnly", "" }, { "SameSite", "Lax" } });
}

void security_context::add_authorization_headers(reply &rep, const user_details user)
{
	using namespace date;
	using namespace std::chrono;

	add_authorization_headers(rep, user, m_default_jwt_exp);
}

// --------------------------------------------------------------------

bool security_context::verify_username_password(const std::string &username, const std::string &raw_password)
{
	bool result = false;

	auto user = m_users.load_user(username);

	for (auto const &[name, pwenc] : m_known_password_encoders)
	{
		if (user.password.compare(0, name.length(), name) != 0)
			continue;

		result = pwenc->matches(raw_password, user.password);
		break;
	}

	return result;
}

void security_context::verify_username_password(const std::string &username, const std::string &raw_password, reply &rep)
{
	if (not verify_username_password(username, raw_password))
		throw invalid_password_exception();

	add_authorization_headers(rep, m_users.load_user(username));
}

// --------------------------------------------------------------------

std::pair<std::string, bool> security_context::get_csrf_token(request &req)
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

} // namespace zeep::http