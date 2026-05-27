//         Copyright Maarten L. Hekkelman, 2022-2026
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

// In this example we don't want to use rsrc based templates
#include <memory>
#undef WEBAPP_USES_RESOURCES
#define WEBAPP_USES_RESOURCES 0

#include <zeep/crypto.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/template-processor.hpp>

#include <mcfp/mcfp.hpp>

#include <exception>
#include <initializer_list>
#include <iostream>
#include <string>

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

int main(int argc, char *const argv[])
{
	try
	{
		auto &config = mcfp::config::instance();

		config.init("usage",
				  mcfp::make_option("help,h", "Display help message"),

				  mcfp::make_option<std::string>("config", "Path of config file to use"),

				  mcfp::make_option<std::string>("address", "0.0.0.0", "Address to listen to"),
				  mcfp::make_option<uint16_t>("port", 8080, "Port to listen to"),

				  mcfp::make_option<std::string>("port", "8080", "Port to listen to"))

			.add_section("openid",
				mcfp::make_option<std::string>("uri", "OpenID authentication provider"),
				mcfp::make_option<std::string>("name", "User friendly name"),
				mcfp::make_option<std::string>("redirect-uri",  "redirect-uri"),
				mcfp::make_option<std::string>("client-id", "client-id"),
				mcfp::make_option<std::string>("client-secret", "client-secret"));

		std::error_code ec;

		config.set_ignore_unknown(true);

		config.parse(argc, argv, ec);
		if (ec)
		{
			std::cerr << "Error parsing arguments: " << ec.message() << '\n';
			return 1;
		}

		if (config.has("help"))
		{
			std::cout << config << '\n';
			return 0;
		}

		config.parse_config_file("config", "openid-sample.conf",
			{ ".", "/etc" }, ec);
		if (ec)
		{
			std::cerr << "Error parsing config file: " << ec.message() << '\n';
			return 1;
		}

		auto uri = config.get("openid.uri");
		auto name = config.get("openid.name");
		auto redirectUri = config.get("openid.redirect-uri");
		auto clientId = config.get("openid.client-id");
		auto clientSecret = config.get("openid.client-secret");

		//[ create_user_service
		// create a OpenID user service
		//]

		zeep::http::openid_user_service users(uri, name, redirectUri, clientId, clientSecret);

		//[ create_security_context
		// Create a security context for use with the specified OpenID provider
		auto sc = std::make_unique<zeep::http::security_context>(clientSecret, users);
		//]

		//[ add_access_rules
		// Add the rule,
		sc->add_rule("/admin", "ADMIN");
		sc->add_rule("/", {});
		//]

		//[ start_server
		/* Use the server constructor that takes the path to a docroot so it will construct a template processor */
		zeep::http::server srv(sc.release(), "docroot");

		srv.add_controller(new hello_controller());
		srv.add_controller(new zeep::http::login_controller());

		srv.bind(config.get("address"), config.get<uint16_t>("port"));
		srv.run(2);
		//]
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << '\n';
	}

	return 0;
}