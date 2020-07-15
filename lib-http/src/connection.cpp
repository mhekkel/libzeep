// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <boost/asio.hpp>

#include <zeep/http/connection.hpp>
#include <zeep/http/server.hpp>
#include <zeep/streambuf.hpp>

namespace zeep::http
{

// Needed for CLang/libc++ on FreeBSD 10
connection* get_pointer(const std::shared_ptr<connection>& p)
{
	return p.get();
}

connection::connection(boost::asio::io_service& service, server& handler)
	: m_socket(service), m_server(handler)
{
}

void connection::start()
{
	m_socket.async_read_some(boost::asio::buffer(m_buffer),
		[self=shared_from_this()](boost::system::error_code ec, size_t bytes_transferred)
			{ self->handle_read(ec, bytes_transferred); });
}

void connection::handle_read(boost::system::error_code ec, size_t bytes_transferred)
{
	if (not ec)
	{
		char_streambuf sb(m_buffer.data(), bytes_transferred);

		auto result = m_request_parser.parse(sb);
		
		auto avail = sb.in_avail();
		if (avail > 0 and avail < bytes_transferred)
		{
			assert(avail + bytes_transferred < m_buffer.size());
			auto used = bytes_transferred - avail;
			std::copy(m_buffer.begin() + used, m_buffer.begin() + bytes_transferred, m_buffer.begin());
		}

		if (result)
		{
			auto req = m_request_parser.get_request();
			req.set_local_endpoint(m_socket);
			m_keep_alive = req.keep_alive();

			m_request_parser.reset();
			
			m_server.handle_request(m_socket, req, m_reply);

			auto buffers = m_reply.to_buffers();

			boost::asio::async_write(m_socket, buffers,
				[self=shared_from_this()](boost::system::error_code ec, size_t bytes_transferred)
					{ self->handle_write(ec, bytes_transferred); });
		}
		else if (not result)
		{
			m_reply = reply::stock_reply(bad_request);

			auto buffers = m_reply.to_buffers();

			boost::asio::async_write(m_socket, buffers,
				[self=shared_from_this()](boost::system::error_code ec, size_t bytes_transferred)
					{ self->handle_write(ec, bytes_transferred); });
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_buffer),
				[self=shared_from_this()](boost::system::error_code ec, size_t bytes_transferred)
					{ self->handle_read(ec, bytes_transferred); });
		}
	}
}

void connection::handle_write(boost::system::error_code ec, size_t bytes_transferred)
{
	if (not ec)
	{
		auto buffers = m_reply.data_to_buffers();
		
		if (not buffers.empty())
		{
			boost::asio::async_write(m_socket, buffers,
				[self=shared_from_this()](boost::system::error_code ec, size_t bytes_transferred)
					{ self->handle_write(ec, bytes_transferred); });
		}
		else if (m_keep_alive)
		{
			m_request_parser.reset();
			m_reply = reply();

			m_socket.async_read_some(boost::asio::buffer(m_buffer),
				[self=shared_from_this()](boost::system::error_code ec, size_t bytes_transferred)
					{ self->handle_read(ec, bytes_transferred); });
		}
		else
			m_socket.close();
	}
}

}
