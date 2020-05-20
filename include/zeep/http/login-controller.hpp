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

#include <zeep/http/controller.hpp>

// --------------------------------------------------------------------
//

namespace zeep::http
{

// --------------------------------------------------------------------

/// \brief http controller that handles login and logout
///
/// There is a html version of this controller as well, that one is a bit nicer

class login_controller : public http::controller
{
  public:
	login_controller(const std::string& prefix_path = "/");

	/// \brief Create an error reply for the error containing a validation header
	///
	/// When a authentication violation is encountered, this function is called to generate
	/// the appropriate reply.
	/// \param req		The request that triggered this call
	/// \param realm	The name of the protected area, might be shown to the user
	/// \param rep		Write the reply in this object
	virtual void create_unauth_reply(const http::request& req, const std::string& realm, http::reply& reply);

	virtual bool handle_request(request& req, reply& rep);
};

} // namespace zeep::html
