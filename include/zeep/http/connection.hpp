// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::connection class, that handles HTTP connections

#include <zeep/config.hpp>

#include <memory>

#include "zeep/http/asio.hpp"

#include <zeep/http/message-parser.hpp>

namespace zeep::http
{

class basic_server;

/// The HTTP server implementation of libzeep is inspired by the example code
/// as provided by boost::asio. These objects are not to be used directly.

class connection
	: public std::enable_shared_from_this<connection>
{
  public:
	connection(connection &) = delete;
	connection &operator=(connection &) = delete;

	connection(asio_ns::io_context &service, basic_server &handler);

	void start();
	void handle_read(asio_system_ns::error_code ec, size_t bytes_transferred);
	void handle_write(asio_system_ns::error_code ec, size_t bytes_transferred);

	asio_ns::ip::tcp::socket &get_socket() { return m_socket; }

  private:
	asio_ns::ip::tcp::socket m_socket;
	basic_server &m_server;
	reply m_reply;
	request_parser m_request_parser;
	bool m_keep_alive = false;
	asio_ns::streambuf m_buffer;
	asio_ns::streambuf::mutable_buffers_type m_bufs;
};

} // namespace zeep::http
