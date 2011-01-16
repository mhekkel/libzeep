//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <zeep/http/connection.hpp>

namespace zeep { namespace http {

connection::connection(boost::asio::io_service& service,
	request_handler& handler)
	: m_socket(service)
	, m_request_handler(handler)
{
}

void connection::start()
{
	m_socket.async_read_some(boost::asio::buffer(m_buffer),
		boost::bind(&connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void connection::handle_read(
	const boost::system::error_code& ec, size_t bytes_transferred)
{
	if (not ec)
	{
		boost::tribool result = m_request_parser.parse(
			m_request, m_buffer.data(), bytes_transferred);
		
		if (result)
		{
			m_request_handler.handle_request(m_socket, m_request, m_reply);
			boost::asio::async_write(m_socket, m_reply.to_buffers(),
				boost::bind(&connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
		}
		else if (not result)
		{
			m_reply = reply::stock_reply(bad_request);
			boost::asio::async_write(m_socket, m_reply.to_buffers(),
				boost::bind(&connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_buffer),
				boost::bind(&connection::handle_read, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
}

void connection::handle_write(const boost::system::error_code& ec)
{
	if (not ec)
	{
		boost::system::error_code ignored_ec;
		m_socket.close();
	}
}

}
}
