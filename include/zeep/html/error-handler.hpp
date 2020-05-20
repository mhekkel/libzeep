// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// The class defined here derives from http::error_handler and
/// provides a bit nicer formatted replies.

#include <zeep/http/error-handler.hpp>
#include <zeep/html/template-processor.hpp>

namespace zeep::html
{

/// \brief A base class for error-handler classes
///
/// To handle errors decently when there are multiple controllers.

class error_handler : public http::error_handler, public template_processor
{
  public:
	error_handler(const std::string& docroot)
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
	virtual bool create_error_reply(const http::request& req, http::status_type status, const std::string& message, http::reply& reply);
};

}