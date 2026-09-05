// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-License-Identifier: BSL-1.0

#ifndef ZEEP_CXX_MODULE
# include "zeep/http/server.hpp"

# include "zeep/el/object.hpp"
# include "zeep/el/processing.hpp"
# include "zeep/http/access-control.hpp"
# include "zeep/http/connection.hpp"
# include "zeep/http/controller.hpp"
# include "zeep/http/error-handler.hpp"
# include "zeep/http/header.hpp"
# include "zeep/http/reply.hpp"
# include "zeep/http/request.hpp"
# include "zeep/http/security.hpp"
# include "zeep/http/status.hpp"
# include "zeep/http/template-processor.hpp"
# include "zeep/unicode-support.hpp"
# include "zeep/uri.hpp"

# if USE_DATE_H
#  include <date/date.h>
#  include <date/tz.h>
# endif

# include <chrono>
# include <exception>
# include <iomanip>
# include <iostream>
# include <list> // for list
# include <memory>
# include <new>
# include <set> // for set
# include <sstream>
# include <string>
# include <string_view>
# include <thread>
# include <tuple> // for tie
# include <vector>
#endif

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
	m_new_connection = std::make_shared<connection>(get_io_context(), *this, m_max_request_size, m_read_timeout);

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

	// if (m_context_name.empty())
	// 	m_context_name = std::format("http://{}:{}/", address, port);

	m_acceptor->open(endpoint.protocol());
	m_acceptor->set_option(asio_ns::ip::tcp::acceptor::reuse_address(true));
	m_acceptor->bind(endpoint);
	m_acceptor->listen();

	std::scoped_lock lock(m_accept_mutex);
	auto acc = m_acceptor;
	auto conn = m_new_connection;
	acc->async_accept(conn->get_socket(),
		[this, acc, conn](asio_system_ns::error_code ec)
		{ this->handle_accept(ec, acc, conn); });
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

void basic_server::set_context_path(uri context_name) noexcept
{
	m_context_path = std::move(context_name);
	if (m_security_context)
		m_security_context->set_context_path(m_context_path);
}

void basic_server::add_controller(controller *c)
{
	{
		std::scoped_lock lock(m_handlers_mutex);
		m_controllers.emplace_back(c);
	}

	// set_server must be called outside the lock: a controller (e.g. login_controller)
	// may call back into add_error_handler from set_server, which would otherwise
	// deadlock on the non-recursive m_handlers_mutex.
	c->set_server(this);
}

void basic_server::add_error_handler(error_handler *eh)
{
	{
		std::scoped_lock lock(m_handlers_mutex);
		m_error_handlers.emplace_front(eh);
	}

	// See add_controller: set_server may re-enter this method.
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
	std::scoped_lock lock(m_accept_mutex);

	// Close the acceptor first so the pending async_accept completes
	// with operation_aborted. The handler will see the error and
	// return without touching the connection.
	if (m_acceptor and m_acceptor->is_open())
		m_acceptor->close();

	m_new_connection.reset();

	m_acceptor.reset();
}

void basic_server::handle_accept(asio_system_ns::error_code ec,
	std::shared_ptr<asio_ns::ip::tcp::acceptor> acceptor,
	std::shared_ptr<connection> conn)
{
	if (not ec)
	{
		conn->start();

		std::scoped_lock lock(m_accept_mutex);
		auto next = std::make_shared<connection>(get_io_context(), *this, m_max_request_size, m_read_timeout);
		m_new_connection = next;
		acceptor->async_accept(next->get_socket(),
			[this, acceptor, next](asio_system_ns::error_code ec)
			{ this->handle_accept(ec, acceptor, next); });
	}
}

void basic_server::handle_request(asio_ns::ip::tcp::socket &socket, request &req, reply &rep)
{
	// we're pessimistic
	rep = reply::stock_reply(status_type::not_found);

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

		// do the actual work. Take a snapshot of the handler lists under the lock so
		// that a concurrent add_controller/add_error_handler does not race with the
		// iteration. Controllers are only ever appended (never removed), so the raw
		// pointers stay valid for the duration of the snapshot.
		std::vector<controller *> controllers;
		std::vector<error_handler *> error_handlers;
		{
			std::scoped_lock lock(m_handlers_mutex);
			for (auto &c : m_controllers)
				controllers.push_back(c.get());
			for (auto &eh : m_error_handlers)
				error_handlers.push_back(eh.get());
		}

		bool processed = false;
		for (auto *c : controllers)
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
			for (auto *eh : error_handlers)
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
			rep.set_cookie("csrf-token", csrf,
				{ { "HttpOnly", "" },
					{ "Secure", "" },
					{ "SameSite", "Lax" },
					{ "Path", get_context_path() .empty() ? "/" :get_context_path().string() } });

		if (not m_context_path.empty() and
			(rep.get_status() == status_type::moved_permanently or rep.get_status() == status_type::moved_temporarily))
		{
			auto location = rep.get_header("location");
			if (location.starts_with("/"))
				rep.set_header("location", (m_context_path / location).string());
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
			}
			catch (status_type s)
			{
				rep = http::reply::stock_reply(s);

				object error({ { "error", get_status_description(s) } });
				rep.set_content(error);
				rep.set_status(s);
			}
			catch (const std::exception &e)
			{
				rep = http::reply::stock_reply(status_type::internal_server_error);

				object error({ { "error", e.what() } });
				rep.set_content(error);
				rep.set_status(status_type::internal_server_error);
			}
			catch (...)
			{
				rep = http::reply::stock_reply(status_type::internal_server_error);

				object error({ { "error", "unknown error" } });
				rep.set_content(error);
				rep.set_status(status_type::internal_server_error);
			}
		}
		else
		{
			std::scoped_lock lock(m_handlers_mutex);
			for (auto &eh : m_error_handlers)
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

		std::ostringstream ts;

		// macOS still has no zoned time...
#if USE_DATE_H
		auto t = date::make_zoned(date::current_zone(), date::floor<std::chrono::seconds>(start));
		date::to_stream(ts, "%d/%b/%Y:%H:%M:%S %Ez", t);
#else
		auto t = std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(start) };
		ts << std::format("{:%d/%b/%Y:%H:%M:%S %Ez}", t);
#endif

		auto s = std::format(R"({} - {} [{}] "{} {} HTTP/{}.{}" {} {} "{}" "{}"{})",
			client,
			username,
			ts.str(),
			req.get_method(),
			req.get_uri().string(),
			major,
			minor,
			static_cast<int>(rep.get_status()),
			rep.size(),
			referer,
			userAgent,
			entry.empty() ? std::string{} : ((std::ostringstream() << ' ' << std::quoted(entry)).str()));

		// Strip log message from CR and LF
		for (auto p = s.find_first_of("\r\n"); p != std::string::npos; p = s.find_first_of("\r\n", p))
			s[p] = ' ';

		std::cout << s << '\n'
				  << std::flush;
	}
	catch (const std::exception &ex)
	{
		std::println(std::clog, "error writing to log: {}", ex.what());
	}
}

} // namespace zeep::http
