// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the zeep::http::connection class, that handles HTTP connections

#include "zeep/http/message-parser.hpp"

#include <memory>

namespace zeep::http
{

class basic_server;

/// \brief Manages an individual HTTP connection
///
/// The HTTP server implementation of libzeep is inspired by the example code
/// as provided by boost::asio. Each \a connection object manages the lifecycle
/// of a single TCP connection, including reading the HTTP request, dispatching
/// it to the server, and writing the response. These objects are not to be used
/// directly.

class connection
	: public std::enable_shared_from_this<connection>
{
  public:
	connection(connection &) = delete;
	connection &operator=(connection &) = delete;

	/// \brief Construct a new connection
	/// \param service  The io_context that will handle async I/O for this connection
	/// \param handler  The server that will process incoming requests
	connection(asio_ns::io_context &service, basic_server &handler);

	/// \brief Start reading the HTTP request from the socket
	void start();

	/// \brief Callback invoked when data has been read from the socket
	/// \param ec                The result of the read operation
	/// \param bytes_transferred The number of bytes read
	void handle_read(asio_system_ns::error_code ec, size_t bytes_transferred);

	/// \brief Callback invoked when the reply has been written to the socket
	/// \param ec                The result of the write operation
	/// \param bytes_transferred The number of bytes written
	void handle_write(asio_system_ns::error_code ec, size_t bytes_transferred);

	/// \brief Return the underlying TCP socket
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
