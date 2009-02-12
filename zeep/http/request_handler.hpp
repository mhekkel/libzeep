//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_REQUEST_HANDLER_HPP
#define SOAP_HTTP_REQUEST_HANDLER_HPP

#include "zeep/http/request.hpp"
#include "zeep/http/reply.hpp"

namespace zeep { namespace http {

class request_handler
{
  public:
	
	virtual void	handle_request(boost::asio::ip::tcp::socket& socket,
						const request& req, reply& reply) = 0;
};

}
}


#endif
