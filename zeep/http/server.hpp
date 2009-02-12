//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_SERVER_HPP
#define SOAP_HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include "zeep/http/request_handler.hpp"
#include "zeep/http/reply.hpp"

namespace zeep { namespace http {

class connection;

class server : public request_handler
{
  public:
						server(const std::string& address,
							short port, int nr_of_threads = 4);

	virtual				~server();

	virtual void		run();

	virtual void		stop();

	// to extend the log entry for a current request, use this ostream:
	static std::ostream&
						log();

  protected:

	virtual void		handle_request(const request& req, reply& rep);

  private:

	virtual void		handle_request(boost::asio::ip::tcp::socket& socket,
							const request& req, reply& rep);

	void				handle_accept(const boost::system::error_code& ec);

	boost::asio::io_service			m_io_service;
	boost::asio::ip::tcp::acceptor	m_acceptor;
	boost::thread_group				m_threads;
	boost::shared_ptr<connection>	m_new_connection;
	int								m_nr_of_threads;
};

}
}

#endif
