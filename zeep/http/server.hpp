//  Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_SERVER_HPP
#define SOAP_HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <zeep/http/request_handler.hpp>
#include <zeep/http/reply.hpp>

namespace zeep { namespace http {

class connection;

// prototype for often used functions: http::decode_url and http::encode_url
std::string decode_url(const std::string& s);
std::string encode_url(const std::string& s);

class server : public request_handler
{
  public:
						server();

	virtual void		bind(const std::string& address,
							unsigned short port);

	virtual				~server();

	virtual void		run(int nr_of_threads);

	virtual void		stop();

	// to extend the log entry for a current request, use this ostream:
	static std::ostream&
						log();

	std::string			address() const				{ return m_address; }
	unsigned short		port() const				{ return m_port; }

  protected:

	virtual void		handle_request(const request& req, reply& rep);

  private:
	friend class preforked_server_base;

						server(const server&);
	server&				operator=(const server&);

	virtual void		handle_request(boost::asio::ip::tcp::socket& socket,
							const request& req, reply& rep);

	void				handle_accept(const boost::system::error_code& ec);

	boost::asio::io_service&
						get_io_service()				{ return m_io_service; }

	boost::asio::io_service			m_io_service;
	boost::shared_ptr<boost::asio::ip::tcp::acceptor>
									m_acceptor;
	boost::thread_group				m_threads;
	boost::shared_ptr<connection>	m_new_connection;
	std::string						m_address;
	unsigned short					m_port;
};

}
}

#endif
