//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the base class zeep::error_handler, the default 
/// creates very simple HTTP replies. Override to do something more fancy.

#include <zeep/config.hpp>

#include <zeep/http/server.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/http/template-processor.hpp>

namespace zeep::http
{

/// \brief A base class for error-handler classes
///
/// To handle errors decently when there are multiple controllers.

class error_handler
{
  public:
	error_handler();
	virtual ~error_handler();

	/// \brief set the server object we're bound to
	void set_server(server* s)				{ m_server = s; }

	/// \brief get the server object we're bound to
	server* get_server()					{ return m_server; }

	/// \brief set the server object we're bound to
	const server* get_server() const		{ return m_server; }

	/// \brief Create an error reply for an exception
	///
	/// This function is called by server with the captured exception.
	/// \param req		The request that triggered this call
	/// \param eptr		The captured exception, use std::rethrow_exception to use this
	/// \param rep		Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_error_reply(const request& req, std::exception_ptr eptr, reply& reply);

	/// \brief Create an error reply for the error containing a validation header
	///
	/// When a authentication violation is encountered, this function is called to generate
	/// the appropriate reply.
	/// \param req		The request that triggered this call
	/// \param realm	The name of the protected area, might be shown to the user
	/// \param rep		Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_unauth_reply(const request& req, const std::string& realm, reply& reply);

	/// \brief Create an error reply for the error
	///
	/// An error should be returned with HTTP status code \a status. This method will create a default error page.
	/// \param req		The request that triggered this call
	/// \param realm	The name of the protected area, might be shown to the user
	/// \param rep		Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_error_reply(const request& req, status_type status, reply& reply);

	/// \brief Create an error reply for the error with an additional message for the user
	///
	/// An error should be returned with HTTP status code \a status and additional information \a message.
	/// This method will create a default error page.
	/// \param req		The request that triggered this call
	/// \param status	The error that triggered this call
	/// \param message	The message describing the error
	/// \param rep		Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_error_reply(const request& req, status_type status, const std::string& message, reply& reply);

  protected:
	error_handler(const error_handler&) = delete;
	error_handler& operator=(const error_handler&) = delete;

	server*	m_server = nullptr;
};

/// \brief A base class for error-handler classes
///
/// This version of the error handler tries to load a reource called error.xhtml

class html_error_handler : public error_handler, public template_processor
{
  public:
	html_error_handler(const std::string& docroot)
		: template_processor(docroot) {}

	/// \brief Create an error reply for the error with an additional message for the user
	///
	/// An error should be returned with HTTP status code \a status and additional information \a message.
	/// This method will create a default error page.
	/// \param req		The request that triggered this call
	/// \param status	The error that triggered this call
	/// \param message	The message describing the error
	/// \param rep		Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_error_reply(const request& req, status_type status, const std::string& message, reply& reply);
};


}