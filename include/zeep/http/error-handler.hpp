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
#include <zeep/http/template-processor.hpp>

namespace zeep::http
{

/// \brief A base class for error-handler classes
///
/// To handle errors decently when there are multiple controllers.

class error_handler
{
  public:
	/// \brief constructor
	///
	/// If \a error_template is not empty, the error handler will try to 
	/// load this XHTML template using the server's template_processor.
	/// If that fails or error_template is empty, a simple stock message
	/// is returned.
	error_handler(const std::string& error_template = "error.xhtml");
	virtual ~error_handler();

	/// \brief set the server object we're bound to
	void set_server(basic_server* s)				{ m_server = s; }

	/// \brief get the server object we're bound to
	basic_server* get_server()					{ return m_server; }

	/// \brief set the server object we're bound to
	const basic_server* get_server() const		{ return m_server; }

	/// \brief Create an error reply for an exception
	///
	/// This function is called by server with the captured exception.
	/// \param req		The request that triggered this call
	/// \param eptr		The captured exception, use std::rethrow_exception to use this
	/// \param reply	Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_error_reply(const request& req, std::exception_ptr eptr, reply& reply);

	/// \brief Create an error reply for the error containing a validation header
	///
	/// When a authentication violation is encountered, this function is called to generate
	/// the appropriate reply.
	/// \param req		The request that triggered this call
	/// \param reply	Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_unauth_reply(const request& req, reply& reply);

	/// \brief Create an error reply for the error
	///
	/// An error should be returned with HTTP status code \a status. This method will create a default error page.
	/// \param req		The request that triggered this call
	/// \param status	The status code, describing the error
	/// \param reply	Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_error_reply(const request& req, status_type status, reply& reply);

	/// \brief Create an error reply for the error with an additional message for the user
	///
	/// An error should be returned with HTTP status code \a status and additional information \a message.
	/// This method will create a default error page.
	/// \param req		The request that triggered this call
	/// \param status	The error that triggered this call
	/// \param message	The message describing the error
	/// \param reply	Write the reply in this object
	/// \return			Return true if the reply was created successfully
	virtual bool create_error_reply(const request& req, status_type status, const std::string& message, reply& reply);

  protected:
	error_handler(const error_handler&) = delete;
	error_handler& operator=(const error_handler&) = delete;

	basic_server*	m_server = nullptr;
	std::string m_error_template;
};

}