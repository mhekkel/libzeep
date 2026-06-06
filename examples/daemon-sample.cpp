//         Copyright Maarten L. Hekkelman, 2022-2025
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <filesystem>
#include <iostream>

#include <zeep/http/html-controller.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/template-processor.hpp>

class hello_controller : public zeep::http::html_controller
{
  public:
	/* Specify the root path as prefix, will handle any request URI */
	hello_controller()
	{
		/* Mount the handler `handle_index` on `/`, `/index` and `/index.html` */
		map_get("{,index,index.html}", &hello_controller::handle_index, "name");

		/* Add REST call */
		map_get_request("restcall", &hello_controller::handle_rest_call);
	}

	zeep::http::reply handle_index(const zeep::http::scope &scope, std::optional<std::string> name)
	{
		zeep::http::scope sub(scope);
		if (name.has_value())
			sub.put("name", *name);

		return get_template_processor().create_reply_from_template("hello.xhtml", sub);
	}

	std::string handle_rest_call()
	{
		return "Hello, world!";
	}
};

int main(int argc, char *const argv[])
{
	using namespace std::literals;

	if (argc != 2)
	{
		std::cout << "No command specified, use of of start, stop, status or reload\n";
		exit(1);
	}

	// --------------------------------------------------------------------

	std::string command = argv[1];

	// Avoid the need for super powers
	std::filesystem::path
		docRoot = std::filesystem::current_path() / "docroot",
		pidFile = std::filesystem::temp_directory_path() / "zeep-daemon-test.pid",
		logFile = std::filesystem::temp_directory_path() / "zeep-daemon-test.log",
		errFile = std::filesystem::temp_directory_path() / "zeep-daemon-test.err";

	if (not std::filesystem::exists(docRoot))
	{
		std::cout << "docroot directory not found in current directory, cannot continue\n";
		exit(1);
	}

	zeep::http::daemon server([&]()
		{
		auto s = new zeep::http::server(docRoot.string());

		// Force using the file based template processor
		s->set_template_processor(new zeep::http::file_based_html_template_processor(docRoot.string()));

		s->add_controller(new hello_controller());

		return s; },
		pidFile.string(), logFile.string(), errFile.string());

	int result;

	if (command == "start")
	{
		std::string address = "localhost";
		unsigned short port = 10330;
		std::string user /* = "www-data" */;	// Using a non-empty username requires super user powers
		std::cout << "starting server at http://" << address << ':' << port << "\n";

		// Using preforked server:
		result = server.start(address, port, 8, 8, user);

		// Alternatively use a threaded server:
		// result = server.start(address, port, 8, user);
	}
	else if (command == "stop")
		result = server.stop();
	else if (command == "status")
		result = server.status();
	else if (command == "reload")
		result = server.reload();
	else
	{
		std::clog << "Invalid command\n";
		result = 1;
	}

	return result;
}
