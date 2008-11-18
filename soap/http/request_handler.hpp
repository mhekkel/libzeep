#ifndef SOAP_HTTP_REQUEST_HANDLER_HPP
#define SOAP_HTTP_REQUEST_HANDLER_HPP

#include "soap/http/request.hpp"
#include "soap/http/reply.hpp"

namespace soap { namespace http {

class request_handler
{
  public:
	
	virtual void	handle_request(const request& req, reply& reply) = 0;
};

}
}


#endif
