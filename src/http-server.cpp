//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <iostream>
#include <sstream>
#include <locale>

#include <zeep/http/server.hpp>
#include <zeep/http/connection.hpp>
#include <zeep/exception.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

using namespace std;

namespace zeep { namespace http {

namespace detail {

// a thread specific logger

boost::thread_specific_ptr<ostringstream>	s_log;
boost::mutex								s_log_lock;

}

// --------------------------------------------------------------------
// decode_url function

string decode_url(const string& s)
{
	string result;
	
	for (string::const_iterator c = s.begin(); c != s.end(); ++c)
	{
		if (*c == '%')
		{
			if (s.end() - c >= 3)
			{
				int value;
				string s2(c + 1, c + 3);
				istringstream is(s2);
				if (is >> std::hex >> value)
				{
					result += static_cast<char>(value);
					c += 2;
				}
			}
		}
		else if (*c == '+')
			result += ' ';
		else
			result += *c;
	}
	return result;
}

// --------------------------------------------------------------------
// encode_url function

string encode_url(const string& s)
{
	const unsigned char kURLAcceptable[96] =
	{/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	    0,0,0,0,0,0,0,0,0,0,7,6,0,7,7,4,		/* 2x   !"#$%&'()*+,-./	 */
	    7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,		/* 3x  0123456789:;<=>?	 */
	    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 4x  @ABCDEFGHIJKLMNO  */
	    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,		/* 5X  PQRSTUVWXYZ[\]^_	 */
	    0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 6x  `abcdefghijklmno	 */
	    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0			/* 7X  pqrstuvwxyz{\}~	DEL */
	};

	const char kHex[] = "0123456789abcdef";

	string result;
	
	for (string::const_iterator c = s.begin(); c != s.end(); ++c)
	{
		unsigned char a = (unsigned char)*c;
		if (not (a >= 32 and a < 128 and (kURLAcceptable[a - 32] & 4)))
		{
			result += '%';
			result += kHex[a >> 4];
			result += kHex[a & 15];
		}
		else
			result += *c;
	}

	return result;
}

// --------------------------------------------------------------------
// http::server

server::server(const std::string& address, short port, int nr_of_threads)
	: m_address(address)
	, m_port(port)
	, m_nr_of_threads(nr_of_threads)
{
}

server::~server()
{
	stop();
}

void server::run()
{
	assert(not m_acceptor);

	if (m_nr_of_threads > 0)
	{
		m_acceptor.reset(new boost::asio::ip::tcp::acceptor(m_io_service));
		m_new_connection.reset(new connection(m_io_service, *this));
	
		boost::asio::ip::tcp::resolver resolver(m_io_service);
		boost::asio::ip::tcp::resolver::query query(m_address, boost::lexical_cast<string>(m_port));
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	
		m_acceptor->open(endpoint.protocol());
		m_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		m_acceptor->bind(endpoint);
		m_acceptor->listen();
		m_acceptor->async_accept(m_new_connection->get_socket(),
			boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
	}
	else
		m_nr_of_threads = -m_nr_of_threads;

	if (m_nr_of_threads > 0)
	{
		// keep the server at work until we call stop
		boost::asio::io_service::work work(m_io_service);
	
		for (int i = 0; i < m_nr_of_threads; ++i)
		{
			m_threads.create_thread(
				boost::bind(&boost::asio::io_service::run, &m_io_service));
		}
	
		m_threads.join_all();
	}
}

void server::stop()
{
	m_io_service.stop();
}

void server::handle_accept(const boost::system::error_code& ec)
{
	if (not ec)
	{
		m_new_connection->start();
		m_new_connection.reset(new connection(m_io_service, *this));
		m_acceptor->async_accept(m_new_connection->get_socket(),
			boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
	}
}

ostream& server::log()
{
	if (detail::s_log.get() == NULL)
		detail::s_log.reset(new ostringstream);
	return *detail::s_log;
}

void server::handle_request(const request& req, reply& rep)
{
	log() << req.uri;
	rep = reply::stock_reply(not_found);
}

void server::handle_request(boost::asio::ip::tcp::socket& socket,
	const request& req, reply& rep)
{
	using namespace boost::posix_time;
	
	detail::s_log.reset(new ostringstream);
	ptime start = second_clock::local_time();
	
	boost::asio::ip::address addr;
	
	string referer("-"), userAgent("-");
	foreach (const header& h, req.headers)
	{
		if (h.name == "Referer")
			referer = h.value;
		else if (h.name == "User-Agent")
			userAgent = h.value;
	}
	
	try
	{
		// asking for the remote endpoint address failed
		// sometimes causing aborting exceptions, so I moved it here.
		addr = socket.remote_endpoint().address();
		
		// do the actual work.
		handle_request(req, rep);
	}
	catch (...)
	{
		rep = reply::stock_reply(internal_server_error);
	}

	try
	{
		// protect the output stream from garbled log messages
		boost::mutex::scoped_lock lock(detail::s_log_lock);
		cout << addr
			 << " [" << start << "] "
			 << second_clock::local_time() - start << ' '
			 << '"' << req.method << ' ' << req.uri << ' '
			 		<< "HTTP/" << req.http_version_major << '.' << req.http_version_minor << "\" "
			 << rep.status << ' '
			 << rep.get_size() << ' '
			 << '"' << referer << '"' << ' '
			 << '"' << userAgent << '"' << ' '
			 << detail::s_log->str() << endl;
	}
	catch (...) {}
}

}
}
