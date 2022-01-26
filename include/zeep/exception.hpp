// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of zeep::exception, base class for exceptions thrown by libzeep

#include <zeep/config.hpp>

#include <exception>
#include <string>

namespace zeep
{

/// \brief base class of the exceptions thrown by libzeep
class exception : public std::exception
{
  public:
	/// \brief Create an exception with the message in \a message
	exception(const std::string& message)
		: m_message(message) {}

	virtual ~exception() throw() {}

	virtual const char* what() const throw() { return m_message.c_str(); }

  protected:
	std::string m_message;
};

} // namespace zeep
