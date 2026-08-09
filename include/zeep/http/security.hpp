// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of various classes that help in handling HTTP authentication.

#include "zeep/crypto.hpp"
#include "zeep/el/processing.hpp"
#include "zeep/exception.hpp"
#include "zeep/http/status.hpp"
#include "zeep/unicode-support.hpp"

#include <cassert>
#include <charconv>
#include <chrono>
#include <initializer_list>
#include <memory>
#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// --------------------------------------------------------------------
//

namespace zeep::http
{

class reply;
class request;

/// \brief exception thrown when unauthorized access is detected
///
/// when using authentication, this exception is thrown for unauthorized access

struct unauthorized_exception : public http_status_exception
{
	/// \brief constructor
	unauthorized_exception()
		: http_status_exception(status_type::unauthorized)
	{
	}
};

// --------------------------------------------------------------------

/// \brief Base class for password encoders
class password_encoder
{
  public:
	virtual ~password_encoder() = default;

	/// \brief Encode a password into a stored-password string
	/// \param password  The raw password to encode
	/// \return The encoded password string (suitable for storage)
	[[nodiscard]] virtual std::string encode(const std::string &password) const = 0;

	/// \brief Check a raw password against a stored encoded password
	/// \param raw_password    The raw password to verify
	/// \param stored_password The previously encoded password string
	/// \return True if the password matches
	[[nodiscard]] virtual bool matches(const std::string &raw_password, const std::string &stored_password) const = 0;
};

// --------------------------------------------------------------------

/// \brief Implementation of @ref zeep::http::password_encoder for the PBKDF2-SHA256 algorithm
/// https://en.wikipedia.org/wiki/PBKDF2
class pbkdf2_sha256_password_encoder : public password_encoder
{
  public:
	/// \brief Return the encoder name identifier
	/// \return "pbkdf2_sha256"
	static inline constexpr const char *name() { return "pbkdf2_sha256"; };

	/// \brief Construct with optional iteration count and key length
	/// \param iterations  Number of PBKDF2 iterations (default 100,000)
	/// \param key_length  Desired key length in bytes (default 32)
	pbkdf2_sha256_password_encoder(int iterations = 100'000, int key_length = 32)
		: m_iterations(iterations)
		, m_key_length(key_length)
	{
	}

	/// \brief Encode a password using PBKDF2-SHA256
	/// \param password  The raw password to encode
	/// \return An encoded string in the format "pbkdf2_sha256$iterations$salt$hash"
	[[nodiscard]] std::string encode(const std::string &password) const override
	{
		using namespace std::literals;

		auto salt = zeep::encode_base64(zeep::random_hash()).substr(12);
		auto pw = zeep::encode_base64(zeep::pbkdf2_hmac_sha256(salt, password, m_iterations, m_key_length));
		return "pbkdf2_sha256$" + std::to_string(m_iterations) + '$' + salt + '$' + pw;
	}

	/// \brief Verify a raw password against a stored PBKDF2-SHA256 encoded string
	/// \param raw_password    The raw password to verify
	/// \param stored_password The previously encoded password string
	/// \return True if the password matches
	[[nodiscard]] bool matches(const std::string &raw_password, const std::string &stored_password) const override
	{
		using namespace std::literals;

		bool result = false;

		auto parts = split(stored_password, "$");
		
		if (parts.size() == 4 and parts.front() == "pbkdf2_sha256")
		{
			int iterations;
			const auto &[ptr, ec] = std::from_chars(parts[1].data(), parts[1].data() + parts[1].size(), iterations);

			if (ec == std::errc{} and ptr == parts[1].data() + parts[1].length())
			{
				auto salt = parts[2];
	
				auto test = zeep::pbkdf2_hmac_sha256(salt, raw_password, iterations, m_key_length);
				test = zeep::encode_base64(test);
	
				result = (parts[3] == test);
			}
		}

		return result;
	}

  private:
	int m_iterations, m_key_length;
};

// --------------------------------------------------------------------

/// \brief simple storage class for user details, returned by user_service
///
/// The user_details struct contains all the information needed to allow
/// access to a resource based on username. The password is the encrypted
/// password.
struct user_details
{
	user_details() = default;
	/// \brief Construct with username, password and roles
	/// \param username  The user's login name
	/// \param password  The (encoded) password
	/// \param roles     The set of roles assigned to this user
	user_details(std::string username, std::string password, std::set<std::string> roles)
		: username(std::move(username))
		, password(std::move(password))
		, roles(std::move(roles))
	{
	}

