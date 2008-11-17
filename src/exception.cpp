#include <iostream>
#include <sstream>
#include <cstdarg>

#include <expat.h>

#include "xml/exception.hpp"

using namespace std;

namespace xml
{

const char* kXML_Parser_Error_Messages[] = {
	"NONE",
	"NO_MEMORY",
	"SYNTAX",
	"NO_ELEMENTS",
	"INVALID_TOKEN",
	"UNCLOSED_TOKEN",
	"PARTIAL_CHAR",
	"TAG_MISMATCH",
	"DUPLICATE_ATTRIBUTE",
	"JUNK_AFTER_DOC_ELEMENT",
	"PARAM_ENTITY_REF",
	"UNDEFINED_ENTITY",
	"RECURSIVE_ENTITY_REF",
	"ASYNC_ENTITY",
	"BAD_CHAR_REF",
	"BINARY_ENTITY_REF",
	"ATTRIBUTE_EXTERNAL_ENTITY_REF",
	"MISPLACED_XML_PI",
	"UNKNOWN_ENCODING",
	"INCORRECT_ENCODING",
	"UNCLOSED_CDATA_SECTION",
	"EXTERNAL_ENTITY_HANDLING",
	"NOT_STANDALONE",
	"UNEXPECTED_STATE",
	"ENTITY_DECLARED_IN_PE",
	"FEATURE_REQUIRES_XML_DTD",
	"CANT_CHANGE_FEATURE_ONCE_PARSING",
	"UNBOUND_PREFIX",
	"UNDECLARING_PREFIX",
	"INCOMPLETE_PE",
	"XML_DECL",
	"TEXT_DECL",
	"PUBLICID",
	"SUSPENDED",
	"NOT_SUSPENDED",
	"ABORTED",
	"FINISHED",
	"SUSPEND_PE",
	"RESERVED_PREFIX_XML",
	"RESERVED_PREFIX_XMLNS",
	"RESERVED_NAMESPACE_URI",
};
	
exception::exception(
	const char*		message,
	...)
{
	char msg_buffer[1024];
	
	va_list vl;
	va_start(vl, message);
	vsnprintf(msg_buffer, sizeof(msg_buffer), message, vl);
	va_end(vl);

	m_message = msg_buffer;
}

exception::exception(
	XML_Parser		parser)
{
	try
	{
		stringstream s;
		
		XML_Error error = XML_GetErrorCode(parser);
		if (error <= XML_ERROR_RESERVED_NAMESPACE_URI)
			s << kXML_Parser_Error_Messages[error];
		else
			s << "Unknown Expat error code";
	
		s << endl
		  << "Parse error at line " << XML_GetCurrentLineNumber(parser)
		  << " column " << XML_GetCurrentColumnNumber(parser)
		  << ":" << endl;
		 
		int offset = 0, size = 0;
		const char* context = XML_GetInputContext(parser, &offset, &size);
		if (context != NULL)
			s << string(context + offset, size) << endl;
	
		m_message = s.str();
	}
	catch (...)
	{
		m_message = "oeps";
	}
}

}
