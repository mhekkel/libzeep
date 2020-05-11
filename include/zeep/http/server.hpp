// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file definition of the zeep::http::server class

#include <thread>

#include <boost/asio.hpp>

#include <zeep/http/request-handler.hpp>
#include <zeep/http/controller.hpp>

namespace zeep::http
{

/// The libzeep HTTP server implementation. Based on code found in boost::asio.

class connection;

class server : public request_handler
{
  public:
	server();

	server(const server&) = delete;
	server& operator=(const server&) = delete;

	/// Add controllers
	void add_controller(controller* c);

	/// Bind the server to address \a address and port \a port
	virtual void bind(const std::string& address,
					  unsigned short port);

	virtual ~server();

	virtual void run(int nr_of_threads);

	virtual void stop();

	/// to extend the log entry for a current request, use this ostream:
	static std::ostream& get_log();

	/// log_forwarded tells the HTTP server to use the last entry in X-Forwarded-For as client log entry
	void set_log_forwarded(bool v) { m_log_forwarded = v; }

	std::string get_address() const { return m_address; }
	unsigned short get_port() const { return m_port; }

	/// get_io_service has to be public since we need it to call notify_fork from child code
	boost::asio::io_service& get_io_service() { return m_io_service; }

  protected:
	virtual void handle_request(request& req, reply& rep);

	/// the default entry logger
	virtual void log_request(const std::string& client,
							 const request& req, const reply& rep,
							 const boost::posix_time::ptime& start,
							 const std::string& referer, const std::string& userAgent,
							 const std::string& entry);

private:
	friend class preforked_server_base;

	virtual void handle_request(boost::asio::ip::tcp::socket& socket,
								request& req, reply& rep);

	void handle_accept(boost::system::error_code ec);

	boost::asio::io_service m_io_service;
	std::shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
	std::list<std::thread> m_threads;
	std::shared_ptr<connection> m_new_connection;
	std::string m_address;
	unsigned short m_port;
	bool m_log_forwarded;
	std::list<controller*> m_controllers;
};

} // namespace zeep::http
