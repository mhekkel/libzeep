#ifndef XML_EXCEPTION_H
#define XML_EXCEPTION_H

#include <exception>
#include <string>

// forward declaration of XML_Parser
struct XML_ParserStruct;
typedef struct XML_ParserStruct *XML_Parser;

namespace xml
{

class exception : public std::exception
{
  public:
				exception(
					const char*		message);

				exception(
					XML_Parser		parser);

	virtual 	~exception() throw() {}

	virtual const char*
				what() const throw()			{ return message.c_str(); }

  private:
	std::string	message;
};

}

#endif
