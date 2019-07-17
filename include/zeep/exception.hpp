// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_EXCEPTION_H
#define SOAP_EXCEPTION_H

#include <exception>
#include <string>

namespace zeep
{

/// zeep::exception is a class used to throw zeep exception.

class exception : public std::exception
{
public:
	/// \brief Create an exception with vsprintf like parameters
	exception(const char* message, ...);

	exception(const std::string& message)
		: m_message(message) {}

	virtual ~exception() throw() {}

	virtual const char *
	what() const throw() { return m_message.c_str(); }

protected:
	std::string m_message;
};

} // namespace zeep

#endif