	std::string username;        ///< The user's login name
	std::string password;        ///< The encoded password
	std::set<std::string> roles; ///< The set of roles assigned to this user
};

// --------------------------------------------------------------------

/// \brief Exception thrown for general authentication failures
class authentication_exception : public zeep::exception
{
  public:
	/// \brief Construct an authentication exception with a message
	/// \param msg  The description of the authentication error
	authentication_exception(std::string msg)
		: zeep::exception(std::move(msg))
	{
	}

	authentication_exception(const authentication_exception &) noexcept = default;
};

/// \brief exception thrown by user_service when trying to load user_details for an unknown user
class user_unknown_exception : public authentication_exception
{
  public:
	user_unknown_exception()
		: authentication_exception("user unknown")
	{
	}
};

/// \brief exception thrown by security_context when a username/password combo is not valid
class invalid_password_exception : public authentication_exception
{
  public:
	invalid_password_exception()
		: authentication_exception("invalid password")
	{
	}
};

// --------------------------------------------------------------------

/// \brief The user service class, provding user data used for authentication
///
/// This is an abstract base class for a user service.

class user_service
{
  public:
	user_service() = default;
	virtual ~user_service() = default;

	/// \brief return the user_details for a user named \a username
	[[nodiscard]] virtual user_details load_user(const std::string &username) const = 0;

	/// \brief return true if the credentials in \a credentials are still sufficient to access this web application
	/// \param credentials  The credentials object to validate
	/// \return True if the user is still valid
	[[nodiscard]] virtual bool user_is_valid(const el::object &credentials) const;

	/// \brief return true if a user named \a username is allowed to access this web application
	/// \param username  The name of the user to validate
	/// \return True if the user is still valid
	[[nodiscard]] virtual bool user_is_valid(const std::string &username) const;
};

// --------------------------------------------------------------------

/// \brief A very simple implementation of the user service class
///
/// This implementation of a user service can be used to jump start a
/// project. Normally you would implement something more robust.

class simple_user_service : public user_service
{
  public:
	/// \brief Construct with an initializer list of user tuples (username, password, roles)
	/// \param users  Initializer list of (username, password, roles) tuples
	simple_user_service(std::initializer_list<std::tuple<std::string, std::string, std::set<std::string>>> users)
	{
		for (auto const &[username, password, roles] : users)
			add_user(username, password, roles);
	}

	/// \brief return the user_details for a user named \a username
	[[nodiscard]] user_details load_user(const std::string &username) const override
	{
		user_details result = {};
		auto ui = std::ranges::find_if(m_users, [username](const user_details &u)
			{ return u.username == username; });
		if (ui != m_users.end())
			result = *ui;
		return result;
	}

	/// \brief Add a user to the service
	/// \param username  The user's login name
	/// \param password  The (encoded) password
	/// \param roles     The set of roles assigned to this user
	void add_user(std::string username, std::string password, std::set<std::string> roles)
	{
		m_users.emplace_back(std::move(username), std::move(password), std::move(roles));
	}

  protected:
	/// \brief The stored list of users
	std::vector<user_details> m_users;
};

// --------------------------------------------------------------------

/// \brief class that manages security in a HTTP scope
///
/// Add this to a HTTP server and it will check authentication.
/// Access to certain paths can be limited by specifying which
/// 'roles' are allowed.
///
/// The authentication mechanism used is based on JSON Web Tokens, JWT in short.

class security_context
{
  public:
	security_context(const security_context &) = delete;
	security_context &operator=(const security_context &) = delete;

	/// \brief constructor taking a validator
	///
	/// Create a security context for server \a s with validator \a validator and
	/// a flag \a defaultAccessAllowed indicating if non-matched uri's should be allowed
	security_context(std::string secret, user_service &users, bool defaultAccessAllowed = false);

	/// \brief destructor
	~security_context();

