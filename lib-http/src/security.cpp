// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

// #include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include <zeep/crypto.hpp>
#include <zeep/utils.hpp>
#include <zeep/http/security.hpp>
#include <zeep/json/parser.hpp>

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
		json::parse_json(decode_base64url(m[2].str()), credentials);

		if (not credentials.is_object() or not credentials["role"].is_object())
			break;

		std::set<std::string> roles;
		for (auto role: credentials["role"])
			roles.insert(role.as<std::string>());

		std::string path = req.get_path();

		for (auto& rule: m_rules)
		{
			if (not glob_match(path, rule.m_pattern))
				continue;
			
			std::set<std::string> common;
			std::set_intersection(roles.begin(), roles.end(), rule.m_roles.begin(), rule.m_roles.end(), std::inserter(common, common.begin()));

			allow = not common.empty();
			break;
		}

		break;
	}

	if (not allow)
		throw unauthorized_exception("no access");
}

// --------------------------------------------------------------------
//

json::element authentication_validation_base::validate_username_password(const std::string &username, const std::string &password)
{
	return {};
}

// --------------------------------------------------------------------

json::element jws_authentication_validation_base::validate_authentication(const request& req)
{
	json::element result;

	for (;;)
	{
		auto access_token = req.get_cookie("access_token");
		if (access_token.empty())
			break;

#define BASE64URL "(?:[-_A-Za-z0-9]{4})*(?:[-_A-Za-z0-9]{2,3})?"

		std::regex rx("^(" BASE64URL R"()\.()" BASE64URL R"()\.()" BASE64URL ")$" );

		std::smatch m;
		if (not std::regex_match(access_token, m, rx))
			break;
		
		json::element JOSEHeader;
		json::parse_json(decode_base64url(m[1].str()), JOSEHeader);

		const json::element kJOSEHeader{ { "typ", "JWT" }, { "alg", "HS256" } };

		if (JOSEHeader != kJOSEHeader)
			break;

		// check sig

		auto sig = encode_base64url(hmac_sha256(m[1].str() + '.' + m[2].str(), m_secret));
		if (sig != m[3].str())
			break;

		json::parse_json(decode_base64url(m[2].str()), result);
		break;
	}

	return result;
}

void jws_authentication_validation_base::add_authorization_headers(reply& rep, const json::element& credentials)
{
	using namespace json::literals;

	auto JOSEHeader = R"({
		"typ": "JWT",
		"alg": "HS256"
	})"_json;

	auto h1 = encode_base64url(JOSEHeader.as<std::string>());
	auto h2 = encode_base64url(credentials.as<std::string>());
	auto h3 = encode_base64url(hmac_sha256(h1 + '.' + h2, m_secret));

	rep.set_cookie("access_token", h1 + '.' + h2 + '.' + h3, {
		{ "HttpOnly", "" },
		{ "SameSite", "Lax" }
	});
}

// --------------------------------------------------------------------

simple_jws_authentication_validation::simple_jws_authentication_validation(const std::string& realm, const std::string& secret,
	std::initializer_list<user_password_pair> validUsers)
	: jws_authentication_validation_base(realm, secret)
{
	for (auto& user: validUsers)
		m_user_hashes.emplace(std::make_pair(user.username, user.password));
}

json::element simple_jws_authentication_validation::validate_username_password(const std::string& username, const std::string& password)
{
	json::element result;

	auto h = m_user_hashes.find(username);
	if (h != m_user_hashes.end())
		result["name"] = username;

	return result;
}

}