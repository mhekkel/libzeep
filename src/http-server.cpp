#include <iostream>
#include <sstream>

#include "soap/http/server.hpp"
#include "soap/http/connection.hpp"

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

namespace soap { namespace http {

namespace detail {

// a thread specific logger

boost::thread_specific_ptr<ostringstream>	s_log;

}

server::server(const std::string& address, short port, int nr_of_threads)
	: m_acceptor(m_io_service)
	, m_new_connection(new connection(m_io_service, *this))
{
	boost::asio::ip::tcp::resolver resolver(m_io_service);
	boost::asio::ip::tcp::resolver::query query(address, boost::lexical_cast<string>(port));
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

	m_acceptor.open(endpoint.protocol());
	m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	m_acceptor.bind(endpoint);
	m_acceptor.listen();
	m_acceptor.async_accept(m_new_connection->get_socket(),
		boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));

	for (int i = 0; i < nr_of_threads; ++i)
	{
		m_threads.create_thread(
			boost::bind(&boost::asio::io_service::run, &m_io_service));
	}
}

server::~server()
{
	stop();
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
		m_acceptor.async_accept(m_new_connection->get_socket(),
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
	detail::s_log.reset(new ostringstream);
	
	log() << socket.remote_endpoint().address() << ' ';
	
	try
	{
		handle_request(req, rep);
	}
	catch (...)
	{
	}
	
	cout << detail::s_log->str() << endl;
}

}
}
