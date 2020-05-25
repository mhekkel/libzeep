/* compile:
clang++  -o http-server-1 http-server-1.cpp -I ~/projects/boost_1_73_0 -DWEBAPP_USES_RESOURCES -I. -fPIC -pthread -std=c++17 -Wall -g -DDEBUG -I ../../include/ -L ../../lib -lzeep-http -lzeep-xml -lzeep-json dummy_rsrc.o  -lstdc++fs
*/

//[ simple_http_server
#include <zeep/http/server.hpp>
#include <zeep/http/html-controller.hpp>

class hello_controller : public zeep::http::html_controller
{
	public:
	hello_controller()
		: zeep::http::html_controller("/", "")
	{
		mount("", &hello_controller::handle_index);
	}

	void handle_index(const zeep::http::request& req, const zeep::http::scope& scope, zeep::http::reply& rep)
	{
		rep = zeep::http::reply::stock_reply(zeep::http::ok);
		rep.set_content("Hello", "text/plain");
	}
};

int main()
{
    zeep::http::server srv;

	srv.add_controller(new hello_controller());

    srv.bind("127.0.0.1", 8080);
    srv.run(2);

    return 0;
}
//]