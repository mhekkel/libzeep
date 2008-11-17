#ifndef SOAP_HTTP_HEADER_H
#define SOAP_HTTP_HEADER_H

#include <string>

namespace soap { namespace http {

struct header
{
	std::string	name;
	std::string	value;
};
	
}
}

#endif
