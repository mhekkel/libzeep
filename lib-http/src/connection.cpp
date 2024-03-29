// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include "zeep/http/asio.hpp"

#include <zeep/http/connection.hpp>
#include <zeep/http/server.hpp>
#include <zeep/streambuf.hpp>

#include <iostream>

namespace zeep::http
{

// Needed for CLang/libc++ on FreeBSD 10
connection *get_pointer(const std::shared_ptr<connection> &p)
{
	return p.get();
}

connection::connection(asio_ns::io_context &service, basic_server &handler)
	: m_socket(service)
	, m_server(handler)
	, m_bufs(m_buffer.prepare(4096))
{
}

void connection::start()
{
	m_bufs = m_buffer.prepare(4096);
	m_socket.async_read_some(m_bufs,
		[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
		{ self->handle_read(ec, bytes_transferred); });
}

void connection::handle_read(asio_system_ns::error_code ec, size_t bytes_transferred)
{
	if (not ec)
	{
		if (bytes_transferred > 0)
			m_buffer.commit(bytes_transferred);

		try
		{
			auto result = m_request_parser.parse(m_buffer);

			if (result)
			{
				auto req = m_request_parser.get_request();
				req.set_local_endpoint(m_socket);

				m_request_parser.reset();

				m_server.handle_request(m_socket, req, m_reply);

				// by now, a client might have taken over our socket, in that case, simply drop out
				if (not m_socket.is_open())
					return;

				m_reply.set_version(req.get_version());

				if (req.keep_alive())
				{
					m_reply.set_header("Connection", "Keep-Alive");
					m_reply.set_header("Keep-Alive", "timeout=5, max=100");
					m_keep_alive = true;
				}

				if (req.get_version() == std::make_tuple(0, 9))
				{
					auto buffers = asio_ns::buffer(m_reply.get_content());

					asio_ns::async_write(m_socket, buffers,
						[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
						{ self->handle_write(ec, bytes_transferred); });
				}
				else
				{
					auto buffers = m_reply.to_buffers();

					asio_ns::async_write(m_socket, buffers,
						[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
						{ self->handle_write(ec, bytes_transferred); });
				}
			}
			else if (not result)
			{
				m_reply = reply::stock_reply(bad_request);

				auto buffers = m_reply.to_buffers();

				asio_ns::async_write(m_socket, buffers,
					[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
					{ self->handle_write(ec, bytes_transferred); });
			}
			else
			{
				m_bufs = m_buffer.prepare(4096);
				m_socket.async_read_some(m_bufs,
					[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
					{ self->handle_read(ec, bytes_transferred); });
			}
		}
		catch (const uri_parse_error &ex)
		{
			std::clog << "Invalid URI requested\n";
			m_reply = reply::stock_reply(bad_request);

			auto buffers = m_reply.to_buffers();

			asio_ns::async_write(m_socket, buffers,
				[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
				{ self->handle_write(ec, bytes_transferred); });
		}
		catch (const std::exception &ex)
		{
			std::clog << "Internal server error: " << std::quoted(ex.what()) << '\n';
			m_reply = reply::stock_reply(internal_server_error);

			auto buffers = m_reply.to_buffers();

			asio_ns::async_write(m_socket, buffers,
				[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
				{ self->handle_write(ec, bytes_transferred); });
		}
		catch (...)
		{

		}
	}
}

void connection::handle_write(asio_system_ns::error_code ec, size_t /*bytes_transferred*/)
{
	if (not ec)
	{
		auto buffers = m_reply.data_to_buffers();

		if (not buffers.empty())
		{
			asio_ns::async_write(m_socket, buffers,
				[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
				{ self->handle_write(ec, bytes_transferred); });
		}
		else if (m_keep_alive)
		{
			m_request_parser.reset();
			m_reply.reset();

			if (m_buffer.in_avail())
				handle_read({}, 0); // special case
			else
			{
				m_bufs = m_buffer.prepare(4096);
				m_socket.async_read_some(m_bufs,
					[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
					{ self->handle_read(ec, bytes_transferred); });
			}
		}
		else
			m_socket.close();
	}
}

} // namespace zeep::http
