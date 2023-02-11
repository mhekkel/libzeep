// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include "client-test-code.hpp"

namespace zh = zeep::http;

zh::reply simple_request(uint16_t port, const std::string& req)
{
	using boost::asio::ip::tcp;

#if BOOST_VERSION > 107000
	boost::asio::io_context io_context;
	tcp::resolver resolver(io_context);
	tcp::resolver::results_type endpoints = resolver.resolve("localhost", std::to_string(port));

	tcp::socket socket(io_context);
	boost::asio::connect(socket, endpoints);
#else
    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoint_iterator = resolver.resolve({ "localhost", std::to_string(port) });

	tcp::socket socket(io_context);

	boost::asio::connect(socket, endpoint_iterator);
#endif

	boost::system::error_code ignored_error;
	boost::asio::write(socket, boost::asio::buffer(req), ignored_error);

	zh::reply result;
	zh::reply_parser p;

	for (;;)
	{
		std::array<char, 128> buf;
		boost::system::error_code error;

		size_t len = socket.read_some(boost::asio::buffer(buf), error);

		if (error == boost::asio::error::eof)
			break; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		zeep::char_streambuf sb(buf.data(), len);

		auto r = p.parse(sb);
		if (r == true)
		{
			result = p.get_reply();
			break;
		}
	}

	return result;
}

zh::reply simple_request(uint16_t port, const zeep::http::request& req)
{
	std::ostringstream os;
	os << req;

	return simple_request(port, os.str());
}