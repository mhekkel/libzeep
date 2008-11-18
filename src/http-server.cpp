#include <iostream>

#include "soap/http/server.hpp"
#include "soap/http/connection.hpp"
#include <boost/lexical_cast.hpp>

using namespace std;

namespace soap { namespace http {

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

void server::handle_request(const request& req, reply& rep)
{
	cout << "Received request for " << req.uri << endl;
	rep = reply::stock_reply(not_found);
}

}
}