	/// \brief register a custom password encoder
	///
	/// The password encoder should derive from the abstract password encoder class above
	/// and also implement the name() method.
	/// \brief Register a custom password encoder
	/// \tparam PWEncoder  A type derived from \a password_encoder with a static name() method
	template <typename PWEncoder>
	void register_password_encoder()
	{
		m_known_password_encoders.emplace_back(PWEncoder::name(), std::make_unique<PWEncoder>());
	}

	/// \brief Add a new rule for access
	///
	/// A new rule will be added to the list, allowing access to \a glob_pattern
	/// to users having role \a role
	///
	/// \a glob_pattern should start with a slash
	void add_rule(std::string glob_pattern, std::string role)
	{
		add_rule(std::move(glob_pattern), { std::move(role) });
	}

	/// \brief Add a new rule for access
	///
	/// A new rule will be added to the list, allowing access to \a glob_pattern
	/// to users having a role in \a roles
	///
	/// If \a roles is empty, access is allowed to anyone.
	///
	/// \a glob_pattern should start with a slash
	void add_rule(std::string glob_pattern, std::initializer_list<std::string> roles)
	{
		assert(glob_pattern.front() == '/');
		m_rules.emplace_back(rule{ std::move(glob_pattern), roles });
	}

	/// \brief Validate the request \a req against the stored rules
	///
	/// This method will validate the request in \a req agains the stored rules
	/// and will throw an exception if access is not allowed.
	/// The request \a req will be updated with the credentials for further use.
	/// If the validate CSRF is set, the CSRF token will also be validated.
	void validate_request(request &req) const;

	/// \brief Add e.g. headers to reply for an authorized request
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with additional information.
	///
	/// \param rep			Then zeep::http::reply object that will be send to the user
	/// \param user			The authorized user details
	void add_authorization_headers(reply &rep, const user_details &user);

	/// \brief Add e.g. headers to reply for an authorized request, with an expiration parameter
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with additional information.
	///
	/// \param rep			The zeep::http::reply object that will be send to the user
	/// \param user			The authorized user details
	/// \param exp			The maximum lifetime for the access token
	void add_authorization_headers(reply &rep, const user_details user,
		std::chrono::system_clock::duration exp);

	/// \brief verify the username/password combination and set a cookie in the reply in case of success
	///
	/// When validation succeeds, add_authorization_headers is called, otherwise an exception is thrown.
	///
	/// \param username		The name for the user
	/// \param password		The password for the user
	/// \param rep			Then zeep::http::reply object that will be send back to the browser
	void verify_username_password(const std::string &username, const std::string &password, reply &rep);

	/// \brief verify the username/password combination and return true if valid
	///
	/// \param username		The name for the user
	/// \param password		The password for the user
	/// \result             True in case of valid combination
	[[nodiscard]] bool verify_username_password(const std::string &username, const std::string &password);

	/// \brief Return reference to the user_service object
	[[nodiscard]] user_service &get_user_service() const noexcept { return m_users; }

	/// \brief Get or create a CSRF token for the current request
	///
	/// Return a CSRF token. If this was not present in the request, a new will be generated
	/// \param req		The HTTP request
	/// \return			A std::pair containing the CSRF token and a flag indicating the token is new
	[[nodiscard]] std::pair<std::string, bool> get_csrf_token(request &req);

	/// \brief To automatically validate CSRF tokens, set this flag
	void set_validate_csrf(bool validate) noexcept { m_validate_csrf = validate; }
	/// \brief Return whether CSRF validation is enabled
	[[nodiscard]] bool get_validate_csrf() const noexcept { return m_validate_csrf; }

	/// \brief Return the default JWT expiration duration
	[[nodiscard]] std::chrono::system_clock::duration get_jwt_exp() const noexcept { return m_default_jwt_exp; }
	/// \brief Set the default JWT expiration duration
	/// \param exp  The expiration duration
	void set_jwt_exp(std::chrono::system_clock::duration exp) noexcept { m_default_jwt_exp = exp; }

  private:
	/// @cond

	struct rule
	{
		std::string m_pattern;
		std::set<std::string> m_roles;
	};

	std::string m_secret;
	user_service &m_users;
	bool m_default_allow;
	bool m_validate_csrf = false;
	std::vector<rule> m_rules;
	std::vector<std::tuple<std::string, std::unique_ptr<password_encoder>>> m_known_password_encoders;
	std::chrono::system_clock::duration m_default_jwt_exp;

	/// @endcond
};

} // namespace zeep::http
