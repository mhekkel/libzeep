//[ simple_http_server
#include <zeep/http/server.hpp>
#include <zeep/http/controller.hpp>

class hello_controller : public zeep::http::controller
{
  public:
    /*<< Specify the root path as prefix, will handle any request URI >>*/
    hello_controller() : controller("/") {}

    bool handle_request(zeep::http::request& req, zeep::http::reply& rep)
    {
        /*<< Construct a simple reply with status OK (200) and content string >>*/
        rep = zeep::http::reply::stock_reply(zeep::http::ok);
        rep.set_content("Hello", "text/plain");
        return true;
    }
};

int main()
{
    zeep::http::server srv;

    srv.add_controller(new hello_controller());

    srv.bind("localhost", 8080);
    srv.run(2);

    return 0;
}
//]