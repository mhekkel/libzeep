//           Copyright Maarten L. Hekkelman, 2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

// In this example we don't want to use rsrc based templates
#define WEBAPP_USES_RESOURCES 0

//[ simple_http_server_3
#include <zeep/http/server.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/template-processor.hpp>

class hello_controller : public zeep::http::html_controller
{
  public:
    hello_controller()
    {
        /*<< Mount the handler `handle_index`, this is on =/= >>*/
        map_get("", &hello_controller::handle_index, "name");

		/*<< Mount the handler on =/index.html= as well>>*/
		map_get("index.html", &hello_controller::handle_index, "name");

		/*<< And mount the handler on a path containing the 'name'>>*/
		map_get("hello/{name}", &hello_controller::handle_index, "name");
    }

    zeep::http::reply handle_index(const zeep::http::scope& scope, std::optional<std::string> user)
    {
        zeep::http::scope sub(scope);
        sub.put("name", user.value_or("world"));

		return get_template_processor().create_reply_from_template("hello.xhtml", sub);
    }
};

int main()
{
    /*<< Use the server constructor that takes the path to a docroot so it will construct a template processor >>*/
    zeep::http::server srv(std::filesystem::canonical("docroot"));

    srv.add_controller(new hello_controller());

    srv.bind("::", 8080);
    srv.run(2);

    return 0;
}
//]