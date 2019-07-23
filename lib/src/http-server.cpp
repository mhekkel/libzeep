// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
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
// #include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace ba = boost::algorithm;

namespace zeep { namespace http {

namespace detail {

// a thread specific logger

boost::thread_specific_ptr<std::ostringstream>	s_log;
boost::mutex								s_log_lock;

}

// --------------------------------------------------------------------
// decode_url function

std::string decode_url(const std::string& s)
{
	std::string result;
	
	for (std::string::const_iterator c = s.begin(); c != s.end(); ++c)
	{
		if (*c == '%')
		{
			if (s.end() - c >= 3)
			{
				int value;
				std::string s2(c + 1, c + 3);
				std::istringstream is(s2);
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

std::string encode_url(const std::string& s)
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

	std::string result;
	
	for (std::string::const_iterator c = s.begin(); c != s.end(); ++c)
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
	: m_log_forwarded(false)
{
	using namespace boost::local_time;

	local_time_facet* lf(new local_time_facet("[%d/%b/%Y:%H:%M:%S %z]"));
	std::cout.imbue(std::locale(std::cout.getloc(), lf));
}

void server::bind(const std::string& address, unsigned short port)
{
	m_address = address;
	m_port = port;
	
	m_acceptor.reset(new boost::asio::ip::tcp::acceptor(m_io_service));
	m_new_connection.reset(new connection(m_io_service, *this));

	boost::asio::ip::tcp::resolver resolver(m_io_service);
	boost::asio::ip::tcp::resolver::query query(address, boost::lexical_cast<std::string>(port));
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

	m_acceptor->open(endpoint.protocol());
	m_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	m_acceptor->bind(endpoint);
	m_acceptor->listen();
	m_acceptor->async_accept(m_new_connection->get_socket(),
		[this](boost::system::error_code ec) { this->handle_accept(ec); });
}

server::~server()
{
	stop();

	for (auto c: m_controllers)
		delete c;
}

void server::add_controller(controller* c)
{
	m_controllers.push_back(c);
}

void server::run(int nr_of_threads)
{
	// keep the server at work until we call stop
	boost::asio::io_service::work work(m_io_service);

	for (int i = 0; i < nr_of_threads; ++i)
		m_threads.emplace_back([this]() { m_io_service.run(); });

	for (auto& t: m_threads)
	{
		if (t.joinable())
			t.join();
	}
}

void server::stop()
{
	if (m_acceptor and m_acceptor->is_open())
		m_acceptor->close();

	m_io_service.stop();
}

void server::handle_accept(boost::system::error_code ec)
{
	if (not ec)
	{
		m_new_connection->start();
		m_new_connection.reset(new connection(m_io_service, *this));
		m_acceptor->async_accept(m_new_connection->get_socket(),
			[this](boost::system::error_code ec) { this->handle_accept(ec); });
	}
}

std::ostream& server::log()
{
	if (detail::s_log.get() == NULL)
		detail::s_log.reset(new std::ostringstream);
	return *detail::s_log;
}

void server::handle_request(const request& req, reply& rep)
{
	log() << req.uri;
	rep = reply::stock_reply(not_found);
}

void server::handle_request(boost::asio::ip::tcp::socket& socket, const request& req, reply& rep)
{
	using namespace boost::posix_time;
	
	detail::s_log.reset(new std::ostringstream);
	ptime start = second_clock::local_time();
	
	std::string referer("-"), userAgent("-"), accept, client;
	
	for (const header& h: req.headers)
	{
		if (m_log_forwarded and ba::iequals(h.name, "X-Forwarded-For"))
		{
			client = h.value;
			std::string::size_type comma = client.rfind(',');
			if (comma != std::string::npos)
			{
				if (comma < client.length() - 1 and client[comma + 1] == ' ')
					++comma;
				client = client.substr(comma + 1, std::string::npos);
			}
		}
		else if (ba::iequals(h.name, "Referer"))
			referer = h.value;
		else if (ba::iequals(h.name, "User-Agent"))
			userAgent = h.value;
		else if (ba::iequals(h.name, "Accept"))
			accept = h.value;
	}
	
	try		// asking for the remote endpoint address failed sometimes
			// causing aborting exceptions, so I moved it here.
	{
		if (client.empty())
		{
			boost::asio::ip::address addr = socket.remote_endpoint().address();
			client = boost::lexical_cast<std::string>(addr);
		}

		// do the actual work.
		bool handled = false;
		for (auto c: m_controllers)
		{
			if (c->handle_request(req, rep))
			{
				handled = true;
				break;
			}
		}

		if (not handled)
			handle_request(req, rep);
		
		// work around buggy IE... also, using req.accept() doesn't work since it contains */* ... duh
		if (ba::starts_with(rep.get_content_type(), "application/xhtml+xml") and
			not ba::contains(accept, "application/xhtml+xml") and
			ba::contains(userAgent, "MSIE"))
		{
			rep.set_content_type("text/html; charset=utf-8");
		}
	}
	catch (exception& e)
	{
		rep = reply::stock_reply(internal_server_error, e.what());
	}
	catch (...)
	{
		rep = reply::stock_reply(internal_server_error);
	}

	try
	{
		log_request(client, req, rep, start, referer, userAgent, detail::s_log->str());
	}
	catch (...) {}
}

void server::log_request(const std::string& client,
	const request& req, const reply& rep,
	const boost::posix_time::ptime& start,
	const std::string& referer, const std::string& userAgent,
	const std::string& entry)
{
	// protect the output stream from garbled log messages
	boost::unique_lock<boost::mutex> lock(detail::s_log_lock);

	using namespace boost::local_time;

	local_date_time start_local(start, time_zone_ptr());

	std::string username = req.username;
	if (username.empty())
		username = "-";

	std::cout << client << ' '
		 << "-" << ' '
		 << username << ' '
		 << start_local << ' '
		 << '"' << to_string(req.method) << ' ' << req.uri << ' '
				<< "HTTP/" << req.http_version_major << '.' << req.http_version_minor << "\" "
		 << rep.get_status() << ' '
		 << rep.get_size() << ' '
		 << '"' << referer << '"' << ' '
		 << '"' << userAgent << '"' << ' ';
	
	if (entry.empty())
		std::cout << '-' << std::endl;
	else
		std::cout << '"' << entry << '"' << std::endl;
}

}
}
