// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/security.hpp"
#include "detail/glob.hpp"
#include "zeep/crypto.hpp"
#include "zeep/el/object.hpp"
#include "zeep/el/processing.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/uri.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <utility>

#if __has_include(<sys/mman.h>)
# include <sys/mman.h>
#endif

namespace zeep::http
{

// --------------------------------------------------------------------

bool user_service::user_is_valid(const object &credentials) const
{
	return user_is_valid(credentials["username"].get<std::string>());
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
		result = false;
	}

	return result;
}

// --------------------------------------------------------------------

security_context::security_context(std::string secret, user_service &users, bool defaultAccessAllowed)
	: m_secret(std::move(secret))
	, m_users(users)
	, m_default_allow(defaultAccessAllowed)
	, m_default_jwt_exp(std::chrono::weeks{ 1 })
{
	// the moved-from source is still a copy of the secret; wipe it
	secure_scrub(secret);

#if __has_include(<sys/mman.h>)
	if (mlock(m_secret.data(), m_secret.length()) != 0)
		std::clog << "Warning: mlock failed, secret may be swapped to disk\n";
#endif

	register_password_encoder<pbkdf2_sha256_password_encoder>();
}

security_context::~security_context()
{
	// wipe out secret
	secure_scrub(m_secret);

#if __has_include(<sys/mman.h>)
	(void)munlock(m_secret.data(), m_secret.length());
#endif
}

void security_context::validate_request(request &req) const
{
	using namespace std::literals;

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

			// Split the JWT into parts
			std::vector<std::string> m;
			split(m, access_token, ".");
			if (m.size() != 3)
				break;

			auto JOSEHeader = object::parse_JSON(decode_base64url(m[0]));

			const object kJOSEHeader{ { "typ", "JWT" }, { "alg", "HS256" } };

			if (JOSEHeader != kJOSEHeader)
				break;

			// check signature
			auto sig = encode_base64url(hmac_sha256(m[0] + '.' + m[1], m_secret));

			// Compare strings in constant time
			if (not strings_match(sig, m[2]))
				break;

			auto credentials = object::parse_JSON(decode_base64url(m[1]));

			// check exp
			using namespace std::chrono;

			auto exp = credentials["exp"].get<int64_t>();
			auto exp_t = time_point<system_clock>() + seconds{ exp };

			if (system_clock::now() > exp_t)
				break; // expired

			if (not credentials.is_object() or not credentials["role"].is_array())
				break;

			// make sure user still exists.
			if (not m_users.user_is_valid(credentials))
				break;

			for (const auto &role : credentials["role"])
				roles.insert(role.get<std::string>());

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
				std::ranges::set_intersection(roles, rule.m_roles, std::inserter(common, common.begin()));

				allow = not common.empty();
			}
			break;
		}

		break;
	}

	if (allow and m_validate_csrf and is_mutating_method(req.get_method()))
	{
		auto p = req.get_parameter("_csrf");
		if (not p.has_value())
			p = req.get_header("X-CSRF-Token");

		if (auto req_csrf_cookie = req.get_cookie("csrf-token");
			not strings_match(req_csrf_cookie, *p))
		{
			allow = false;
			std::clog << "CSRF validation failed\n";
		}
	}

	if (not allow)
		throw unauthorized_exception();
}

// --------------------------------------------------------------------

void security_context::add_authorization_headers(reply &rep, const user_details &user,
	std::chrono::system_clock::duration exp)
{
	using namespace std::chrono;

	object JOSEHeader{
		{ "typ", "JWT" },
		{ "alg", "HS256" }
	};

	auto exp_t = duration_cast<seconds>(system_clock::now() + exp - system_clock::time_point()).count();

	object credentials{
		{ "username", user.username },
		{ "exp", exp_t }
	};

	for (auto &role : user.roles)
		credentials["role"].push_back(role);

	auto h1 = encode_base64url(JOSEHeader.get_JSON());
	auto h2 = encode_base64url(credentials.get_JSON());
	auto h3 = encode_base64url(hmac_sha256(h1 + '.' + h2, m_secret));

	auto when = floor<seconds>(system_clock::now() + exp);

	rep.set_cookie("access_token", h1 + '.' + h2 + '.' + h3,
		// clang-format off
		{
			{ "HttpOnly", "" },
#ifdef NDEBUG
			{ "Secure", "" },
#endif
			{ "Path", "/" },
			{ "SameSite", "Lax" },
			{ "Expires", std::format(R"({0:%a}, {0:%d} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} GMT)", when) }
		}
		// clang-format on
	);
}

void security_context::add_authorization_headers(reply &rep, const user_details &user)
{
	add_authorization_headers(rep, user, m_default_jwt_exp);
}

// --------------------------------------------------------------------

bool security_context::verify_username_password(const std::string &username, const std::string &raw_password)
{
	auto user = m_users.load_user(username);

	for (auto const &[name, pwenc] : m_known_password_encoders)
	{
		if (!user.password.starts_with(name))
			continue;

		return pwenc->matches(raw_password, user.password);
	}

	// Unknown user or unrecognized encoding: run a dummy PBKDF2 verification so the
	// response time is indistinguishable from a real password check. This prevents an
	// attacker from enumerating valid usernames through a timing side channel.
	verify_dummy_password(raw_password);

	return false;
}

void security_context::verify_dummy_password(const std::string &raw_password) const
{
	// A well-formed PBKDF2-SHA256 hash used only for its computational cost, so that the
	// response time for an unknown user is indistinguishable from a real password check.
	// The salt and hash values are irrelevant to the outcome.
	std::string dummy_hash = "pbkdf2_sha256$" + std::to_string(m_dummy_iterations) + "$QUFBQUFBQUFBQUFB$AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";

	pbkdf2_sha256_password_encoder encoder;
	(void)encoder.matches(raw_password, dummy_hash);
}

void security_context::verify_username_password(const std::string &username, const std::string &raw_password, reply &rep)
{
	if (not login_attempt_allowed(username))
		throw invalid_password_exception();

	if (not verify_username_password(username, raw_password))
	{
		record_login_failure(username);
		throw invalid_password_exception();
	}

	record_login_success(username);
	add_authorization_headers(rep, m_users.load_user(username));
}

bool security_context::login_attempt_allowed(const std::string &username) const
{
	std::scoped_lock lock(m_failures_mutex);

	auto it = m_login_failures.find(username);
	if (it == m_login_failures.end())
		return true;

	auto now = std::chrono::steady_clock::now();
	if (now - it->second.first >= m_login_lockout_duration)
	{
		m_login_failures.erase(it);
		return true;
	}

	return it->second.second < m_max_login_attempts;
}

void security_context::record_login_failure(const std::string &username)
{
	std::scoped_lock lock(m_failures_mutex);

	auto now = std::chrono::steady_clock::now();
	auto it = m_login_failures.find(username);

	if (it == m_login_failures.end() or now - it->second.first >= m_login_lockout_duration)
		m_login_failures[username] = { now, 1 };
	else
		++it->second.second;
}

void security_context::record_login_success(const std::string &username)
{
	std::scoped_lock lock(m_failures_mutex);
	m_login_failures.erase(username);
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
