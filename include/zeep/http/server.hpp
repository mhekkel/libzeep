// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::server class

#include <zeep/config.hpp>

#include <thread>
#include <mutex>

#include <boost/asio.hpp>

#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/http/template-processor.hpp>

namespace zeep::http
{

class connection;
class controller;
class security_context;
class error_handler;

/// \brief The libzeep HTTP server implementation. Originally based on example code found in boost::asio.
///
/// The server class is a simple, stand alone HTTP server. Call bind to make it listen to an address/port
/// combination. Add controller classes to do the actual work. These controllers will be tried in the order
/// at which they were added to see if they want to process a request.

class basic_server
{
  public:

	/// \brief Simple server, no security, no template processor
	basic_server();

	/// \brief Simple server, no security, create default template processor with \a docroot
	basic_server(const std::string& docroot)
		: basic_server()
	{
		set_template_processor(new template_processor(docroot));
	}

	/// \brief server with a security context for limited access
	basic_server(security_context* s_ctxt);

	/// \brief server with a security context for limited access, create default template processor with \a docroot
	basic_server(security_context* s_ctxt, const std::string& docroot)
		: basic_server(s_ctxt)
	{
		set_template_processor(new template_processor(docroot));
	}

	basic_server(const basic_server&) = delete;
	basic_server& operator=(const basic_server&) = delete;

	virtual ~basic_server();

	/// \brief Get the security context provided in the constructor
	security_context& get_security_context() 		{ return *m_security_context; }

	/// \brief Test if a security context was provided in the constructor
	bool has_security_context() const				{ return (bool)m_security_context; }

	/// \brief Set the set of allowed methods (default is "GET", "POST", "PUT", "OPTIONS", "HEAD", "DELETE")
	void set_allowed_methods(const std::set<std::string>& methods)
	{
		m_allowed_methods = methods;
	}

	/// \brief Get the set of allowed methods
	std::set<std::string> get_allowed_methods() const
	{
		return m_allowed_methods;
	}

	/// \brief Set the context_name
	///
	/// The context name is used in constructing relative URL's that start with a forward slash
	void set_context_name(const std::string& context_name)			{ m_context_name = context_name; }

	/// \brief Get the context_name
	///
	/// The context name is used in constructing relative URL's that start with a forward slash
	std::string get_context_name() const							{ return m_context_name; }

	/// \brief Add controller to the list of controllers
	///
	/// When a request is received, the list of controllers get a chance
	/// of handling it, in the order of which they were added to this server.
	/// If none of the controller handle the request the not_found error is returned.
	void add_controller(controller* c);

	/// \brief Add an error handler
	///
	/// Errors are handled by the error handler list. The last added handler
	/// is called first.
	void add_error_handler(error_handler* eh);

	/// \brief Set the template processor
	///
	/// A template processor handles loading templates and processing
	/// the contents.
	void set_template_processor(basic_template_processor* template_processor);

	/// \brief Get the template processor
	///
	/// A template processor handles loading templates and processing
	/// the contents. This will throw if the processor has not been set
	/// yet.
	basic_template_processor& get_template_processor()
	{
		if (not m_template_processor)
			throw std::logic_error("Template processor not specified yet");
		return *m_template_processor;
	}

	/// \brief Get the template processor
	///
	/// A template processor handles loading templates and processing
	/// the contents. This will throw if the processor has not been set
	/// yet.
	const basic_template_processor& get_template_processor() const
	{
		if (not m_template_processor)
			throw std::logic_error("Template processor not specified yet");
		return *m_template_processor;
	}

	/// \brief returns whether template processor has been set
	bool has_template_processor() const
	{
		return (bool)m_template_processor;
	}

	/// \brief Bind the server to address \a address and port \a port
	virtual void bind(const std::string& address, unsigned short port);

	/// \brief Run as many as \a nr_of_threads threads simultaneously
	virtual void run(int nr_of_threads);

	/// \brief Stop all threads and stop listening
	virtual void stop();

	/// \brief to extend the log entry for a current request, use this ostream:
	static std::ostream& get_log();

	/// \brief log_forwarded tells the HTTP server to use the last entry in X-Forwarded-For as client log entry
	void set_log_forwarded(bool v) { m_log_forwarded = v; }

	/// \brief returns the address as specified in bind
	std::string get_address() const { return m_address; }

	/// \brief returns the port as specified in bind
	unsigned short get_port() const { return m_port; }

	/// \brief get_io_context has to be public since we need it to call notify_fork from child code
	virtual boost::asio::io_context& get_io_context() = 0;

	/// \brief get_executor has to be public since we need it to call notify_fork from child code
	boost::asio::io_context::executor_type get_executor() { return get_io_context().get_executor(); }

  protected:

	/// \brief the default entry logger
	virtual void log_request(const std::string& client,
							 const request& req, const reply& rep,
							 const boost::posix_time::ptime& start,
							 const std::string& referer, const std::string& userAgent,
							 const std::string& entry) noexcept;

  private:
	friend class preforked_server_base;
	friend class connection;

	virtual void handle_request(boost::asio::ip::tcp::socket& socket,
								request& req, reply& rep);

	void handle_accept(boost::system::error_code ec);

	std::shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
	std::list<std::thread> m_threads;
	std::shared_ptr<connection> m_new_connection;
	std::string m_address;
	unsigned short m_port;
	bool m_log_forwarded;
	std::string m_context_name;		/// \brief This is required for proxied servers e.g.
	std::unique_ptr<security_context> m_security_context;
	std::unique_ptr<basic_template_processor> m_template_processor;
	std::list<controller*> m_controllers;
	std::list<error_handler*> m_error_handlers;
	std::set<std::string> m_allowed_methods;
};

// --------------------------------------------------------------------
/// \brief The most often used server class, contains its own io_context.

class server : public basic_server
{
  public:
	/// \brief Simple server, no security, no template processor
	server() {}

	/// \brief Simple server, no security, create default template processor with \a docroot
	server(const std::string& docroot) : basic_server(docroot) {}

	/// \brief server with a security context for limited access
	server(security_context* s_ctxt) : basic_server(s_ctxt) {}

	/// \brief server with a security context for limited access, create default template processor with \a docroot
	server(security_context* s_ctxt, const std::string& docroot) : basic_server(s_ctxt, docroot) {}

	boost::asio::io_context& get_io_context() override
	{
		return m_io_context;
	}

	/// \brief Stop the server and also stop the io_context
	virtual void stop()
	{
		basic_server::stop();
		m_io_context.stop();
	}

  private:
	boost::asio::io_context m_io_context;
};

} // namespace zeep::http
