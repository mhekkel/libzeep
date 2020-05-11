// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <iostream>
#include <sstream>
#include <locale>
#include <thread>

#include <zeep/http/server.hpp>
#include <zeep/http/connection.hpp>
#include <zeep/exception.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/algorithm/string.hpp>

namespace ba = boost::algorithm;

namespace zeep::http
{

namespace detail
{

// a thread specific logger

thread_local std::unique_ptr<std::ostringstream> s_log;
std::mutex s_log_lock;

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

std::ostream& server::get_log()
{
	if (detail::s_log.get() == NULL)
		detail::s_log.reset(new std::ostringstream);
	return *detail::s_log;
}

void server::handle_request(request& req, reply& rep)
{
	get_log() << req.uri;
	rep = reply::stock_reply(not_found);
}

void server::handle_request(boost::asio::ip::tcp::socket& socket, request& req, reply& rep)
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
		// shortcut, check for supported method
		if (req.method != method_type::GET and req.method != method_type::POST and req.method != method_type::PUT and
			req.method != method_type::OPTIONS and req.method != method_type::HEAD and req.method != method_type::DELETE)
		{
			throw bad_request;
		}

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
	catch (status_type status)
	{
		rep = reply::stock_reply(status);
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
	std::unique_lock<std::mutex> lock(detail::s_log_lock);

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
		 << rep.size() << ' '
		 << '"' << referer << '"' << ' '
		 << '"' << userAgent << '"' << ' ';
	
	if (entry.empty())
		std::cout << '-' << std::endl;
	else
		std::cout << '"' << entry << '"' << std::endl;
}

}
