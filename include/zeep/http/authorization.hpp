//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

#include <map>
#include <mutex>

#include <zeep/config.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/server.hpp>
#include <zeep/el/element.hpp>

// --------------------------------------------------------------------
//

namespace zeep::http
{

/// when using authentication, this exception is thrown for unauthorized access

struct unauthorized_exception : public zeep::exception
{
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

struct authorization_stale_exception : public unauthorized_exception
{
	authorization_stale_exception(const std::string &realm)
		: unauthorized_exception(realm) {}
};

/// class to use in authentication
struct auth_info;
using auth_info_list = std::list<auth_info>;

// --------------------------------------------------------------------

class authentication_validation_base
{
  public:
	authentication_validation_base(const std::string& realm)
		: m_realm(realm) {}
	virtual ~authentication_validation_base() {}

	authentication_validation_base(const authentication_validation_base &) = delete;
	authentication_validation_base &operator=(const authentication_validation_base &) = delete;

	const std::string& get_realm() const			{ return m_realm; }

	/// Validate the authorization, returns the validated user. Throws unauthorized_exception in case of failure
	virtual el::element validate_authentication(const request &req) = 0;

	/// This method should check the username/password combination. It if is valid, it should
	/// return a JSON object containing the 'username' and optionally other fields.
	virtual el::element validate_username_password(const std::string &username, const std::string &password);

	/// Add e.g. headers to reply for an unauthorized request
	virtual void add_challenge_headers(reply &rep, bool stale) {}

	/// Add e.g. headers to reply for an authorized request
	virtual void add_authorization_headers(reply &rep, const el::element& credentials) {}

  protected:
	std::string m_realm;
};

// --------------------------------------------------------------------
/// Digest access authentication based on RFC 2617

class digest_authentication_validation : public authentication_validation_base
{
  public:
	digest_authentication_validation(const std::string& realm);
	~digest_authentication_validation();

	virtual el::element validate_authentication(const request &req);

	/// Add headers to reply for an unauthorized request
	virtual void add_challenge_headers(reply &rep, bool stale);

	/// Subclasses should implement this to return the password for the user,
	/// result should be the MD5 hash of the string username + ':' + realm + ':' + password
	virtual std::string get_hashed_password(const std::string &username) = 0;

  private:
	auth_info_list m_auth_info;
	std::mutex m_auth_mutex;
};

// --------------------------------------------------------------------

class simple_digest_authentication_validation : public digest_authentication_validation
{
  public:
	struct user_password_pair
	{
		std::string username, password;
	};

	simple_digest_authentication_validation(const std::string &realm, std::initializer_list<user_password_pair> validUsers);
	virtual std::string get_hashed_password(const std::string &username);

  private:
	std::map<std::string, std::string> m_user_hashes;
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
	/// \param realm  The unique string that should be put in the 'sub' field
	/// \param secret The secret used to sign the token

	jws_authentication_validation_base(const std::string &realm, const std::string &secret)
		: authentication_validation_base(realm), m_secret(secret) {}

	/// Validate the authorization, returns the validated user or a null object in case of failure
	virtual el::element validate_authentication(const request &req);

	/// Add e.g. headers to reply for an authorized request
	virtual void add_authorization_headers(reply &rep, const el::element& credentials);

	virtual bool handles_login() const
	{
		return true;
	}

  private:
	std::string m_sub, m_secret;
};

// --------------------------------------------------------------------

class simple_jws_authentication_validation : public jws_authentication_validation_base
{
  public:
	struct user_password_pair
	{
		std::string username, password;
	};

	simple_jws_authentication_validation(const std::string &realm, const std::string &secret,
										 std::initializer_list<user_password_pair> validUsers);

	virtual el::element validate_username_password(const std::string &username, const std::string &password);

  private:
	std::map<std::string, std::string> m_user_hashes;
};

} // namespace zeep::http