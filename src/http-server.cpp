// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <iostream>
#include <sstream>
#include <locale>

#include <zeep/http/server.hpp>
#include <zeep/http/connection.hpp>
#include <zeep/exception.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>

using namespace std;
namespace ba = boost::algorithm;

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

server::server()
{
}

void server::bind(const std::string& address, unsigned short port)
{
	m_address = address;
	m_port = port;
	
	m_acceptor.reset(new boost::asio::ip::tcp::acceptor(m_io_service));
	m_new_connection.reset(new connection(m_io_service, *this));

	boost::asio::ip::tcp::resolver resolver(m_io_service);
	boost::asio::ip::tcp::resolver::query query(address, boost::lexical_cast<string>(port));
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

	m_acceptor->open(endpoint.protocol());
	m_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	m_acceptor->bind(endpoint);
	m_acceptor->listen();
	m_acceptor->async_accept(m_new_connection->get_socket(),
		boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
}

server::~server()
{
	stop();
}

void server::run(int nr_of_threads)
{
	// keep the server at work until we call stop
	boost::asio::io_service::work work(m_io_service);

	for (int i = 0; i < nr_of_threads; ++i)
	{
		m_threads.create_thread(
			boost::bind(&boost::asio::io_service::run, &m_io_service));
	}

	m_threads.join_all();
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
	
	string referer("-"), userAgent("-"), accept, host;
	foreach (const header& h, req.headers)
	{
		if (h.name == "Host")
			host = h.value;
		else if (h.name == "Referer")
			referer = h.value;
		else if (h.name == "User-Agent")
			userAgent = h.value;
		else if (h.name == "Accept")
			accept = h.value;
	}
	
	try
	{
		// asking for the remote endpoint address failed
		// sometimes causing aborting exceptions, so I moved it here.
		addr = socket.remote_endpoint().address();
		
		// some checks (we're a http/1.1 server, right?)
		
		if (host.empty())
			rep = reply::stock_reply(bad_request);
		else
			// do the actual work.
			handle_request(req, rep);
		
		// work around buggy IE...
		if (ba::starts_with(rep.get_content_type(), "application/xhtml+xml") and
			not ba::contains(accept, "application/xhtml+xml") and
			ba::contains(userAgent, "MSIE"))
		{
			rep.set_content_type("text/html; charset=utf-8");
		}
	}
	catch (...)
	{
		rep = reply::stock_reply(internal_server_error);
	}

	try
	{
		// protect the output stream from garbled log messages
		boost::mutex::scoped_lock lock(detail::s_log_lock);
		
		using namespace boost::local_time;

		local_time_facet* lf(new local_time_facet("[%d/%b/%Y:%H:%M:%S %z]"));
		cout.imbue(std::locale(std::cout.getloc(), lf));

		local_date_time start_local(start, time_zone_ptr());
		
		cout << addr << ' '
			 << "-" << ' '
			 << "-" << ' '
			 << start_local << ' '
			 << '"' << req.method << ' ' << req.uri << ' '
			 		<< "HTTP/" << req.http_version_major << '.' << req.http_version_minor << "\" "
			 << rep.get_status() << ' '
			 << rep.get_size() << ' '
			 << '"' << referer << '"' << ' '
			 << '"' << userAgent << '"' << ' '
			 << detail::s_log->str() << endl;
	}
	catch (...) {}
}

}
}
