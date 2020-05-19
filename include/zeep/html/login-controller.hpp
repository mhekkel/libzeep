// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

/// \file
/// definition of the zeep::http::login_controller class. This class inherits from
/// html::controller and provides a default for /login and /logout handling.

#include <zeep/html/controller.hpp>

// --------------------------------------------------------------------
//

namespace zeep::html
{

// --------------------------------------------------------------------

/// \brief html controller that handles login and logout
///
/// basic_html_controller is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class login_controller : public html::controller
{
  public:
	login_controller(const std::string& prefix_path = "/");

	/// \brief Create an error reply for the error containing a validation header
	///
	/// When a authentication violation is encountered, this function is called to generate
	/// the appropriate reply.
	/// \param req		The request that triggered this call
	/// \param stale	For Digest authentication, indicates the authentication information is correct but out of date
	/// \param realm	The name of the protected area, might be shown to the user
	/// \param rep		Write the reply in this object
	virtual void create_unauth_reply(const http::request& req, bool stale, const std::string& realm, http::reply& reply);

  protected:

	/// \brief default GET login handler, will simply return the login page
	///
	/// This is the default handler for `GET /login`. If you want to provide a custom login
	/// page, you have to override this method.
	virtual void handle_get_login(const http::request& request, const scope& scope, http::reply& reply);

	/// \brief default POST login handler, will process the credentials from the form in the login page
	///
	/// This is the default handler for `POST /login`. If you want to provide a custom login
	/// procedure, you have to override this method.
	virtual void handle_post_login(const http::request& request, const scope& scope, http::reply& reply);

	/// \brief default logout handler, will return a redirect to the base URL and remove the authentication Cookie
	virtual void handle_logout(const http::request& request, const scope& scope, http::reply& reply);
};

} // namespace zeep::html
