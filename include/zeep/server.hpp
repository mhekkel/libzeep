// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_SERVER_HPP
#define SOAP_SERVER_HPP

#include <zeep/config.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/dispatcher.hpp>

namespace zeep
{

/// zeep::server inherits from zeep::http::server and zeep::dispatcher to do its work.
/// You construct a new server object by passing in a namespace in the \a ns parameter and
/// a service name in the \a service parameter.
///
/// If the server is behind a proxy, you'll also need to call set_location to specify the
/// external address of your server.

class server
	: public dispatcher,
	  public http::server
{
public:
	/// The constructor takes two arguments
	/// \param ns      The namespace as used for the WSDL and SOAP messages
	/// \param service The service name for this server
	server(const std::string& ns, const std::string& service);

	/// \brief Calls zeep::http::server and sets m_location if it wasn't already specified
	virtual void bind(const std::string& address, unsigned short port);

	/// If your server is located behind a reverse proxy, you'll have to specify the address
	/// where it can be found by clients. If you don't, the WSDL will contain an unreachable
	/// address.
	/// \param location The URL that specifies the external address of this server.
	void set_location(const std::string& location)
	{
		m_location = location;
	}

protected:
	virtual void handle_request(const http::request& req, http::reply& rep);

private:
	std::string m_location;
};

} // namespace zeep

#endif
