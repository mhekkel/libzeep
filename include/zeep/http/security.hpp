//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

/// \file
/// definition of various classes that help in handling HTTP authentication.

#include <map>
#include <mutex>

#include <zeep/config.hpp>
#include <zeep/exception.hpp>
#include <zeep/crypto.hpp>

#include <zeep/http/request.hpp>
#include <zeep/http/server.hpp>

#include <zeep/json/element.hpp>

// --------------------------------------------------------------------
//

namespace zeep::http
{

/// \brief exception thrown when unauthorized access is detected
///
/// when using authentication, this exception is thrown for unauthorized access

struct unauthorized_exception : public zeep::exception
{
	/// \brief constructor
	///
	/// Constructor, \a realm is the name of the area that should be protected.

	unauthorized_exception(const std::string &realm)
		: exception("unauthorized")
	{
		std::string::size_type n = realm.length();
		if (n > sizeof(m_realm) - 1)
			n = sizeof(m_realm) - 1;
		realm.copy(m_realm, n);
		m_realm[n] = 0;
	}

	char m_realm[256]; ///< Realm for which the authorization failed
};

// --------------------------------------------------------------------

class password_encoder
{
  public:
	virtual ~password_encoder() {}

	virtual std::string encode(const std::string& password) = 0;
	virtual bool matches(const std::string& raw_password, const std::string& stored_password) = 0;
};

// --------------------------------------------------------------------

class pbkdf2_sha256_password_encoder : public password_encoder
{
  public:
	pbkdf2_sha256_password_encoder(int iterations)
		: m_iterations(iterations) {}

	virtual std::string encode(const std::string& password) = 0;
	virtual bool matches(const std::string& raw_password, const std::string& stored_password) = 0;

  private:
	int m_iterations;
};

// --------------------------------------------------------------------

/// \brief class that manages security in a HTTP scope
///
/// Add this to a HTTP server and it will check authentication.
/// Access to certain paths can be limited by specifying which
/// 'roles' are allowed.
///
/// The mechanism used is based on JSON Web Tokens, JWT in short.

class security_context
{
  public:
	/// \brief constructor taking a validator
	///
	/// Create a security context for server \a s with validator \a validator and
	/// a flag \a defaultAccessAllowed indicating if non-matched uri's should be allowed

	security_context(server& s, password_encoder* encoder, bool defaultAccessAllowed = false)
		: m_server(s), m_default_allow(defaultAccessAllowed), m_encoder(encoder) {}

	/// \brief Add a new rule for access
	///
	/// A new rule will be added to the list, allowing access to \a glob_pattern
	/// to users having role \a role
	void add_rule(const std::string& glob_pattern, const std::string& role)
	{
		m_rules.emplace_back(rule { glob_pattern, { role } });
	}

	/// \brief Add a new rule for access
	///
	/// A new rule will be added to the list, allowing access to \a glob_pattern
	/// to users having a role in \a roles
	void add_rule(const std::string& glob_pattern, std::initializer_list<std::string> roles)
	{
		m_rules.emplace_back(rule { glob_pattern, roles });
	}

	/// \brief Validate the request \a req against the stored rules
	///
	/// This method will validate the request in \a req agains the stored rules
	/// and will throw an exception if access is not allowed.
	void validate_request(const request& req) const;

  private:
	security_context(const security_context&) = delete;
	security_context& operator=(const security_context&) = delete;

	struct rule
	{
		std::string				m_pattern;
		std::set<std::string>	m_roles;
	};

	server &m_server;
	std::string m_secret;
	bool m_default_allow;
	std::unique_ptr<password_encoder> m_encoder;
	std::vector<rule> m_rules;
};

// --------------------------------------------------------------------

/// \brief base class for the authentication validation system
///
/// This is an abstract base class. Derived implementation should
/// at least provide the validate_authentication method.

class authentication_validation_base
{
  public:
	/// \brief constructor
	///
	/// Constructor, \a realm is the name of the area that should be protected.
	authentication_validation_base(const std::string& realm)
		: m_realm(realm) {}
	virtual ~authentication_validation_base() {}

	authentication_validation_base(const authentication_validation_base &) = delete;
	authentication_validation_base &operator=(const authentication_validation_base &) = delete;

	/// \brief Return the name of the protected area
	const std::string& get_realm() const			{ return m_realm; }

