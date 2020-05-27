/* compile:
clang++  -o authentication-sample authentication-sample.cpp -I ~/projects/boost_1_73_0 -DWEBAPP_USES_RESOURCES -I. -fPIC -pthread -std=c++17 -Wall -g -DDEBUG -I ../../include/ -L ../../lib -lzeep-http -lzeep-xml -lzeep-json dummy_rsrc.o  -lstdc++fs
*/

// In this example we don't want to use rsrc based templates
#undef WEBAPP_USES_RESOURCES

//[ authentication_sample
#include <zeep/http/server.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/template-processor.hpp>
#include <zeep/http/security.hpp>

class hello_controller : public zeep::http::html_controller
{
  public:
    hello_controller()
    {
        /*<< Mount the handler `handle_index` on =/=, =/index= and =/index.html= >>*/
        mount("{,index,index.html}", &hello_controller::handle_index);

        /*<< This admin page will only be accessible by authorized users >>*/      
        mount("admin", &hello_controller::handle_admin);

        mount("{css,scripts}/", &hello_controller::handle_file);
    }

    void handle_index(const zeep::http::request& req, const zeep::http::scope& scope, zeep::http::reply& rep)
    {
        get_template_processor().create_reply_from_template("security-hello.xhtml", scope, rep);
    }

    void handle_admin(const zeep::http::request& req, const zeep::http::scope& scope, zeep::http::reply& rep)
    {
        get_template_processor().create_reply_from_template("security-admin.xhtml", scope, rep);
    }
};

int main()
{
    // Create a user service with a single user
    zeep::http::simple_user_service users({
        {
            "scott",
            zeep::http::pbkdf2_sha256_password_encoder().encode("tiger"),
            { "USER", "ADMIN" }
        }
    });

    // Create a security context with a secret and users
    std::string secret = zeep::random_hash();
    auto sc = new zeep::http::security_context(secret, users, false);
    
    // Add the rule,
    sc->add_rule("/admin", "ADMIN");
    sc->add_rule("/", {});

    /*<< Use the server constructor that takes the path to a docroot so it will construct a template processor >>*/
    zeep::http::server srv(sc, "docroot");

    srv.add_controller(new hello_controller());
    srv.add_controller(new zeep::http::login_controller());

    srv.bind("127.0.0.1", 8080);
    srv.run(2);

    return 0;
}
//]