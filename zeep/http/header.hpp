//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_HEADER_H
#define SOAP_HTTP_HEADER_H

#include <string>

namespace zeep { namespace http {

/// The header object contains the header lines as found in a
/// HTTP Request. The lines are parsed into name / value pairs.

struct header
{
	std::string	name;
	std::string	value;
};
	
}
}

#endif
