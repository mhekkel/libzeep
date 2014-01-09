// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_CONNECTION_HPP
#define SOAP_HTTP_CONNECTION_HPP

//#include <tr1/memory>
#include <boost/tr1/memory.hpp>

#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <zeep/http/message_parser.hpp>
#include <zeep/http/request_handler.hpp>

namespace zeep { namespace http {

/// The HTTP server implementation of libzeep is inspired by the example code
/// as provided by boost::asio. These objects are not to be used directly.

class connection
	: public std::tr1::enable_shared_from_this<connection>
	, public boost::noncopyable
{
  public:
				connection(boost::asio::io_service& service,
					request_handler& handler);

	void		start();

	void		handle_read(const boost::system::error_code& ec,
					size_t bytes_transferred);

	void		handle_write(const boost::system::error_code& ec);

	boost::asio::ip::tcp::socket&
				get_socket()				{ return m_socket; }

  private:
	boost::asio::ip::tcp::socket			m_socket;
	request_parser							m_request_parser;
	request_handler&						m_request_handler;
	boost::array<char,8192>					m_buffer;						
	request									m_request;
	reply									m_reply;
};

}
}

#endif
