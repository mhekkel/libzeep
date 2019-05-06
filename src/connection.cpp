//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <zeep/http/connection.hpp>

using namespace std;

namespace zeep { namespace http {

// Needed for CLang/libc++ on FreeBSD 10
connection* get_pointer(const shared_ptr<connection>& p)
{
	return p.get();
}

connection::connection(boost::asio::io_service& service,
	request_handler& handler)
	: m_socket(service)
	, m_request_handler(handler)
{
}

void connection::start()
{
	m_request = request();	// reset
	
	m_request.local_address =
		boost::lexical_cast<string>(m_socket.local_endpoint().address());
	m_request.local_port = m_socket.local_endpoint().port();
	
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
		boost::tribool result;
		size_t used;

		tie(result, used) = m_request_parser.parse(
			m_request, m_buffer.data(), bytes_transferred);

// #pragma message("Need to fix this, check for used == 0")

		if (result)
		{
			m_reply.set_version(m_request.http_version_major, m_request.http_version_minor);
			
			m_request_handler.handle_request(m_socket, m_request, m_reply);

			vector<boost::asio::const_buffer> buffers;
			m_reply.to_buffers(buffers);

			boost::asio::async_write(m_socket, buffers,
				boost::bind(&connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
		}
		else if (not result)
		{
			m_reply = reply::stock_reply(bad_request);

			vector<boost::asio::const_buffer> buffers;
			m_reply.to_buffers(buffers);

			boost::asio::async_write(m_socket, buffers,
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
		vector<boost::asio::const_buffer> buffers;
		
		if (m_reply.data_to_buffers(buffers))
		{
			boost::asio::async_write(m_socket, buffers,
				boost::bind(&connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
		}
		else if (m_request.http_version_minor >= 1 and not m_request.close)
		{
			m_request_parser.reset();
			m_request = request();
			m_reply = reply();

			m_socket.async_read_some(boost::asio::buffer(m_buffer),
				boost::bind(&connection::handle_read, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
			m_socket.close();
	}
}

}
}
