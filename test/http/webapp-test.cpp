#include "zeep/http/asio.hpp"

#include "test-main.hpp"

#include <zeep/crypto.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/http/server.hpp>
#include <zeep/streambuf.hpp>

#include "signals.hpp"
#include "client-test-code.hpp"

#include <iostream>
#include <random>

namespace z = zeep;

using webapp = zeep::http::html_controller_v1;

void compare(zeem::document &a, zeem::document &b)
{
	CHECK(a == b);
	if (a != b)
	{
		std::cerr << std::string(80, '-') << '\n'
			 << a << '\n'
			 << std::string(80, '-') << '\n'
			 << b << '\n'
			 << std::string(80, '-') << '\n';
	}
}

TEST_CASE("webapp_1")
{
	class my_webapp : public webapp
	{
	  public:
		my_webapp()
		{
			// mount("test", &my_webapp::handle_test);
			mount_get("test", &my_webapp::handle_get_test);
			mount_post("test", &my_webapp::handle_post_test);
		}

		void handle_test(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
		}

		void handle_get_test(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("get", "text/plain");
		}

		void handle_post_test(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("post", "text/plain");
		}

	} app;

	zeep::http::request req("GET", "/test", { 1, 0 });

	zeep::http::reply rep;

	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "get");

	req.set_method("POST");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "post");

	req.set_method("DELETE");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::not_found);
}

// TEST_CASE("webapp_2")
// {
// 	webapp app;

// 	app.mount("test", &zeep::http::controller::handle_file);

// 	zeep::http::request req;
// 	req.method = zeep::"GET";
// 	req.set_uri("/test");

// 	zeep::http::reply rep;

// 	app.handle_request(req, rep);

// 	CHECK(rep.get_status() == zeep::http::internal_server_error);
// }

// TEST_CASE("webapp_4")
// {
// 	using namespace zeem::literals;

// 	webapp app;
// 	zx::document doc;

// 	app.load_template("fragment-file :: frag1", doc);

// 	auto test1 = R"(<?xml version="1.0"?>
// <div>fragment-1</div>)"_xml;

// 	compare(doc, test1);

// 	doc.clear();

// 	app.load_template("fragment-file :: #frag2", doc);

// 	auto test2 = R"(<?xml version="1.0"?>
// <div>fragment-2</div>)"_xml;

// 	compare(doc, test2);

// }

// test various ways of mounting handlers
TEST_CASE("webapp_5")
{
	class my_webapp : public webapp
	{
	  public:
		my_webapp()
		{
			mount("test", &my_webapp::handle_test1);
			mount("*/*.x", &my_webapp::handle_test2);
			mount("**/*.x", &my_webapp::handle_test2b);
			mount("test/*", &my_webapp::handle_test3);
			mount("test/**", &my_webapp::handle_test4);

			mount("{css,scripts}/", &my_webapp::handle_testf);
		}

		void handle_test1(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("1", "text/plain");
		}

		void handle_test2(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("2", "text/plain");
		}

		void handle_test2b(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("2b", "text/plain");
		}

		void handle_test3(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("3", "text/plain");
		}

		void handle_test4(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("4", "text/plain");
		}

		void handle_testf(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("f", "text/plain");
		}

	} app;

	zeep::http::request req("GET", "/test");
	zeep::http::reply rep;

	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "1");

	req.set_uri("/test/x");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "3");

	req.set_uri("/test/x/x");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "4");

	req.set_uri("iew.x");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "2b");

	req.set_uri("x/iew.x");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "2");

	req.set_uri("x/x/iew.x");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "2b");

	req.set_uri("css/styles/my-style.css");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "f");

	req.set_uri("scripts/x.js");
	app.handle_request(req, rep);
	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content() == "f");
}

class hello_controller : public zeep::http::html_controller_v1
{
  public:
	hello_controller()
		: zeep::http::html_controller_v1("/")
	{
		mount("", &hello_controller::handle_index);
	}

	void handle_index(const zeep::http::request & /*req*/, const zeep::http::scope & /*scope*/, zeep::http::reply &rep)
	{
		rep = zeep::http::reply::stock_reply(zeep::http::ok);
		rep.set_content("Hello", "text/plain");
	}
};

