// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/security.hpp"
#include "detail/glob.hpp"
#include "zeep/crypto.hpp"
#include "zeep/el/object.hpp"
#include "zeep/el/processing.hpp"
#include "zeep/http/client.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/uri.hpp"

#include "glob.hpp"

#include <openssl/bio.h>
#include <openssl/decoder.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>
#include <openssl/rsa.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <regex>
#include <set>
#include <sstream>
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

openid_user_service::openid_user_service(
	zeep::uri uri, std::string name, zeep::uri redirect_uri, std::string client_id, std::string client_secret)
	: m_base_uri(std::move(uri))
	, m_name(std::move(name))
	, m_redirect_uri(std::move(redirect_uri))
	, m_client_id(std::move(client_id))
	, m_client_secret(std::move(client_secret))
{
	m_config = fetch(m_base_uri / ".well-known" / "openid-configuration");
	zeep::uri jwks_uri = zeep::uri(m_config["jwks_uri"].get<std::string>());
	m_keys = fetch(jwks_uri)["keys"];
}

zeep::el::object openid_user_service::fetch(zeep::uri uri)
{
	auto r = zeep::http::get_request(uri);

	if (r.get_status() != zeep::http::ok)
	{
		if (r.get_content_type() == "application/json")
		{
			auto error = zeep::el::object::parse_JSON(r.get_content());
			throw std::runtime_error(std::format("Failed to fetch openid information: {} ({})",
				error["error"].get<std::string>(), error["error_description"].get<std::string>()));
		}
		throw std::runtime_error(
			std::format("Failed to fetch openid information: {}", r.get_content()));
	}

	if (r.get_content_type() != "application/json")
		throw std::runtime_error("Failed to fetch openid information");

	return zeep::el::object::parse_JSON(r.get_content());
}

bool openid_user_service::verify_rsa(const std::string &kid, const std::string &message, const std::string &signature)
{
	bool result = false;

	auto hash = zeep::sha256(message);

	for (auto key : m_keys)
	{
		if (key["kid"] != kid)
			continue;

		// Create bignums
		auto ns = zeep::decode_base64url(key["n"].get<std::string>());
		auto es = zeep::decode_base64url(key["e"].get<std::string>());

		BIGNUM *bnn = nullptr, *bne = nullptr;
		OSSL_PARAM_BLD *params_build = nullptr;

		bnn = BN_bin2bn(reinterpret_cast<unsigned char *>(ns.data()), ns.length(), nullptr);
		bne = BN_bin2bn(reinterpret_cast<unsigned char *>(es.data()), es.length(), nullptr);

		params_build = OSSL_PARAM_BLD_new();

		if (bnn and bne and params_build and
			OSSL_PARAM_BLD_push_BN(params_build, "n", bnn) and
			OSSL_PARAM_BLD_push_BN(params_build, "e", bne))
		{
			if (auto params = OSSL_PARAM_BLD_to_param(params_build))
			{
				EVP_PKEY_CTX *pctx = nullptr;
				EVP_PKEY *pkey = nullptr;

				if (pctx = EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr);
					pctx and
					EVP_PKEY_fromdata_init(pctx) == 1 and
					EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) == 1)
				{
					if (auto ctx = EVP_PKEY_CTX_new(pkey, nullptr))
					{
						if (EVP_PKEY_verify_init(ctx) > 0 and
							EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) > 0 and
							EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) > 0)
						{
							auto r = EVP_PKEY_verify(ctx,
								reinterpret_cast<const unsigned char *>(signature.data()), signature.length(),
								reinterpret_cast<const unsigned char *>(hash.data()), hash.length());

							result = r == 1;
						}

						EVP_PKEY_CTX_free(ctx);
					}
				}
				if (pctx)
					EVP_PKEY_CTX_free(pctx);

				OSSL_PARAM_free(params);
			}
		}

		if (params_build)
			OSSL_PARAM_BLD_free(params_build);

		if (bne)
			BN_free(bne);

		if (bnn)
			BN_free(bnn);

		ERR_print_errors_fp(stdout);

		break;
	}

	return result;
}

bool openid_user_service::validate_request(request &req) const
{
	return false;
}

user_details openid_user_service::load_user(const std::string &username) const
{
	return {};
}

// --------------------------------------------------------------------

