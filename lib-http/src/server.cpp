// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <iostream>
#include <sstream>
#include <locale>
#include <mutex>
#include <thread>

#include <zeep/crypto.hpp>
#include <zeep/exception.hpp>

#include <zeep/http/security.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/error-handler.hpp>
#include <zeep/http/connection.hpp>
#include <zeep/http/controller.hpp>

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
	: m_log_forwarded(false), m_add_csrf_token(false)
	, m_security_context(nullptr)
{
	using namespace boost::local_time;

	local_time_facet* lf(new local_time_facet("[%d/%b/%Y:%H:%M:%S %z]"));
	std::cout.imbue(std::locale(std::cout.getloc(), lf));

	// add a default error handler
	add_error_handler(new error_handler());
}

server::server(security_context* s_cntxt)
	: server()
{
	m_security_context.reset(s_cntxt);
}

server::~server()
{
	stop();

	for (auto c: m_controllers)
		delete c;

	for (auto eh: m_error_handlers)
		delete eh;
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

void server::add_controller(controller* c)
{
	m_controllers.push_back(c);
	c->set_server(this);
}

void server::add_error_handler(error_handler* eh)
{
	m_error_handlers.push_front(eh);
	eh->set_server(this);
}

json::element server::get_credentials(const request& req) const
{
	json::element result;

	if (m_security_context)
		result = m_security_context->get_credentials(req);

	return result;
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

void server::handle_request(boost::asio::ip::tcp::socket& socket, request& req, reply& rep)
{
	using namespace boost::posix_time;

	// we're pessimistic
	rep = reply::stock_reply(not_found);

	// set up a logging stream and collect logging information
	detail::s_log.reset(new std::ostringstream);
	ptime start = second_clock::local_time();
	
	std::string referer("-"), userAgent("-"), accept, client;
	
	for (const header& h: req.headers)
	{
		if (m_log_forwarded and iequals(h.name, "X-Forwarded-For"))
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
		else if (iequals(h.name, "Referer"))
			referer = h.value;
		else if (iequals(h.name, "User-Agent"))
			userAgent = h.value;
		else if (iequals(h.name, "Accept"))
			accept = h.value;
	}
	
	try
	{
		// asking for the remote endpoint address failed sometimes
		// causing aborting exceptions, so I moved it here.
		if (client.empty())
		{
			boost::asio::ip::address addr = socket.remote_endpoint().address();
			client = boost::lexical_cast<std::string>(addr);
		}

		// shortcut, check for supported method
		if (req.method != method_type::GET and req.method != method_type::POST and req.method != method_type::PUT and
			req.method != method_type::OPTIONS and req.method != method_type::HEAD and req.method != method_type::DELETE)
		{
			throw bad_request;
		}

		std::string csrf;
		bool csrf_is_new = false;

		if (m_security_context)
		{
			m_security_context->validate_request(req);
			std::tie(csrf, csrf_is_new) = m_security_context->get_csrf_token(req);
		}

		// parse the uri
		std::string path = req.get_path();

		// do the actual work.
		for (auto c: m_controllers)
		{
			if (not c->path_matches_prefix(path))
				continue;

			if (c->handle_request(req, rep))
				break;
		}
		
		if (req.method == method_type::HEAD)
			rep.set_content("", rep.get_content_type());
		else if (csrf_is_new)
			rep.set_cookie("csrf-token", csrf, {
				{ "HttpOnly", "" },
				{ "SameSite", "Lax" },
				{ "Path", "/" }
			});

		// work around buggy IE... also, using req.accept() doesn't work since it contains */* ... duh
		if (ba::starts_with(rep.get_content_type(), "application/xhtml+xml") and
			not ba::contains(accept, "application/xhtml+xml") and
			ba::contains(userAgent, "MSIE"))
		{
			rep.set_content_type("text/html; charset=utf-8");
		}
	}
	catch (...)
	{
		auto eptr = std::current_exception();
		for (auto eh: m_error_handlers)
		{
			if (eh->create_error_reply(req, eptr, rep))
				break;
		}
	}

	log_request(client, req, rep, start, referer, userAgent, detail::s_log->str());
}

void server::log_request(const std::string& client,
	const request& req, const reply& rep,
	const boost::posix_time::ptime& start,
	const std::string& referer, const std::string& userAgent,
	const std::string& entry) noexcept
{
	try
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
	catch (...) {}
}

}
