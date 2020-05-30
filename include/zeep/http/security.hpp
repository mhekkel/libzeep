//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of various classes that help in handling HTTP authentication.

#include <zeep/config.hpp>

#include <set>

#include <zeep/exception.hpp>
#include <zeep/crypto.hpp>
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
	unauthorized_exception() : exception("unauthorized") { }
};

// --------------------------------------------------------------------

class password_encoder
{
  public:
	virtual ~password_encoder() {}

	virtual std::string encode(const std::string& password) const = 0;
	virtual bool matches(const std::string& raw_password, const std::string& stored_password) const = 0;
};

// --------------------------------------------------------------------

class pbkdf2_sha256_password_encoder : public password_encoder
{
  public:
	static inline constexpr const char* name() { return "pbkdf2_sha256"; };

	pbkdf2_sha256_password_encoder(int iterations = 30000, int key_length = 32)
		: m_iterations(iterations), m_key_length(key_length) {}

	virtual std::string encode(const std::string& password) const
	{
		using namespace std::literals;

		auto salt = zeep::encode_base64(zeep::random_hash()).substr(12);
		auto pw = zeep::encode_base64(zeep::pbkdf2_hmac_sha256(salt, password, m_iterations, m_key_length));
		return "pbkdf2_sha256$" + std::to_string(m_iterations) + '$' + salt + '$' + pw;
	}

	virtual bool matches(const std::string& raw_password, const std::string& stored_password) const
	{
		using namespace std::literals;

		bool result = false;

		std::regex rx(R"(pbkdf2_sha256\$(\d+)\$([^$]+)\$(.+))");

		std::smatch m;
		if (std::regex_match(stored_password, m, rx))
		{
			auto salt = m[2].str();
			auto iterations = std::stoul(m[1]);

			auto test = zeep::pbkdf2_hmac_sha256(salt, raw_password, iterations, m_key_length);
			test = zeep::encode_base64(test);

			result = (m[3] == test);
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
	user_details() {}
	user_details(const std::string& username, const std::string& password, const std::set<std::string>& roles)
		: username(username), password(password), roles(roles) {}

	std::string username;
	std::string password;
	std::set<std::string> roles;
};

// --------------------------------------------------------------------

/// \brief exception thrown by user_service when trying to load user_details for an unknown user
class user_unknown_exception : public zeep::exception
{
  public:
	user_unknown_exception() : zeep::exception("user unknown") {};
};

/// \brief exception thrown by security_context when a username/password combo is not valid
class invalid_password_exception : public zeep::exception
{
  public:
	invalid_password_exception() : zeep::exception("invalid password") {};
};

// --------------------------------------------------------------------

/// \brief The user service class, provding user data used for authentication
///
/// This is an abstract base class for a user service.

class user_service
{
  public:
	user_service() {}
	virtual ~user_service() {}

	/// \brief return the user_details for a user named \a username
	virtual user_details load_user(const std::string& username) const = 0;
};

// --------------------------------------------------------------------

/// \brief A very simple implementation of the user service class
///
/// This implementation of a user service can be used to jump start a
/// project. Normally you would implement something more robust.

class simple_user_service : public user_service
{
  public:
	simple_user_service(std::initializer_list<std::tuple<std::string,std::string,std::set<std::string>>> users)
	{
		for (auto const& [username, password, roles]: users)
			add_user(username, password, roles);
	}

	/// \brief return the user_details for a user named \a username
	virtual user_details load_user(const std::string& username) const
	{
		user_details result = {};
		auto ui = std::find_if(m_users.begin(), m_users.end(), [username](const user_details& u) { return u.username == username; });
		if (ui != m_users.end())
			result = *ui;
		return result;
	}

	void add_user(const std::string& username, const std::string& password, const std::set<std::string>& roles)
	{
		m_users.emplace_back(username, password, roles);
	}

  protected:
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
	/// \brief constructor taking a validator
	///
	/// Create a security context for server \a s with validator \a validator and
	/// a flag \a defaultAccessAllowed indicating if non-matched uri's should be allowed
	security_context(const std::string& secret, user_service& users, bool defaultAccessAllowed = false)
		: m_secret(secret), m_users(users), m_default_allow(defaultAccessAllowed)
	{
		register_password_encoder<pbkdf2_sha256_password_encoder>();
	}

	/// \brief register a custom password encoder
	///
	/// The password encoder should derive from the abstract password encoder class above
	/// and also implement the name() method.
	template<typename PWEncoder>
	void register_password_encoder()
	{
		m_known_password_encoders.emplace_back(PWEncoder::name(), new PWEncoder());
	}

	/// \brief Add a new rule for access
	///
	/// A new rule will be added to the list, allowing access to \a glob_pattern
	/// to users having role \a role
	///
 	/// \a glob_pattern should start with a slash
	void add_rule(const std::string& glob_pattern, const std::string& role)
	{
		add_rule(glob_pattern, { role });
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
		m_rules.emplace_back(rule{glob_pattern, roles});
	}

	/// \brief Validate the request \a req against the stored rules
	///
	/// This method will validate the request in \a req agains the stored rules
	/// and will throw an exception if access is not allowed.
	void validate_request(const request& req) const;

	/// \brief Return the credentials in the request \a req
	///
	/// Will return the null object in case the credentials are not valid
	/// \param req	The current HTTP request
	/// \return		The json object with the credentials, or the null object
	json::element get_credentials(const request& req) const;

	/// \brief Add e.g. headers to reply for an authorized request
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with additional information.
	///
	/// \param rep			Then zeep::http::reply object that will be send to the user
	/// \param username		The name for the authorized user, credentials will be fetched from the user_service
	void add_authorization_headers(reply &rep, const std::string& username);

	/// \brief Add e.g. headers to reply for an authorized request
	///
	/// When validation succeeds, a HTTP reply is send to the user and this routine will be
	/// called to augment the reply with additional information.
	///
	/// \param rep			Then zeep::http::reply object that will be send to the user
	/// \param user			The authorized user details
	void add_authorization_headers(reply &rep, const user_details user);

	/// \brief verify the username/password combination and set a cookie in the reply in case of success
	///
	/// When validation succeeds, add_authorization_headers is called, otherwise an exception is thrown.
	///
	/// \param username		The name for the user
	/// \param password		The password for the user
	/// \param rep			Then zeep::http::reply object that will be send back to the browser
	void verify_username_password(const std::string& username, const std::string& password, reply &rep);

	/// \brief return reference to the user_service object
	user_service& get_user_service() const			{ return m_users; }

	/// \brief CSRF support
	///
	/// Return a CSRF token. If this was not present in the request, a new will be generated
	/// \param req		The HTTP request
	/// \return			A std::pair containing the CSRF token and a flag indicating the token is new
	std::pair<std::string,bool> get_csrf_token(request& req);

  private:
	security_context(const security_context&) = delete;
	security_context& operator=(const security_context&) = delete;

	struct rule
	{
		std::string				m_pattern;
		std::set<std::string>	m_roles;
	};

	std::string m_secret;
	user_service& m_users;
	bool m_default_allow;
	std::vector<rule> m_rules;
	std::vector<std::tuple<std::string,std::unique_ptr<password_encoder>>> m_known_password_encoders;
};


} // namespace zeep::http