security_context::security_context(std::string secret, user_service &users, bool /* defaultAccessAllowed */)
	: m_secret(std::move(secret))
	, m_users(users)
	, m_default_jwt_exp(std::chrono::weeks{ 1 })
{
#if __has_include(<sys/mman.h>)
	mlock(m_secret.data(), m_secret.length());
#endif

	register_password_encoder<pbkdf2_sha256_password_encoder>();
}

security_context::~security_context()
{
	// wipe out secret
	for (char &ch : m_secret)
		ch = 0;
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

			if (not JOSEHeader.contains("typ") or JOSEHeader["typ"] != "JWT")
				break;

			if (auto alg = JOSEHeader["alg"].get<std::string>(); alg == "HS256")
			{
				// check signature
				auto sig = zeep::encode_base64url(zeep::hmac_sha256(m[0] + '.' + m[1], m_secret));
				if (sig != m[2])
					break;
			}
			else if (alg == "RS256")
			{
				auto msg = m[0] + '.' + m[1];
				auto sig = zeep::decode_base64url(m[2]);

				if (not verify_rsa(JOSEHeader["kid"].get<std::string>(), msg, sig))
					break;
			}
			else
				break;

			// check signature
			auto sig = encode_base64url(hmac_sha256(m[0] + '.' + m[1], m_secret));

			// Apparently, we need a constant time comparison here to avoid timing attacks
			if (sig.length() != m[2].length())
				break;

			// Compare strings in constant time
			int diff = 0;
			for (const auto &[a, b] : std::views::zip(sig, m[2]))
				diff |= a xor b;
			if (diff)
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

	if (allow and m_validate_csrf)
	{
		if (auto p = req.get_parameter("_csrf"); p.has_value())
		{
			const auto &req_csrf_param = *p;
			if (auto req_csrf_cookie = req.get_cookie("csrf-token"); req_csrf_cookie != req_csrf_param)
			{
				allow = false;
				std::clog << "CSRF validation failed\n";
			}
		}
	}

	if (not allow)
		throw unauthorized_exception();
}

bool security_context::verify_rsa(const std::string &kid, const std::string &message, const std::string &signature) const
{
	bool result = false;

	auto hash = zeep::sha256(message);

	for (auto key : m_keys)
	{
		if (key["kid"] != kid)
			continue;

		// Create bignums
		auto ns = zeep::decode_base64url(key["n"].get<std::string>());
		auto es = zeep::decode_base64url(key["e"].get<std::string>());

		auto bnn = BN_bin2bn(reinterpret_cast<unsigned char *>(ns.data()), ns.length(), nullptr);
		auto bne = BN_bin2bn(reinterpret_cast<unsigned char *>(es.data()), es.length(), nullptr);

		auto params_builder = OSSL_PARAM_BLD_new();

		if (bnn and bne and params_builder and
			OSSL_PARAM_BLD_push_BN(params_builder, "n", bnn) and
			OSSL_PARAM_BLD_push_BN(params_builder, "e", bne))
		{
			if (auto param = OSSL_PARAM_BLD_to_param(params_builder))
			{
				EVP_PKEY_CTX *pctx = nullptr;
				EVP_PKEY *pkey = nullptr;

				if (pctx = EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr);
					pctx and
					EVP_PKEY_fromdata_init(pctx) == 1 and
					EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, param) == 1)
				{
					if (auto ctx = EVP_PKEY_CTX_new(pkey, nullptr))
					{
						if (EVP_PKEY_verify_init(ctx) == 1 and
							EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) == 1 and
							EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) == 1)
						{
							auto r = EVP_PKEY_verify(ctx,
								reinterpret_cast<const unsigned char *>(signature.data()), signature.length(),
								reinterpret_cast<const unsigned char *>(hash.data()), hash.length());

							result = r == 1;
						}

						EVP_PKEY_CTX_free(ctx);
					}
				}
				if (pctx)
					EVP_PKEY_CTX_free(pctx);

				OSSL_PARAM_free(param);
			}
		}

		if (params_builder)
			OSSL_PARAM_BLD_free(params_builder);

		if (bne)
			BN_free(bne);

		if (bnn)
			BN_free(bnn);

		ERR_print_errors_fp(stderr);

		break;
	}

	return result;
}

// --------------------------------------------------------------------

void security_context::add_authorization_headers(reply &rep, const user_details user,
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
	bool result = false;

	auto user = m_users.load_user(username);

	for (auto const &[name, pwenc] : m_known_password_encoders)
	{
		if (!user.password.starts_with(name))
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
