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
						server(const std::string& ns, const std::string& service,
							const std::string& address, short port,
							int nr_of_threads = 4);

						// if the default is not correct (reverse proxy e.g.)
	void				set_location(const std::string& location)
							{ m_location = location; }

  protected:

	virtual void		handle_request(const http::request& req, http::reply& rep);

  private:
	std::string			m_location;
};
	
}

#endif
