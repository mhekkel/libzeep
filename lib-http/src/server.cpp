// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <iostream>

#include <zeep/http/connection.hpp>
#include <zeep/http/controller.hpp>
#include <zeep/http/error-handler.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/uri.hpp>

namespace zeep::http
{

namespace detail
{

	// a thread specific logger
	thread_local std::unique_ptr<std::ostringstream> s_log;
	std::mutex s_log_lock;

} // namespace detail

// --------------------------------------------------------------------
// http::basic_server

basic_server::basic_server()
	: m_log_forwarded(true)
	, m_security_context(nullptr)
	, m_allowed_methods{ "GET", "POST", "PUT", "OPTIONS", "HEAD", "DELETE" }
{
	// add a default error handler
	add_error_handler(new default_error_handler());
}

basic_server::basic_server(security_context *s_cntxt)
	: basic_server()
{
	m_security_context.reset(s_cntxt);
}

basic_server::~basic_server()
{
	stop();

	for (auto c : m_controllers)
		delete c;

	for (auto eh : m_error_handlers)
		delete eh;
}

void basic_server::set_template_processor(basic_template_processor *template_processor)
{
	m_template_processor.reset(template_processor);
}

void basic_server::bind(const std::string &address, unsigned short port)
{
	m_address = address;
	m_port = port;

	m_acceptor.reset(new asio_ns::ip::tcp::acceptor(get_io_context()));
	m_new_connection.reset(new connection(get_io_context(), *this));

	// then bind the address here
	asio_ns::ip::tcp::endpoint endpoint;

	asio_system_ns::error_code ec;
	auto addr = asio_ns::ip::make_address(address, ec);
	if (not ec)
		endpoint = asio_ns::ip::tcp::endpoint(addr, port);
	else
	{
		asio_ns::ip::tcp::resolver resolver(get_io_context());
		asio_ns::ip::tcp::resolver::query query(address, std::to_string(port));
		endpoint = *resolver.resolve(query);
	}

	m_acceptor->open(endpoint.protocol());
	m_acceptor->set_option(asio_ns::ip::tcp::acceptor::reuse_address(true));
	m_acceptor->bind(endpoint);
	m_acceptor->listen();
	m_acceptor->async_accept(m_new_connection->get_socket(),
		[this](asio_system_ns::error_code ec)
		{ this->handle_accept(ec); });
}

void basic_server::get_options_for_request(const request &req, reply &rep)
{
	rep = reply::stock_reply(no_content);
	rep.set_header("Allow", join(m_allowed_methods, ","));
	rep.set_header("Cache-Control", "max-age=86400");

	set_access_control_headers(req, rep);
}

void basic_server::set_access_control_headers([[maybe_unused]] const request &req, reply &rep)
{
	if (m_access_control)
		m_access_control->get_access_control_headers(rep);
}

void basic_server::add_controller(controller *c)
{
	m_controllers.push_back(c);
	c->set_server(this);
}

void basic_server::add_error_handler(error_handler *eh)
{
	m_error_handlers.push_front(eh);
	eh->set_server(this);
}

void basic_server::run(int nr_of_threads)
{
	// keep the server at work until we call stop
	auto work = asio_ns::make_work_guard(get_io_context());

	for (int i = 0; i < nr_of_threads; ++i)
		m_threads.emplace_back([this]()
			{ get_io_context().run(); });

	for (auto &t : m_threads)
	{
		if (t.joinable())
			t.join();
	}
}

void basic_server::stop()
{
	m_new_connection.reset();

	if (m_acceptor and m_acceptor->is_open())
		m_acceptor->close();

	m_acceptor.reset();
}

void basic_server::handle_accept(asio_system_ns::error_code ec)
{
	if (not ec)
	{
		m_new_connection->start();
		m_new_connection.reset(new connection(get_io_context(), *this));
		m_acceptor->async_accept(m_new_connection->get_socket(),
			[this](asio_system_ns::error_code ec)
			{ this->handle_accept(ec); });
	}
}

std::ostream &basic_server::get_log()
{
	if (detail::s_log.get() == NULL)
		detail::s_log.reset(new std::ostringstream);
	return *detail::s_log;
}

void basic_server::handle_request(asio_ns::ip::tcp::socket &socket, request &req, reply &rep)
{
	// we're pessimistic
	rep = reply::stock_reply(not_found);

	// set up a logging stream and collect logging information
	detail::s_log.reset(new std::ostringstream);
	auto start = std::chrono::system_clock::now();

	std::string referer("-"), userAgent("-"), accept, client;

	for (const header &h : req.get_headers())
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
			asio_ns::ip::address addr = socket.remote_endpoint().address();
			client = addr.to_string();
		}

