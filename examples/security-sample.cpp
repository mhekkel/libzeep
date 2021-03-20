/* compile:
clang++  -o security-sample security-sample.cpp -I ~/projects/boost_1_73_0 -DWEBAPP_USES_RESOURCES -I. -fPIC -pthread -std=c++17 -Wall -g -DDEBUG -I ../../include/ -L ../../lib -lzeep-http -lzeep-xml -lzeep-json dummy_rsrc.o  -lstdc++fs
*/

// In this example we don't want to use rsrc based templates
#undef WEBAPP_USES_RESOURCES

#include <zeep/http/server.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/template-processor.hpp>
#include <zeep/http/security.hpp>

//[ sample_security_controller
class hello_controller : public zeep::http::html_controller
{
  public:
    hello_controller()
    {
        // Mount the handler `handle_index` on /, /index and /index.html
        mount("{,index,index.html}", &hello_controller::handle_index);

        // This admin page will only be accessible by authorized users
        mount("admin", &hello_controller::handle_admin);

        // scripts & css
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
//]

int main()
{
    //[ create_user_service
    // Create a user service with a single user
    zeep::http::simple_user_service users({
        {
            "scott",
            zeep::http::pbkdf2_sha256_password_encoder().encode("tiger"),
            { "USER", "ADMIN" }
        }
    });
    //]

    //[ create_security_context
    // Create a security context with a secret and users
    std::string secret = zeep::random_hash();
    auto sc = new zeep::http::security_context(secret, users, false);
    //]
    
    //[ add_access_rules
    // Add the rule,
    sc->add_rule("/admin", "ADMIN");
    sc->add_rule("/", {});
    //]

    //[ start_server
    /*<< Use the server constructor that takes the path to a docroot so it will construct a template processor >>*/
    zeep::http::server srv(sc, "docroot");

    srv.add_controller(new hello_controller());
    srv.add_controller(new zeep::http::login_controller());

    srv.bind("::", 8080);
    srv.run(2);
    //]

    return 0;
}