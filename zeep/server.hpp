//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_SERVER_HPP
#define SOAP_SERVER_HPP

#include <zeep/config.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/dispatcher.hpp>

namespace zeep {

class server
	: public dispatcher
	, public http::server
{
  public:
					server(const std::string& ns, const std::string& service);

	virtual void	bind(const std::string& address, short port);

					// if the default is not correct (reverse proxy e.g.)
	void			set_location(const std::string& location)
						{ m_location = location; }

  protected:

	virtual void	handle_request(const http::request& req, http::reply& rep);

  private:
	std::string		m_location;
};
	
}

#endif
