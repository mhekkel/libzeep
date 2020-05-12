//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

/// \file
/// definition of various classes that handle HTTP authentication

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

/// \brief exception thrown when the authentication information has expired
///
/// Some authentication mechanisms have a limited life-time for the token.
/// When the token has expired, this exception is thrown. This happens e.g.
/// in the RFC 2617 HTTP Digest authentication.

struct authorization_stale_exception : public unauthorized_exception
{
	/// \brief constructor
	///
	/// Constructor, \a realm is the name of the area that should be protected.

	authorization_stale_exception(const std::string &realm)
		: unauthorized_exception(realm) {}
};

struct auth_info;
using auth_info_list = std::list<auth_info>;

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
	/// \result		A zeep::el::element object containing the credentials
	virtual el::element validate_authentication(const request &req) = 0;

	/// \brief validate whether \a password is the valid password for \a username
	///
	/// This method should check the username/password combination. It if is valid, it should
	/// return a JSON object containing the 'username' and optionally other fields.
	///
	/// \param username	The username
	/// \param password	The password, in clear text
	/// \result			A zeep::el::element object containing the credentials. In case of JWT
	///					this is stored in a Cookie, so keep it compact.
	virtual el::element validate_username_password(const std::string &username, const std::string &password);

	/// \brief Add e.g. headers to reply for an unauthorized request
	///
	/// When validation fails, the unauthorized HTTP reply is sent back to the user. This
	/// routine will be called to augment the reply with additional information.
	///
	/// \param rep		Then zeep::http::reply object that will be send to the user
	/// \param stale	Boolean indicating whether the authentication provided has expired,
	///					only used by Digest authentication for now.
	virtual void add_challenge_headers(reply &rep, bool stale) {}

	/// \brief Add e.g. headers to reply for an authorized request
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with additional information.
	///
	/// \param rep			Then zeep::http::reply object that will be send to the user
	/// \param credentials	The credentials for the authorized user.
	virtual void add_authorization_headers(reply &rep, const el::element& credentials) {}

  protected:
	std::string m_realm;
};

// --------------------------------------------------------------------
/// \brief Digest access authentication based on RFC 2617
///
/// This abstract class handles authentication according to the Digest HTTP specification.
/// The get_hashed_password method should be implemented by derived classes.

class digest_authentication_validation : public authentication_validation_base
{
  public:
	/// \brief constructor
	///
	/// Constructor, \a realm is the name of the area that should be protected.
	digest_authentication_validation(const std::string& realm);
	~digest_authentication_validation();

	/// \brief Validate the authorization, returns the validated user or null
	///
	/// Validate the authorization using the information available in \a req and return
	/// a JSON object containing the credentials. This should at least contain a _username_.
	/// Return the _null_ object (empty object) when authentication fails.
	///
	/// \param req	The zeep::http::request object
	/// \result		A zeep::el::element object containing the credentials
	virtual el::element validate_authentication(const request &req);

	/// \brief Add the WWW-Authenticate header to a reply for an unauthorized request
	///
	/// When validation fails, the unauthorized HTTP reply is sent back to the user. This
	/// routine will add the WWW-Authenticate header to the reply.
	///
	/// \param rep		Then zeep::http::reply object that will be send to the user
	/// \param stale	Boolean indicating whether the authentication provided has expired,
	///					only used by Digest authentication for now.
	virtual void add_challenge_headers(reply &rep, bool stale);

	/// \brief Get the password for a user, hashed according to the standard
	///
	/// Subclasses should implement this to return the password for the user,
	/// \param username	The user for whom the password should be returned
	/// \result			The MD5 hash of the string \a username + ':' + realm + ':' + password
	///                 hex encoded.
	virtual std::string get_hashed_password(const std::string &username) = 0;

  private:
	auth_info_list m_auth_info;
	std::mutex m_auth_mutex;
};

// --------------------------------------------------------------------
/// \brief Simple implementation of the zeep::http::digest_authentication_validation class
///
/// This class takes a list of username/password pairs in the constructor.

class simple_digest_authentication_validation : public digest_authentication_validation
{
  public:
	struct user_password_pair
	{
		std::string username, password;
	};

	/// \brief constructor
	///
	/// \param realm		The name of the area that should be protected.
	/// \param validUsers	The list of username/passwords that are allowed to access the realm.
	simple_digest_authentication_validation(const std::string &realm, std::initializer_list<user_password_pair> validUsers);

	/// \brief Return the password for a user, hashed according to the standard
	///
	/// This implementation takes the password for \a username if found in the list.
	/// \param username	The user for whom the password should be returned
	/// \result			The MD5 hash of the string \a username + ':' + realm + ':' + password
	///                 hex encoded.
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
	/// \result		A zeep::el::element object containing the credentials
	virtual el::element validate_authentication(const request &req);

	/// \brief Add e.g. headers to reply for an authorized request
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with the JWT Cookie called access_token.
	///
	/// \param rep			Then zeep::http::reply object that will be send to the user
	/// \param credentials	The credentials for the authorized user.
	virtual void add_authorization_headers(reply &rep, const el::element& credentials);

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
	/// \result			A zeep::el::element object containing the credentials. In case of JWT
	///					this is stored in a Cookie, so keep it compact.
	virtual el::element validate_username_password(const std::string &username, const std::string &password);

  private:
	std::map<std::string, std::string> m_user_hashes;
};

} // namespace zeep::http