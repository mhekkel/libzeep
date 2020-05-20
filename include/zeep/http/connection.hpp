// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::connection class, that handles HTTP connections

#include <memory>

#include <boost/asio/posix/stream_descriptor.hpp>

#include <zeep/http/message-parser.hpp>

namespace zeep::http
{

class server;

/// The HTTP server implementation of libzeep is inspired by the example code
/// as provided by boost::asio. These objects are not to be used directly.

class connection
	: public std::enable_shared_from_this<connection>
{
  public:
	connection(connection &) = delete;
	connection& operator=(connection &) = delete;

	connection(boost::asio::io_service& service, server& handler);

	void start();
	void handle_read(boost::system::error_code ec, size_t bytes_transferred);
	void handle_write(boost::system::error_code ec, size_t bytes_transferred);

	boost::asio::ip::tcp::socket& get_socket() { return m_socket; }

  private:
	boost::asio::ip::tcp::socket m_socket;
	request_parser m_request_parser;
	server& m_server;
	std::array<char, 8192> m_buffer;
	request m_request;
	reply m_reply;
};

} // namespace zeep::http
