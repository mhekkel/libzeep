//        Copyright Maarten L. Hekkelman, 2014-2022
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the base class zeep::http::controller, used by e.g. rest_controller and soap_controller

#include <zeep/config.hpp>

#include <zeep/http/server.hpp>

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

	controller(const std::string &prefix_path);

	virtual ~controller();

	/// \brief Calls handle_request but stores a pointer to the request first
	virtual bool dispatch_request(asio_ns::ip::tcp::socket &socket, request &req, reply &rep);

	/// \brief The pure virtual method that actually handles the request
	virtual bool handle_request(request &req, reply &rep) = 0;

	/// \brief returns the defined prefix path
	uri get_prefix() const { return m_prefix_path; }

	/// \brief return whether this uri request path matches our prefix
	bool path_matches_prefix(const uri &path) const;

	/// \brief return the path with the prefix path stripped off
	uri get_prefixless_path(const request &req) const;

	/// \brief bind this controller to \a server
	virtual void set_server(basic_server *server)
	{
		m_server = server;
	}

	/// \brief return the server object we're bound to
	const basic_server *get_server() const { return m_server; }
	basic_server *get_server() { return m_server; }

	/// \brief return the context name, if specified. Empty string otherwise
	std::string get_context_name() const
	{
		return m_server ? m_server->get_context_name() : "";
	}

	/// \brief get the credentials for the current request
	json::element get_credentials() const;

	/// \brief get the remote client address for the current request
	std::string get_remote_address() const;

	/// \brief returns whether the current user has role \a role
	bool has_role(const std::string &role) const;

	/// \brief return a specific header line from the original request
	std::string get_header(const char *name) const;

	/// \brief Fill in the OPTIONS in reply \a rep for a request \a req
	virtual void get_options(const request &req, reply &rep);

  protected:
	controller(const controller &) = delete;
	controller &operator=(const controller &) = delete;

	uri m_prefix_path;
	basic_server *m_server = nullptr;
	static thread_local request *s_request;
};

} // namespace zeep::http
