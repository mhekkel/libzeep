// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// Code for a preforked http server implementation

#include <zeep/config.hpp>

#include <zeep/http/server.hpp>

#if HTTP_SERVER_HAS_PREFORK

namespace zeep::http
{

/// \brief class to create a preforked HTTP server
///
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
/// \code{.cpp}
///		class my_server {
///     public:
///		  my_server(const string& my_param);
///
///		....
///
///		zeep::http::preforked_server<my_server> server(
///         []() { return new my_server("my param value"); }
///     );
///
///     // all addresses, port 10333 and two listener threads
///		std::thread t(std::bind(&zeep::http::preforked_server::run, &server, "0.0.0.0", 10333, 2));
///
///		... // wait for signal to stop
///     
///		server.stop();
///		t.join();
/// \endcode

class preforked_server
{
  public:
    preforked_server(const preforked_server&) = delete;
    preforked_server& operator=(const preforked_server&) = delete;

    /// \brief constructor
    ///
    /// The constructor takes one argument, a function object that creates 
    /// a server class instance.
    preforked_server(std::function<server*(void)> server_factory);
    virtual ~preforked_server();

    /// \brief forks child and starts listening, should be a separate thread
    virtual void run(const std::string& address, short port, int nr_of_threads);
    virtual void start();			///< signal the thread it can start listening:
    virtual void stop();			///< stop the running thread

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
    std::mutex						m_lock;
};

}

#endif