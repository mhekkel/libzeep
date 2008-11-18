#ifndef SOAP_SERVER_HPP
#define SOAP_SERVER_HPP

#include "soap/http/server.hpp"
#include "soap/http/request.hpp"
#include "soap/http/reply.hpp"
#include "soap/dispatcher.hpp"

namespace soap {

class server
	: public dispatcher
	, public http::server
{
  public:
						server(const std::string& ns,
							const std::string& address,
							short port, int nr_of_threads = 4)
							: dispatcher(ns)
							, http::server(address, port, nr_of_threads) {}

  protected:

	virtual void		handle_request(const http::request& req, http::reply& rep);

};
	
}

#endif
