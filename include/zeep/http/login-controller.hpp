// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2026
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::login_controller class. This class inherits from
/// html::controller and provides a default for /login and /logout handling.

#include "zeep/http/html-controller.hpp"
#include "zeep/http/reply.hpp"

#include <zeem/zeem.hpp>

#include <string>

// --------------------------------------------------------------------
//

namespace zeep::http
{

class request;
class scope;

// --------------------------------------------------------------------

/// \brief http controller that handles login and logout
///
/// There is a html version of this controller as well, that one is a bit nicer

class login_controller : public html_controller
{
  public:
	login_controller(const std::string &prefix_path = "/");
	~login_controller() override;

	/// \brief bind this controller to \a server
	///
	/// Makes sure the server has a security context and adds rules
	/// to this security context to allow access to the /login page
	void set_server(basic_server *server) override;

	/// \brief return the XHTML login form, subclasses can override this to provide custom login forms
	///
	/// The document returned should have input fields for 'username', 'password' and a hidden '_csrf'
	/// and 'uri' value.
	///
	/// The _csrf value is used to guard against CSRF attacks. The uri is the location to redirect to
	/// in case of a valid login.
	///
	/// \param req		The request that triggered this call
	[[nodiscard]] virtual zeem::document load_login_form(const request &req) const;

	/// \brief Create an error reply for an unauthorized access
	///
	/// An error handler may call this method to create a decent login screen.
	/// \param req		The request that triggered this call
	/// \param rep		Write the reply in this object
	virtual void create_unauth_reply(const request &req, reply &rep);

	/// \brief Handle a GET on /login
	[[nodiscard]] reply handle_get_login(const scope &scope_);

	/// \brief Handle a POST on /login
	[[nodiscard]] reply handle_post_login(const scope &scope_, const std::string &username, const std::string &password);

	/// \brief Handle a GET or POST on /logout
	[[nodiscard]] reply handle_logout(const scope &scope_);

	/// \brief Return a reply for a redirect to the requested or default destination.
	[[nodiscard]] reply create_redirect_for_request(const request &req) const;

  private:
	std::shared_ptr<int> m_alive;
};

} // namespace zeep::http
