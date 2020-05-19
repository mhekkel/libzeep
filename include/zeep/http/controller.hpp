// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the base class zeep::http::controller, used by e.g. rest_controller and soap_controller

#include <zeep/http/server.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>

namespace zeep::http
{

/// \brief A base class for controllers, classes that handle a request
///
/// This concept is inspired by the Spring way of delegating the work to
/// controller classes. In libzeep there are two major implementations of
/// controllers: zeep::http::rest_controller and zeep::http::soap_controller
///
/// There can be multiple controllers in a web application, each is connected
/// to a certain prefix-path. This is the leading part of the request URI.

class controller
{
  public:
	/// \brief constructor
	///
	/// \param prefix_path  The prefix path this controller is bound to

	controller(const std::string& prefix_path);

	virtual ~controller();

	virtual bool handle_request(request& req, reply& rep);

	/// \brief returns the defined prefix path
	std::string get_prefix() const      { return m_prefix_path; }

	/// \brief return whether this uri request path matches our prefix
	bool path_matches_prefix(const std::string& path) const;

	/// \brief bind this controller to \a server
	void set_server(server* server)
	{
		m_server = server;
	}

	/// \brief return the server object we're bound to
	const server& get_server() const	{ return *m_server; }
	server& get_server()				{ return *m_server; }

  protected:

	controller(const controller&) = delete;
	controller& operator=(const controller&) = delete;

	std::string m_prefix_path;
	server* m_server = nullptr;
};

} // namespace zeep::http
