// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/connection.hpp"
#include "zeep/http/asio.hpp"
#include "zeep/http/message-parser.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/server.hpp"
#include "zeep/uri.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace zeep::http
{

std::vector<asio_ns::const_buffer> get_buffers(reply &rep)
{
	std::vector<asio_ns::const_buffer> result;
	for (auto &buffer : rep.to_buffers())
		result.emplace_back(buffer.data(), buffer.size());
	return result;
}

std::vector<asio_ns::const_buffer> get_data_buffers(reply &rep)
{
	std::vector<asio_ns::const_buffer> result;
	for (auto &buffer : rep.data_to_buffers())
		result.emplace_back(buffer.data(), buffer.size());
	return result;
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
				req.set_local_endpoint(
					m_socket.local_endpoint().address().to_string(),
					m_socket.local_endpoint().port());

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
					auto buffers = get_buffers(m_reply);

					asio_ns::async_write(m_socket, buffers,
						[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
						{ self->handle_write(ec, bytes_transferred); });
				}
		}
		else if (not result)
		{
			m_reply = reply::stock_reply(status_type::bad_request);

			auto buffers = get_buffers(m_reply);

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
		m_reply = reply::stock_reply(status_type::bad_request);

		auto buffers = get_buffers(m_reply);

		asio_ns::async_write(m_socket, buffers,
			[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
			{ self->handle_write(ec, bytes_transferred); });
	}
	catch (const std::exception &ex)
	{
		std::clog << "Internal server error: " << std::quoted(ex.what()) << '\n';
		m_reply = reply::stock_reply(status_type::internal_server_error);

		auto buffers = get_buffers(m_reply);

		asio_ns::async_write(m_socket, buffers,
			[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
			{ self->handle_write(ec, bytes_transferred); });
	}
	catch (...)
	{
		std::clog << "Internal server error\n";

		m_reply = reply::stock_reply(status_type::internal_server_error);

		auto buffers = get_buffers(m_reply);

		asio_ns::async_write(m_socket, buffers,
			[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
			{ self->handle_write(ec, bytes_transferred); });
	}
}
}

void connection::handle_write(asio_system_ns::error_code ec, size_t /*bytes_transferred*/)
{
	if (not ec)
	{
		auto buffers = get_data_buffers(m_reply);

		if (not buffers.empty())
		{
			asio_ns::async_write(m_socket, buffers,
				[self = shared_from_this()](asio_system_ns::error_code ec, size_t bytes_transferred)
				{ self->handle_write(ec, bytes_transferred); });
		}
		else if (m_keep_alive)
		{
			m_request_parser.reset();
			m_reply = {};

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
