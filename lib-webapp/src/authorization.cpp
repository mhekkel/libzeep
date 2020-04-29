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
#include <zeep/http/authorization.hpp>
#include <zeep/el/parser.hpp>

namespace pt = boost::posix_time;

namespace zeep::http
{

// --------------------------------------------------------------------
//

el::element authentication_validation_base::validate_username_password(const std::string &username, const std::string &password)
{
	return {};
}

// --------------------------------------------------------------------

struct auth_info
{
	auth_info(const std::string& realm);

	bool validate(method_type method, const std::string& uri, const std::string& ha1, std::map<std::string, std::string>& info);

	std::string get_challenge() const;
	bool stale() const;

	std::string m_nonce, m_realm;
	std::set<uint32_t> m_replay_check;
	pt::ptime m_created;
};

auth_info::auth_info(const std::string& realm)
	: m_realm(realm)
{
	using namespace boost::gregorian;
	m_nonce = encode_hex(random_hash());
	m_created = pt::second_clock::local_time();
}

std::string auth_info::get_challenge() const
{
	std::string challenge = "Digest ";
	challenge += "realm=\"" + m_realm + "\", qop=\"auth\", nonce=\"" + m_nonce + '"';
	return challenge;
}

bool auth_info::stale() const
{
	pt::time_duration age = pt::second_clock::local_time() - m_created;
	return age.total_seconds() > 1800;
}

bool auth_info::validate(method_type method, const std::string& uri, const std::string& ha1, std::map<std::string, std::string>& info)
{
	bool valid = false;

	uint32_t nc = strtol(info["nc"].c_str(), nullptr, 16);
	if (not m_replay_check.count(nc))
	{
		std::string ha2 = encode_hex(md5(to_string(method) + std::string{':'} + info["uri"]));

		std::string response = encode_hex(md5(
								   ha1 + ':' +
								   info["nonce"] + ':' +
								   info["nc"] + ':' +
								   info["cnonce"] + ':' +
								   info["qop"] + ':' +
								   ha2));

		valid = info["response"] == response;

		// keep a list of seen nc-values in m_replay_check
		m_replay_check.insert(nc);
	}
	return valid;
}

// --------------------------------------------------------------------
//

digest_authentication_validation::digest_authentication_validation(const std::string& realm)
	: authentication_validation_base(realm)
{
}

digest_authentication_validation::~digest_authentication_validation()
{
}

void digest_authentication_validation::add_challenge_headers(reply& rep, bool stale)
{
	std::unique_lock<std::mutex> lock(m_auth_mutex);

	// create_error_reply(req, unauthorized, get_status_text(unauthorized), rep);

	m_auth_info.push_back(auth_info(m_realm));

	std::string challenge = m_auth_info.back().get_challenge();
	if (stale)
		challenge += ", stale=\"true\"";

	rep.set_header("WWW-Authenticate", challenge);
}

el::element digest_authentication_validation::validate_authentication(const request& req)
{
	std::string authorization = req.get_header("Authorization");

	if (authorization.empty())
		throw unauthorized_exception(m_realm);

	// That was easy, now check the response

	std::map<std::string, std::string> info;

	// Ahhh... std::regex does not support (?| | )
	// std::regex re(R"rx((\w+)=(?|"([^"]*)"|'([^']*)'|(\w+))(?:,\s*)?)rx");
	std::regex re(R"rx((\w+)=("[^"]*"|'[^']*'|\w+)(?:,\s*)?)rx");

	const char *b = authorization.c_str();
	const char *e = b + authorization.length();
	std::match_results<const char *> m;
	while (b < e and std::regex_search(b, e, m, re))
	{
		auto p = m[2].str();
		if (p.length() > 1 and ((p.front() == '"' and p.back() == '"') or (p.front() == '\'' and p.back() == '\'')))
			p = p.substr(1, p.length() - 2);

		info[m[1].str()] = p;
		b = m[0].second;
	}

	if (info["realm"] != m_realm)
		throw unauthorized_exception(m_realm);

	std::string ha1 = get_hashed_password(info["username"]);

	// lock to avoid accessing m_auth_info from multiple threads at once
	std::unique_lock<std::mutex> lock(m_auth_mutex);
	bool authorized = false;

	for (auto auth = m_auth_info.begin(); auth != m_auth_info.end(); ++auth)
	{
		if (auth->m_realm == m_realm and
			auth->m_nonce == info["nonce"] and
			auth->validate(req.method, req.uri, ha1, info))
		{
			authorized = true;
			if (auth->stale())
			{
				m_auth_info.erase(auth);
				throw authorization_stale_exception(m_realm);
			}
			break;
		}
	}

	if (not authorized)
		throw unauthorized_exception(m_realm);

	return info["username"];
}

// --------------------------------------------------------------------

simple_digest_authentication_validation::simple_digest_authentication_validation(
	const std::string& realm, std::initializer_list<user_password_pair> validUsers)
	: digest_authentication_validation(realm)
{
	for (auto& user: validUsers)
		m_user_hashes.emplace(std::make_pair(user.username, encode_hex(md5(user.username + ':' + realm + ':' + user.password))));
}

std::string simple_digest_authentication_validation::get_hashed_password(const std::string& username)
{
	auto h = m_user_hashes.find(username);
	if (h == m_user_hashes.end())
		throw unauthorized_exception(m_realm);
	return h->second;
}

// --------------------------------------------------------------------

el::element jws_authentication_validation_base::validate_authentication(const request& req)
{
	el::element result;

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
		
		el::element JOSEHeader;
		el::parse_json(decode_base64url(m[1].str()), JOSEHeader);

		const el::element kJOSEHeader{ { "typ", "JWT" }, { "alg", "HS256" } };

		if (JOSEHeader != kJOSEHeader)
			break;

		// check sig

		auto sig = encode_base64url(hmac_sha256(m[1].str() + '.' + m[2].str(), m_secret));
		if (sig != m[3].str())
			break;

		el::parse_json(decode_base64url(m[2].str()), result);
		break;
	}

	return result;
}

void jws_authentication_validation_base::add_authorization_headers(reply& rep, const el::element& credentials)
{
	using namespace el::literals;

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

el::element simple_jws_authentication_validation::validate_username_password(const std::string& username, const std::string& password)
{
	el::element result;

	auto h = m_user_hashes.find(username);
	if (h != m_user_hashes.end())
		result["name"] = username;

	return result;
}

}