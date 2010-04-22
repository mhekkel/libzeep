//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/config.hpp"

#include <iostream>
#include <sstream>
#include <locale>

#include "zeep/http/server.hpp"
#include "zeep/http/connection.hpp"
#include "zeep/exception.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <sys/wait.h>

using namespace std;

namespace zeep { namespace http {

namespace detail {

// a thread specific logger

boost::thread_specific_ptr<ostringstream>	s_log;
boost::mutex								s_log_lock;

}

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
	
	try
	{
		handle_request(req, rep);
	}
	catch (...)
	{
		rep = reply::stock_reply(internal_server_error);
	}

	// protect the output stream from garbled log messages
	boost::mutex::scoped_lock lock(detail::s_log_lock);
	cout << socket.remote_endpoint().address()
		 << " [" << start << "] "
		 << second_clock::local_time() - start << ' '
		 << rep.status << ' '
		 << detail::s_log->str() << endl;
}

}
}
