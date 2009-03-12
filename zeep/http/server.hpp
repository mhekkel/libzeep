//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_SERVER_HPP
#define SOAP_HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/noncopyable.hpp>
#include "zeep/http/request_handler.hpp"
#include "zeep/http/reply.hpp"

namespace zeep { namespace http {

class connection;

class server : public request_handler, public boost::noncopyable
{
  public:
	// if nr_of_threads > 0 server object will bind to the
	// specified address and port and will start listening.
	// if nr_of_threads <= 0 server was created in run_worker
						server(const std::string& address,
							short port, int nr_of_threads = 4);

	virtual				~server();

	virtual void		run();

	// The next call will fork a worker instance to do
	// the actual processing and then starts listening
	// the address and port specified.
	template<class Server>
	static void			run_worker(const std::string& address, short port);

	virtual void		stop();

	// to extend the log entry for a current request, use this ostream:
	static std::ostream&
						log();

  protected:

	virtual void		handle_request(const request& req, reply& rep);

  private:
	friend class server_starter;

	void				run_worker(int fd);

	static void			read_socket_from_parent(int fd_socket,
							boost::asio::ip::tcp::socket& socket);

	virtual void		handle_request(boost::asio::ip::tcp::socket& socket,
							const request& req, reply& rep);

	void				handle_accept(const boost::system::error_code& ec);

	boost::asio::io_service			m_io_service;
	boost::shared_ptr<boost::asio::ip::tcp::acceptor>
									m_acceptor;
	boost::thread_group				m_threads;
	boost::shared_ptr<connection>	m_new_connection;
	int								m_nr_of_threads;
};

class server_starter
{
  public:
						server_starter(const std::string& address, short port);

	void				run();
	void				stop();

	template<class Server>
	void				register_constructor()
						{
							m_constructor.reset(new server_constructor<Server>());
						}

  private:

	struct server_constructor_base
	{
		virtual server*	construct(const std::string& address, short port) = 0;
	};

	template<class Server>
	struct server_constructor : public server_constructor_base
	{
		virtual server*	construct(const std::string& address, short port)
						{
							return new Server(address, port);
						}
	};

	static int			fork_worker(const std::string& address, short port);
	
	static void			write_socket_to_worker(int fd_socket,
							boost::asio::ip::tcp::socket& socket);

	void				handle_accept(const boost::system::error_code& ec);

	std::string						m_address;
	short							m_port;
	boost::asio::io_service			m_io_service;
	boost::shared_ptr<boost::asio::ip::tcp::acceptor>
									m_acceptor;
	boost::asio::ip::tcp::socket	m_socket;
	std::auto_ptr<server_constructor_base>
									m_constructor;
	int								m_fd;
};

//// --------------------------------------------------------------------
//// The implementation of the worker invocation
//
//template<class Server>
//void server::run_worker(const std::string& address, short port)
//{
//	// if this call returns, we're in a forked worker child process
//	int fd = fork_worker(address, port);
//
//	// Time to construct the Server object
//	Server srvr(address, port);
//	
//	try
//	{
//		srvr.run_worker(fd);
//	}
//	catch (std::exception& e)
//	{
//		std::cerr << "Exception caught: " << e.what() << std::endl;
//		exit(1);
//	}
//	
//	exit(0);
//}

}
}

#endif