	/// \brief Validate the authorization, returns the validated user or null
	///
	/// Validate the authorization using the information available in \a req and return
	/// a JSON object containing the credentials. This should at least contain a _username_.
	/// Return the _null_ object (empty object) when authentication fails.
	///
	/// \param req	The zeep::http::request object
	/// \result		A zeep::json::element object containing the credentials
	virtual json::element validate_authentication(const request &req) = 0;

	/// \brief validate whether \a password is the valid password for \a username
	///
	/// This method should check the username/password combination. It if is valid, it should
	/// return a JSON object containing the 'username' and optionally other fields.
	///
	/// \param username	The username
	/// \param password	The password, in clear text
	/// \result			A zeep::json::element object containing the credentials. In case of JWT
	///					this is stored in a Cookie, so keep it compact.
	virtual json::element validate_username_password(const std::string &username, const std::string &password);

	/// \brief Add e.g. headers to reply for an unauthorized request
	///
	/// When validation fails, the unauthorized HTTP reply is sent back to the user. This
	/// routine will be called to augment the reply with additional information.
	///
	/// \param rep		Then zeep::http::reply object that will be send to the user
	virtual void add_challenge_headers(reply &rep) {}

	/// \brief Add e.g. headers to reply for an authorized request
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with additional information.
	///
	/// \param rep			Then zeep::http::reply object that will be send to the user
	/// \param credentials	The credentials for the authorized user.
	virtual void add_authorization_headers(reply &rep, const json::element& credentials) {}

  protected:
	std::string m_realm;
};

// --------------------------------------------------------------------

/// \brief A base class for JSON Web Token based authentication using JWS
///
/// This is a most basic implementation of authentication using JSON Web Tokens
/// See for more information e.g. https://tools.ietf.org/html/rfc7519 or
/// https://en.wikipedia.org/wiki/JSON_Web_Token

class jws_authentication_validation_base : public authentication_validation_base
{
  public:
	/// \brief constructor
	///
	/// \param realm  The name of the area that should be protected. This unique string will be put in the 'sub' field
	/// \param secret The secret used to sign the token

	jws_authentication_validation_base(const std::string &realm, const std::string &secret)
		: authentication_validation_base(realm), m_secret(secret) {}

	/// \brief Validate the authorization, returns the validated user or null
	///
	/// Validate the authorization using the information available in \a req and return
	/// a JSON object containing the credentials. This should at least contain a _username_.
	/// Return the _null_ object (empty object) when authentication fails.
	///
	/// \param req	The zeep::http::request object
	/// \result		A zeep::json::element object containing the credentials
	virtual json::element validate_authentication(const request &req);

	/// \brief Add e.g. headers to reply for an authorized request
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with the JWT Cookie called access_token.
	///
	/// \param rep			Then zeep::http::reply object that will be send to the user
	/// \param credentials	The credentials for the authorized user.
	virtual void add_authorization_headers(reply &rep, const json::element& credentials);

	/// \brief Return whether the built-in login dialog should be used
	///
	/// JWT authentication works with a login form. The default implementation has
	/// a simple implementation of this form. Subclasses can decide to provide their
	/// own implementation and should return false.
	/// \return		True if the default login form should be used
	virtual bool handles_login() const
	{
		return true;
	}

  private:
	std::string m_sub, m_secret;
};

// --------------------------------------------------------------------
/// \brief Simple implementation of the zeep::http::digest_authentication_validation class
///
/// This class takes a list of username/password pairs in the constructor.

class simple_jws_authentication_validation : public jws_authentication_validation_base
{
  public:
	struct user_password_pair
	{
		std::string username, password;
	};

	/// \brief constructor
	///
	/// \param realm		The name of the area that should be protected.
	/// \param secret The secret used to sign the token
	/// \param validUsers	The list of username/passwords that are allowed to access the realm.
	simple_jws_authentication_validation(const std::string &realm, const std::string &secret,
										 std::initializer_list<user_password_pair> validUsers);

	/// \brief validate whether \a password is the valid password for \a username
	///
	/// This method compares the username/password with the stored list.
	///
	/// \param username	The username
	/// \param password	The password, in clear text
	/// \result			A zeep::json::element object containing the credentials. In case of JWT
	///					this is stored in a Cookie, so keep it compact.
	virtual json::element validate_username_password(const std::string &username, const std::string &password);

  private:
	std::map<std::string, std::string> m_user_hashes;
};

} // namespace zeep::http