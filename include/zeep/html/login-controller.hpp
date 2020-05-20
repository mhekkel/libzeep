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
/// The login_controller is a controller that handles both GET and POST
/// on a /login and /logout.

class login_controller : public html::controller
{
  public:
	login_controller(const std::string& prefix_path = "/");

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
