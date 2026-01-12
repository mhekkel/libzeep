// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2025
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/http/server.hpp"

#include "zeep/el/object.hpp"
#include "zeep/el/processing.hpp"
#include "zeep/http/access-control.hpp"
#include "zeep/http/connection.hpp"
#include "zeep/http/controller.hpp"
#include "zeep/http/error-handler.hpp"
#include "zeep/http/header.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/security.hpp"
#include "zeep/http/status.hpp"
#include "zeep/http/template-processor.hpp"
#include "zeep/http/uri.hpp"
#include "zeep/unicode-support.hpp"

#include <chrono>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <list> // for list
#include <memory>
#include <new>
#include <set> // for set
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple> // for tie

namespace zeep::http
{

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
	try
	{
		basic_server::stop();
	}
	catch (const std::exception &ex)
	{
		std::clog << "error stopping server: " << ex.what() << '\n';
	}

	for (auto c : m_controllers)
		delete c;

	for (auto eh : m_error_handlers)
		delete eh;
}

void basic_server::set_template_processor(basic_template_processor *template_processor)
{
	m_template_processor.reset(template_processor);
}

void basic_server::bind(std::string_view address, unsigned short port)
{
	m_address = address;
	m_port = port;

	m_acceptor = std::make_shared<asio_ns::ip::tcp::acceptor>(get_io_context());
	m_new_connection = std::make_shared<connection>(get_io_context(), *this);

	// then bind the address here
	asio_ns::ip::tcp::endpoint endpoint;

	asio_system_ns::error_code ec;
	auto addr = asio_ns::ip::make_address(address, ec);
	if (not ec)
		endpoint = asio_ns::ip::tcp::endpoint(addr, port);
	else
	{
		asio_ns::ip::tcp::resolver resolver(get_io_context());
		for (auto &ep : resolver.resolve(address, std::to_string(port)))
		{
			endpoint = ep;
			break;
		}
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
	rep = reply::stock_reply(status_type::no_content);
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
		m_new_connection = std::make_shared<connection>(get_io_context(), *this);
		m_acceptor->async_accept(m_new_connection->get_socket(),
			[this](asio_system_ns::error_code ec)
			{ this->handle_accept(ec); });
	}
}

void basic_server::handle_request(asio_ns::ip::tcp::socket &socket, request &req, reply &rep)
{
	// we're pessimistic
	rep = reply::stock_reply(status_type::not_found);

	// set up a logging stream and collect logging information
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
			throw http_status_exception(status_type::bad_request);

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
					if (eh->create_error_reply(req, status_type::not_found, rep))
						break;
				}
				catch (...)
				{
					continue;
				}
			}
		}

		if (method == "HEAD" or method == "OPTIONS")
			rep.set_content("", rep.get_content_type());
		else if (csrf_is_new)
			rep.set_cookie("csrf-token", csrf, { { "HttpOnly", "" }, { "SameSite", "Lax" }, { "Path", "/" } });

		if (not m_context_name.empty() and
			(rep.get_status() == status_type::moved_permanently or rep.get_status() == status_type::moved_temporarily))
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
		bool handled = false;

		// special case, caller expects a JSON reply
		if (req.get_accept("application/json") == 1.0f)
		{
			try
			{
				if (eptr)
					std::rethrow_exception(eptr);
			}
			catch (const http_status_exception &ex)
			{
				rep = http::reply::stock_reply(ex.status());

				object error({ { "error", get_status_description(ex.status()) } });
				rep.set_content(error);
				rep.set_status(ex.status());

				handled = true;
			}
			catch (status_type s)
			{
				rep = http::reply::stock_reply(s);

				object error({ { "error", get_status_description(s) } });
				rep.set_content(error);
				rep.set_status(s);

				handled = true;
			}
			catch (const std::exception &e)
			{
				rep = http::reply::stock_reply(status_type::internal_server_error);

				object error({ { "error", e.what() } });
				rep.set_content(error);
				rep.set_status(status_type::internal_server_error);

				handled = true;
			}
			catch (...)
			{
				rep = http::reply::stock_reply(status_type::internal_server_error);

				object error({ { "error", "unknown error" } });
				rep.set_content(error);
				rep.set_status(status_type::internal_server_error);

				handled = true;
			}
		}
		else
		{
			for (auto eh : m_error_handlers)
			{
				try
				{
					if (eh->create_error_reply(req, eptr, rep))
						break;
				}
				catch (...)
				{
					continue;
				}
			}
		}
	}

	log_request(client, req, rep, start, referer, userAgent, {});
}

void basic_server::log_request(std::string_view client,
	const request &req, const reply &rep,
	std::chrono::system_clock::time_point start,
	std::string_view referer, std::string_view userAgent,
	std::string_view entry) noexcept
{
	try
	{
		auto credentials = req.get_credentials();
		std::string username = credentials.is_object() ? credentials["username"].get<std::string>() : "";
		if (username.empty())
			username = "-";

		const auto &[major, minor] = req.get_version();

		std::cout << std::format(R"({} - {} [{:%d/%b/%Y:%H:%M:%S}] "{} {} HTTP/{}.{}" {} {} "{}" "{}"{})", 
			client,
			username,
			std::chrono::current_zone()->to_local(start),
			req.get_method(),
			req.get_uri().string(),
			major,
			minor,
			static_cast<int>(rep.get_status()),
			rep.size(),
			referer,
			userAgent,
			entry.empty() ? std::string{} : ( (std::ostringstream() << ' ' << std::quoted(entry)).str() )
		);
	}
	catch (const std::exception &ex)
	{
		std::cerr << "error writing to log: " << ex.what() << '\n';
	}
}

} // namespace zeep::http
