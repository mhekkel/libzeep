//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_EXCEPTION_H
#define SOAP_EXCEPTION_H

#include <exception>
#include <string>

// forward declaration of XML_Parser
struct XML_ParserStruct;
typedef struct XML_ParserStruct *XML_Parser;

namespace zeep {

class exception : public std::exception
{
  public:
				exception(
					const char*		message,
					...);

				exception(
					XML_Parser		parser);

	virtual 	~exception() throw() {}

	virtual const char*
				what() const throw()			{ return m_message.c_str(); }

  private:
	std::string	m_message;
};

}

#endif
