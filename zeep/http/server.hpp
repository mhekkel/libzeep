//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_SERVER_HPP
#define SOAP_HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/noncopyable.hpp>

#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/accumulate.hpp>

#include "zeep/http/request_handler.hpp"
#include "zeep/http/reply.hpp"

namespace f = boost::fusion;

namespace zeep { namespace http {

class connection;

class server : public request_handler, public boost::noncopyable
{
  public:
						server(const std::string& address, short port);

	virtual				~server();

	virtual void		run(const std::string& address, short port, int nr_of_threads);

	virtual void		stop();

	// to extend the log entry for a current request, use this ostream:
	static std::ostream&
						log();

  protected:

	virtual void		handle_request(const request& req, reply& rep);

  private:
	friend class server_starter;

	void				run_worker(int fd, int nr_of_threads);

	static bool			read_socket_from_parent(int fd_socket,
							boost::asio::ip::tcp::socket& socket);

	virtual void		handle_request(boost::asio::ip::tcp::socket& socket,
							const request& req, reply& rep);

	void				handle_accept(const boost::system::error_code& ec);

	boost::asio::io_service			m_io_service;
	boost::shared_ptr<boost::asio::ip::tcp::acceptor>
									m_acceptor;
	boost::thread_group				m_threads;
	boost::shared_ptr<connection>	m_new_connection;
};

class server_starter
{
  public:
	
	template<class Server>
	static server_starter*
						create(const std::string& address, short port, bool preforked, int nr_of_threads)
						{
							server_starter* starter = new server_starter(address, port, preforked, nr_of_threads);
							starter->m_constructor = new server_constructor<void(Server::*)()>();
							return starter;
						}
	
	template<class Server, typename T0>
	static server_starter*
						create(const std::string& address, short port, bool preforked, int nr_of_threads, T0 t0)
						{
							server_starter* starter = new server_starter(address, port, preforked, nr_of_threads);
							starter->m_constructor = new server_constructor<void(Server::*)(T0)>(t0);
							return starter;
						}

	template<class Server, typename T0, typename T1>
	static server_starter*
						create(const std::string& address, short port, bool preforked, int nr_of_threads, T0 t0, T1 t1)
						{
							server_starter* starter = new server_starter(address, port, preforked, nr_of_threads);
							starter->m_constructor = new server_constructor<void(Server::*)(T0,T1)>(t0, t1);
							return starter;
						}

	virtual				~server_starter();

	void				run();

	// for pre-forked servers you have to call start_listening
	// when done with initialising the application.
	virtual void		start_listening();

	void				stop();

  private:

						server_starter(const std::string& address, short port, bool preforked, int nr_of_threads);

	struct server_constructor_base
	{
		virtual server*	construct(const std::string& address, short port) = 0;
	};
	
	template<class Signature>
	struct server_constructor;

	template<class Server>
	struct server_constructor<void(Server::*)()> : public server_constructor_base
	{
		virtual server*	construct(const std::string& address, short port)
							{ return new Server(address, port); }
	};

	template<class Server, typename T0>
	struct server_constructor<void(Server::*)(T0)> : public server_constructor_base
	{
		typedef typename boost::remove_const<typename boost::remove_reference<T0>::type>::type t_0;
		typedef typename f::vector<t_0>	argument_type;

		argument_type	arguments;

						server_constructor(T0 t0) : arguments(t0) {}
		
		virtual server*	construct(const std::string& address, short port)
							{ return new Server(address, port, f::at_c<0>(arguments)); }
	};

	template<class Server, typename T0, typename T1>
	struct server_constructor<void(Server::*)(T0, T1)> : public server_constructor_base
	{
		typedef typename boost::remove_const<typename boost::remove_reference<T0>::type>::type t_0;
		typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type t_1;
		typedef typename f::vector<t_0,t_1>	argument_type;

		argument_type	arguments;

						server_constructor(T0 t0, T1 t1) : arguments(t0, t1) {}
		
		virtual server*	construct(const std::string& address, short port)
							{ return new Server(address, port, f::at_c<0>(arguments), f::at_c<1>(arguments)); }
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
	server_constructor_base*		m_constructor;
	int								m_fd;
	int								m_pid;
	int								m_nr_of_threads;
	bool							m_preforked;
	server*							m_server;
	boost::mutex					m_startup_lock;
};

}
}

#endif
