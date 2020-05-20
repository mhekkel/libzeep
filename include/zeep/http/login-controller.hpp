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

	virtual bool handle_request(request& req, reply& rep);
};

}
