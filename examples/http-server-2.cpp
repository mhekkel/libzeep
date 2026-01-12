//           Copyright Maarten L. Hekkelman, 2022-2025
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

// In this example we don't want to use rsrc based templates
#undef WEBAPP_USES_RESOURCES
#define WEBAPP_USES_RESOURCES 0

//[ simple_http_server_2

#include <zeep/http/html-controller.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/http/scope.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/template-processor.hpp>

#include <exception>
#include <iostream>
#include <optional>
#include <string>

class hello_controller : public zeep::http::html_controller
{
  public:
	hello_controller()
	{
		/* Mount the handler `handle_index` on `/`, `/index` and `/index.html` */
		map_get("{,index,index.html}", &hello_controller::handle_index, "name");
	}

	zeep::http::reply handle_index(const zeep::http::scope &scope, std::optional<std::string> name)
	{
		zeep::http::scope sub(scope);
		if (name.has_value())
			sub.put("name", *name);

		return get_template_processor().create_reply_from_template("hello.xhtml", sub);
	}
};

int main()
{
	try
	{
		/* Use the server constructor that takes the path to a docroot so it will construct a template processor */
		zeep::http::server srv("docroot");

		srv.add_controller(new hello_controller());

		srv.bind("::", 8080);
		srv.run(2);
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << '\n';
	}

	return 0;
}
//]