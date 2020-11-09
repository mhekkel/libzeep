/* compile:
clang++  -o http-server-1 http-server-1.cpp -I ~/projects/boost_1_73_0 -DWEBAPP_USES_RESOURCES -I. -fPIC -pthread -std=c++17 -Wall -g -DDEBUG -I ../../include/ -L ../../lib -lzeep-http -lzeep-xml -lzeep-json dummy_rsrc.o  -lstdc++fs
*/

// In this example we don't want to use rsrc based templates
#undef WEBAPP_USES_RESOURCES

//[ simple_http_server_2
#include <zeep/http/server.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/template-processor.hpp>

class hello_controller : public zeep::http::html_controller
{
  public:
    hello_controller()
    {
        /*<< Mount the handler `handle_index` on =/=, =/index= and =/index.html= >>*/
        mount("{,index,index.html}", &hello_controller::handle_index);
    }

    void handle_index(const zeep::http::request& req, const zeep::http::scope& scope, zeep::http::reply& rep)
    {
        zeep::http::scope sub(scope);
        auto name = req.get_parameter("name");
        if (not name.empty())
            sub.put("name", name);
        
        get_template_processor().create_reply_from_template("hello.xhtml", sub, rep);
    }
};

int main()
{
    /*<< Use the server constructor that takes the path to a docroot so it will construct a template processor >>*/
    zeep::http::server srv("docroot");

    srv.add_controller(new hello_controller());

    srv.bind("localhost", 8080);
    srv.run(2);

    return 0;
}
//]