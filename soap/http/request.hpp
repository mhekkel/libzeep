#ifndef SOAP_HTTP_REQUEST_HPP
#define SOAP_HTTP_REQUEST_HPP

#include <vector>

#include "soap/http/header.hpp"

namespace soap { namespace http {

struct request
{
	std::string		method;
	std::string		uri;
	int				http_version_major;
	int				http_version_minor;
	std::vector<header>
					headers;
	std::string		payload;
};

}
}

#endif