		req.set_remote_address(client);

		// shortcut, check for supported method
		auto method = req.get_method();
		if (not(m_allowed_methods.empty() or m_allowed_methods.count(method)))
			throw bad_request;

		std::string csrf;
		bool csrf_is_new = false;

		if (m_security_context)
		{
			m_security_context->validate_request(req);
			std::tie(csrf, csrf_is_new) = m_security_context->get_csrf_token(req);
		}

		// do the actual work.
		bool processed = false;
		for (auto c : m_controllers)
		{
			if (not c->path_matches_prefix(req.get_uri()))
				continue;

			if (c->dispatch_request(socket, req, rep))
			{
				processed = true;
				break;
			}
		}

		if (not processed)
		{
			for (auto eh : m_error_handlers)
			{
				try
				{
					if (eh->create_error_reply(req, not_found, rep))
						break;
				}
				catch (...)
				{
				}
			}
		}

		if (method == "HEAD" or method == "OPTIONS")
			rep.set_content("", rep.get_content_type());
		else if (csrf_is_new)
			rep.set_cookie("csrf-token", csrf, { { "HttpOnly", "" }, { "SameSite", "Lax" }, { "Path", "/" } });

		if (not m_context_name.empty() and
			(rep.get_status() == moved_permanently or rep.get_status() == moved_temporarily))
		{
			auto location = rep.get_header("location");
			if (location.front() == '/')
				rep.set_header("location", m_context_name + location);
		}

		// work around buggy IE... also, using req.accept() doesn't work since it contains */* ... duh
		if (starts_with(rep.get_content_type(), "application/xhtml+xml") and
			not contains(accept, "application/xhtml+xml") and
			contains(userAgent, "MSIE"))
		{
			rep.set_content_type("text/html; charset=utf-8");
		}

		set_access_control_headers(req, rep);
	}
	catch (...)
	{
		auto eptr = std::current_exception();
		for (auto eh : m_error_handlers)
		{
			try
			{
				if (eh->create_error_reply(req, eptr, rep))
					break;
			}
			catch (...)
			{
			}
		}
	}

	log_request(client, req, rep, start, referer, userAgent, detail::s_log->str());
}

void basic_server::log_request(const std::string &client,
	const request &req, const reply &rep,
	const std::chrono::system_clock::time_point &start,
	const std::string &referer, const std::string &userAgent,
	const std::string &entry) noexcept
{
	try
	{
		// protect the output stream from garbled log messages
		std::unique_lock<std::mutex> lock(detail::s_log_lock);

		const std::time_t now_t = std::chrono::system_clock::to_time_t(start);

		auto credentials = req.get_credentials();
		std::string username = credentials.is_object() ? credentials["username"].as<std::string>() : "";
		if (username.empty())
			username = "-";

		const auto &[major, minor] = req.get_version();

		std::cout << client << ' '
				  << "-" << ' '
				  << username << ' '
				  << std::put_time(std::localtime(&now_t), "[%d/%b/%Y:%H:%M:%S %z]") << ' '
				  << '"' << req.get_method() << ' ' << req.get_uri() << ' '
				  << "HTTP/" << major << '.' << minor << "\" "
				  << rep.get_status() << ' '
				  << rep.size() << ' '
				  << '"' << referer << '"' << ' '
				  << '"' << userAgent << '"' << ' ';

		if (entry.empty())
			std::cout << "-" << std::endl;
		else
			std::cout << std::quoted(entry) << std::endl;
	}
	catch (...)
	{
	}
}

} // namespace zeep::http
