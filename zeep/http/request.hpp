//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_REQUEST_HPP
#define SOAP_HTTP_REQUEST_HPP

#include <vector>

#include <zeep/http/header.hpp>

namespace zeep { namespace http {

enum http_version {
	http_version_1_0,
	http_version_1_1
};

struct request
{
	std::string		method;
	std::string		uri;
	int				http_version_major;
	int				http_version_minor;
	std::vector<header>
					headers;
	std::string		payload;
	
	http_version	version;
	bool			close;
};

}
}

#endif
