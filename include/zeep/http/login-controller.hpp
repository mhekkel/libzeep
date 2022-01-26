// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::login_controller class. This class inherits from
/// html::controller and provides a default for /login and /logout handling.

#include <zeep/config.hpp>

#include <zeep/xml/document.hpp>
#include <zeep/http/controller.hpp>

// --------------------------------------------------------------------
//

namespace zeep::http
{

// --------------------------------------------------------------------

/// \brief http controller that handles login and logout
///
/// There is a html version of this controller as well, that one is a bit nicer

class login_controller : public controller
{
  public:
	login_controller(const std::string& prefix_path = "/");

	/// \brief bind this controller to \a server
	///
	/// Makes sure the server has a security context and adds rules
	/// to this security context to allow access to the /login page
	virtual void set_server(basic_server* server);

	/// \brief will handle the actual requests
	virtual bool handle_request(request& req, reply& rep);

	/// \brief return the XHTML login form, subclasses can override this to provide custom login forms
	///
	/// The document returned should have input fields for 'username', 'password' and a hidden '_csrf'
	/// and 'uri' value.
	///
	/// The _csrf value is used to guard against CSRF attacks. The uri is the location to redirect to
	/// in case of a valid login.
	virtual xml::document load_login_form(const request& req) const;

	/// \brief Create an error reply for an unauthorized access
	///
	/// An error handler may call this method to create a decent login screen.
	/// \param req		The request that triggered this call
	/// \param rep		Write the reply in this object
	virtual void create_unauth_reply(const request& req, reply& reply);
};

}