TEST_CASE("webapp_8")
{
	// start up a http server with a html_controller and stop it again

	zeep::http::daemon d([]()
		{
		auto server = new zeep::http::server;
		server->add_controller(new hello_controller());
		return server; },
		"zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zeep::http::daemon::run_foreground, d, "::", port));

	std::clog << "started daemon at port " << port << '\n';

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(100ms);

	try
	{
		auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");

		CHECK(reply.get_status() == zeep::http::ok);
		CHECK(reply.get_content() == "Hello");
	}
	catch (const std::exception &ex)
	{
		std::clog << ex.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}

TEST_CASE("webapp_10")
{
	zeep::http::server srv;

	srv.add_controller(new hello_controller());

	std::thread t([&srv]() mutable
		{
		
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);

		srv.stop(); });

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	srv.bind("::", port);
	srv.run(2);

	t.join();
}

// // a more generic set of tests, should be in a separate file I guess

// TEST_CASE("split_1")
// {
// 	std::vector<std::string> p;
// 	zeep::split(p, ",een,twee"s, ",", false);

// 	REQUIRE(p.size() == 3);
// 	CHECK(p[0] == "");
// 	CHECK(p[1] == "een");
// 	CHECK(p[2] == "twee");

// 	zeep::split(p, ",een,twee"s, ",", true);

// 	REQUIRE(p.size() == 2);
// 	CHECK(p[0] == "een");
// 	CHECK(p[1] == "twee");
// }

// --------------------------------------------------------------------

// // test various ways of mounting handlers
// TEST_CASE("webapp_5")
// {
// 	class my_webapp : public webapp_2
// 	{
// 	  public:
// 		my_webapp() {
// 			mount("test", &my_webapp::handle_test1);
// 			mount("*/*.x", &my_webapp::handle_test2);
// 			mount("**/*.x", &my_webapp::handle_test2b);
// 			mount("test/*", &my_webapp::handle_test3);
// 			mount("test/**", &my_webapp::handle_test4);

// 			mount("{css,scripts}/", &my_webapp::handle_testf);
// 		}

// 		virtual void handle_test1(const zeep::http::request& /*request*/, const zeep::http::scope& /*scope*/, zeep::http::reply& reply)
// 		{
// 			reply = zeep::http::reply::stock_reply(zeep::http::ok);
// 			reply.set_content("1", "text/plain");
// 		}

// 		virtual void handle_test2(const zeep::http::request& /*request*/, const zeep::http::scope& /*scope*/, zeep::http::reply& reply)
// 		{
// 			reply = zeep::http::reply::stock_reply(zeep::http::ok);
// 			reply.set_content("2", "text/plain");
// 		}

// 		virtual void handle_test2b(const zeep::http::request& /*request*/, const zeep::http::scope& /*scope*/, zeep::http::reply& reply)
// 		{
// 			reply = zeep::http::reply::stock_reply(zeep::http::ok);
// 			reply.set_content("2b", "text/plain");
// 		}

// 		virtual void handle_test3(const zeep::http::request& /*request*/, const zeep::http::scope& /*scope*/, zeep::http::reply& reply)
// 		{
// 			reply = zeep::http::reply::stock_reply(zeep::http::ok);
// 			reply.set_content("3", "text/plain");
// 		}

// 		virtual void handle_test4(const zeep::http::request& /*request*/, const zeep::http::scope& /*scope*/, zeep::http::reply& reply)
// 		{
// 			reply = zeep::http::reply::stock_reply(zeep::http::ok);
// 			reply.set_content("4", "text/plain");
// 		}

// 		virtual void handle_testf(const zeep::http::request& /*request*/, const zeep::http::scope& /*scope*/, zeep::http::reply& reply)
// 		{
// 			reply = zeep::http::reply::stock_reply(zeep::http::ok);
// 			reply.set_content("f", "text/plain");
// 		}

// 	} app;

// 	zeep::http::request req("GET", "/test");
// 	zeep::http::reply rep;

// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "1");

// 	req.set_uri("/test/x");
// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "3");

// 	req.set_uri("/test/x/x");
// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "4");

// 	req.set_uri("iew.x");
// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "2b");

// 	req.set_uri("x/iew.x");
// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "2");

// 	req.set_uri("x/x/iew.x");
// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "2b");

// 	req.set_uri("css/styles/my-style.css");
// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "f");

// 	req.set_uri("scripts/x.js");
// 	app.handle_request(req, rep);
// 	CHECK(rep.get_status() == zeep::http::ok);
// 	CHECK(rep.get_content() == "f");
// }

class hello_controller_2 : public zeep::http::html_controller
{
  public:
	hello_controller_2()
		: zeep::http::html_controller("/")
	{
		map_get("", &hello_controller_2::handle_index, "user");
		map_get("hello/{user}", &hello_controller_2::handle_hello, "user");
		map_get("hello/{user}/x", &hello_controller_2::handle_hello, "user");
	}

	zeep::http::reply handle_index([[maybe_unused]] const zeep::http::scope &scope, std::optional<std::string> user)
	{
		auto rep = zeep::http::reply::stock_reply(zeep::http::ok);
		rep.set_content("Hello, " + user.value_or("world") + "!", "text/plain");
		return rep;
	}

	zeep::http::reply handle_hello([[maybe_unused]] const zeep::http::scope &scope, std::optional<std::string> user)
	{
		auto rep = zeep::http::reply::stock_reply(zeep::http::ok);
		rep.set_content("Hello, " + user.value_or("world") + "!", "text/plain");
		return rep;
	}
};

TEST_CASE("controller_2_1")
{
	// start up a http server with a html_controller and stop it again

	zeep::http::daemon d([]()
		{
		auto server = new zeep::http::server;
		server->add_controller(new hello_controller_2());
		return server; },
		"zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zeep::http::daemon::run_foreground, d, "::", port));

	std::clog << "started daemon at port " << port << '\n';

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(100ms);

	try
	{
		auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");

		CHECK(reply.get_status() == zeep::http::ok);
		CHECK(reply.get_content() == "Hello, world!");

		reply = simple_request(port, "GET /?user=maarten HTTP/1.0\r\n\r\n");

		CHECK(reply.get_status() == zeep::http::ok);
		CHECK(reply.get_content() == "Hello, maarten!");

		reply = simple_request(port, "GET /hello/maarten HTTP/1.0\r\n\r\n");

		CHECK(reply.get_status() == zeep::http::ok);
		CHECK(reply.get_content() == "Hello, maarten!");

		reply = simple_request(port, "GET /hello/maarten/x HTTP/1.0\r\n\r\n");

		CHECK(reply.get_status() == zeep::http::ok);
		CHECK(reply.get_content() == "Hello, maarten!");

		reply = simple_request(port, "GET /hello//x HTTP/1.0\r\n\r\n");

		CHECK(reply.get_status() == zeep::http::ok);
		CHECK(reply.get_content() == "Hello, world!");

		reply = simple_request(port, "GET /hello/dani%C3%ABlle/x HTTP/1.0\r\n\r\n");

		CHECK(reply.get_status() == zeep::http::ok);
		CHECK(reply.get_content() == "Hello, daniëlle!");

	}
	catch (const std::exception &ex)
	{
		std::clog << ex.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}
