//           Copyright Maarten L. Hekkelman, 2022-2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <zeep/http/controller.hpp>
#include <zeep/http/daemon.hpp>

namespace zh = zeep::http;

class hello_controller : public zh::controller
{
  public:
	/* Specify the root path as prefix, will handle any request URI */
	hello_controller()
		: controller("/")
	{
	}

	bool handle_request(zh::request &req, zh::reply &rep)
	{
		/* Construct a simple reply with status OK (200) and content string */
		rep = zh::reply::stock_reply(zh::ok);
		rep.set_content("Hello", "text/plain");
		return true;
	}
};

int main(int argc, char *const argv[])
{
	using namespace std::literals;

	if (argc != 2)
	{
		std::cout << "No command specified, use of of start, stop, status or reload" << std::endl;
		exit(1);
	}

	// --------------------------------------------------------------------

	std::string command = argv[1];

	zh::daemon server([&]()
		{
		auto s = new zeep::http::server(/*sc*/);

		s->add_controller(new hello_controller());

		return s; },
		"hello-daemon");

	int result;

	if (command == "start")
	{
		std::string address = "127.0.0.1";
		unsigned short port = 10330;
		std::string user = "www-data";
		std::cout << "starting server at http://" << address << ':' << port << '/' << std::endl;
		result = server.start(address, port, 1, 16, user);
	}
	else if (command == "stop")
		result = server.stop();
	else if (command == "status")
		result = server.status();
	else if (command == "reload")
		result = server.reload();
	else
	{
		std::cerr << "Invalid command" << std::endl;
		result = 1;
	}

	return result;
}
