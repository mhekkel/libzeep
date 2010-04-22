//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_PREFORKED_SERVER_HPP
#define SOAP_HTTP_PREFORKED_SERVER_HPP

#include "zeep/http/server.hpp"
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>

/// preforked server support.
/// A preforked server means you have a master process that listens to a port
/// and whenever a request comes in, the socket is passed to a client. This
/// client will then process the request.
/// This approach has several advantages related to security and stability.
///
/// The way it works in libzeep is, you still create a server that derives
/// from zeep::server (if you need a SOAP server) or zeep::http::server (if
/// you only want a HTTP server). You then create a
/// zeep::http::preforked_server_base<YourServer> instance passing in the
/// parameters required and then call run() on this preforked server.
///
///	The preforked_server class records the way your server needs to be
/// constructed. When this preforked_server is run, it forks and then
/// constructs your server class in the child process.
///
/// Example:
///
///		class my_server {
///						my_server(const string& address, short port, int nr_of_threads,
///							const string& my_param);
///		....
///
///		zeep::http::preforked_server<my_server> server("0.0.0.0", 10333, 2, "my extra param");
///		boost::thread t(
///			boost::bind(&zeep::http::preforked_server<my_server>::run, &server));
///		... // wait for signal to stop
///		server.stop();
///		t.join();

namespace zeep { namespace http {

class preforked_server_base
{
  public:


	virtual					~preforked_server_base();

	virtual void			run();
	virtual void			stop();

  protected:

	struct server_constructor_base
	{
		virtual server*	construct(const std::string& address, short port, int nr_of_threads) = 0;
	};

	template<class Signature>
	struct server_constructor;
	
  							preforked_server_base(const std::string& address, short port, int nr_of_threads,
  								server_constructor_base* constructor)
							: m_address(address)
							, m_port(port)
							, m_nr_of_threads(nr_of_threads)
							, m_constructor(constructor)
							, m_acceptor(m_io_service)
							, m_socket(m_io_service)
						{
						}

  private:
							preforked_server_base(const preforked_server_base&);
	preforked_server_base&	operator=(const preforked_server_base&);

	static bool				read_socket_from_parent(int fd_socket, boost::asio::ip::tcp::socket& socket);
	
	static void				write_socket_to_worker(int fd_socket, boost::asio::ip::tcp::socket& socket);

	void					handle_accept(const boost::system::error_code& ec);

	std::string						m_address;
	short							m_port;
	int								m_nr_of_threads;
	server_constructor_base*		m_constructor;
	boost::asio::io_service			m_io_service;
	boost::asio::ip::tcp::acceptor	m_acceptor;
	boost::asio::ip::tcp::socket	m_socket;
	int								m_fd;
	int								m_pid;
};

template<class Server>
struct preforked_server_base::server_constructor<void(Server::*)()> : public server_constructor_base
{
	virtual server*	construct(const std::string& address, short port, int nr_of_threads)
						{ return new Server(address, port, nr_of_threads); }
};

template<class Server, typename T0>
struct preforked_server_base::server_constructor<void(Server::*)(T0)> : public server_constructor_base
{
						server_constructor(T0 t0)
							: m_t0(t0) {}

	virtual server*	construct(const std::string& address, short port, int nr_of_threads)
						{ return new Server(address, port, nr_of_threads, m_t0); }

  private:
	typedef typename boost::remove_const<
			typename boost::remove_reference<T0>::type>::type	value_type_0;

	value_type_0	m_t0;
};

template<class Server, typename T0, typename T1>
struct preforked_server_base::server_constructor<void(Server::*)(T0,T1)> : public server_constructor_base
{
						server_constructor(T0 t0, T1 t1)
							: m_t0(t0), m_t1(t1) {}

	virtual server*	construct(const std::string& address, short port, int nr_of_threads)
						{ return new Server(address, port, nr_of_threads, m_t0, m_t1); }

  private:
	typedef typename boost::remove_const<
			typename boost::remove_reference<T0>::type>::type	value_type_0;
	typedef typename boost::remove_const<
			typename boost::remove_reference<T1>::type>::type	value_type_1;

	value_type_0	m_t0;
	value_type_1	m_t1;
};

template<class Server>
class preforked_server : public preforked_server_base
{
	typedef	Server									server_type;

  public:

						preforked_server(const std::string& address, short port, int nr_of_threads)
							: preforked_server_base(address, port, nr_of_threads, new server_constructor<void(Server::*)()>())
						{
						}

	template<typename T0>
						preforked_server(const std::string& address, short port, int nr_of_threads, T0 t0)
							: preforked_server_base(address, port, nr_of_threads, new server_constructor<void(Server::*)(T0)>(t0))
						{
						}

	template<typename T0, typename T1>
						preforked_server(const std::string& address, short port, int nr_of_threads, T0 t0, T1 t1)
							: preforked_server_base(address, port, nr_of_threads, new server_constructor<void(Server::*)(T0,T1)>(t0, t1))
						{
						}
};

}
}

#endif
