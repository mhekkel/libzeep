//           Copyright Maarten L. Hekkelman, 2022-2025
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

// In this example we don't want to use rsrc based templates
#undef WEBAPP_USES_RESOURCES
#define WEBAPP_USES_RESOURCES 0

#include "zeep/crypto.hpp"
#include "zeep/http/reply.hpp"

#include <exception>
#include <initializer_list>
#include <iostream>
#include <string>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/template-processor.hpp>

//[ sample_security_controller
class hello_controller : public zeep::http::html_controller
{
  public:
	hello_controller()
	{
		// Mount the handler `handle_index` on /, /index and /index.html
		map_get("{,index,index.html}", &hello_controller::handle_index);

		// This admin page will only be accessible by authorized users
		map_get("admin", &hello_controller::handle_admin);

		// scripts & css
		map_get_file("{css,scripts}/");
	}

	zeep::http::reply handle_index(const zeep::http::scope &scope)
	{
		return get_template_processor().create_reply_from_template("security-hello.xhtml", scope);
	}

	zeep::http::reply handle_admin(const zeep::http::scope &scope)
	{
		return get_template_processor().create_reply_from_template("security-admin.xhtml", scope);
	}
};
//]

int main()
{
	try
	{
		//[ create_user_service
		// Create a user service with a single user
		zeep::http::simple_user_service users({ { "scott",
			zeep::http::pbkdf2_sha256_password_encoder().encode("tiger"),
			{ "USER", "ADMIN" } } });
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
		/* Use the server constructor that takes the path to a docroot so it will construct a template processor */
		zeep::http::server srv(sc, "docroot");

		srv.add_controller(new hello_controller());
		srv.add_controller(new zeep::http::login_controller());

		srv.bind("::", 8080);
		srv.run(2);
		//]
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << '\n';
	}

	return 0;
}