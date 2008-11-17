#ifndef XML_EXCEPTION_H
#define XML_EXCEPTION_H

#include <exception>
#include <string>

// forward declaration of XML_Parser
struct XML_ParserStruct;
typedef struct XML_ParserStruct *XML_Parser;

namespace soap {

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
