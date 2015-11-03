//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_PREFORKED_SERVER_HPP
#define SOAP_HTTP_PREFORKED_SERVER_HPP

#include <zeep/http/server.hpp>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/invoke.hpp>

/// preforked server support.
/// A preforked server means you have a master process that listens to a port
/// and whenever a request comes in, the socket is passed to a client. This
/// client will then process the request.
/// This approach has several advantages related to security and stability.
///
/// The way it works in libzeep is, you still create a server that derives
/// from zeep::server (if you need a SOAP server) or zeep::http::server (if
/// you only want a HTTP server). You then create a
/// zeep::http::preforked_server<YourServer> instance passing in the
/// parameters required and then call run() on this preforked server.
///
///	The preforked_server class records the way your server needs to be
/// constructed. When this preforked_server is run, it forks and then
/// constructs your server class in the child process.
///
/// Example:
///
///		class my_server {
///						my_server(const string& my_param);
///		....
///
///		zeep::http::preforked_server\<my_server\> server(
///         [=]() -> zeep::http::server* { return new my_server("my param value"); }
///     );
///		boost::thread t(
///			server.run("0.0.0.0", 10333, 2);	// all addresses, port 10333 and two listener threads
///
///		... // wait for signal to stop
///     
///		server.stop();
///		t.join();
///

namespace zeep { namespace http {

class preforked_server
{
  public:
	preforked_server(const preforked_server&) = delete;
	preforked_server& operator=(const preforked_server&) = delete;

	preforked_server(std::function<server*(void)> server_factory);
	virtual ~preforked_server();

									/// forks child and starts listening, should be a separate thread
	virtual void run(const std::string& address, short port, int nr_of_threads);
	virtual void start();			///< signal the thread it can start listening:
	virtual void stop();			///< stop the running thread

#ifndef LIBZEEP_DOXYGEN_INVOKED

  private:

	void fork_child();

	static bool read_socket_from_parent(int fd_socket, boost::asio::ip::tcp::socket& socket);
	static void write_socket_to_worker(int fd_socket, boost::asio::ip::tcp::socket& socket);

	void handle_accept(const boost::system::error_code& ec);

	std::function<server*(void)>	m_constructor;
	boost::asio::io_service			m_io_service;
	boost::asio::ip::tcp::acceptor	m_acceptor;
	boost::asio::ip::tcp::socket	m_socket;
	int								m_fd;
	int								m_pid;
	boost::mutex					m_lock;

#endif
};

}
}

#endif
