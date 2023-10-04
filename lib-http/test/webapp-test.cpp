#include "zeep/http/asio.hpp"

#define BOOST_TEST_MODULE WebApp_Test
#include <boost/test/included/unit_test.hpp>

#include <random>

#include <zeep/crypto.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/http/server.hpp>
#include <zeep/streambuf.hpp>

#include "../src/signals.hpp"
#include "client-test-code.hpp"

using namespace std;
namespace z = zeep;
namespace zx = zeep::xml;

using webapp = zeep::http::html_controller;

void compare(zeep::xml::document &a, zeep::xml::document &b)
{
	BOOST_CHECK_EQUAL(a, b);
	if (a != b)
	{
		cerr << string(80, '-') << endl
			 << a << endl
			 << string(80, '-') << endl
			 << b << endl
			 << string(80, '-') << endl;
	}
}

BOOST_AUTO_TEST_CASE(webapp_1)
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

		virtual void handle_test(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
		}

		virtual void handle_get_test(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("get", "text/plain");
		}

		virtual void handle_post_test(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("post", "text/plain");
		}

	} app;

	zeep::http::request req("GET", "/test", { 1, 0 });

	zeep::http::reply rep;

	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "get");

	req.set_method("POST");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "post");

	req.set_method("DELETE");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::not_found);
}

// BOOST_AUTO_TEST_CASE(webapp_2)
// {
// 	webapp app;

// 	app.mount("test", &zeep::http::controller::handle_file);

// 	zeep::http::request req;
// 	req.method = zeep::"GET";
// 	req.set_uri("/test");

// 	zeep::http::reply rep;

// 	app.handle_request(req, rep);

// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::internal_server_error);
// }

// BOOST_AUTO_TEST_CASE(webapp_4)
// {
// 	using namespace zeep::xml::literals;

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
BOOST_AUTO_TEST_CASE(webapp_5)
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

		virtual void handle_test1(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("1", "text/plain");
		}

		virtual void handle_test2(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("2", "text/plain");
		}

		virtual void handle_test2b(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("2b", "text/plain");
		}

		virtual void handle_test3(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("3", "text/plain");
		}

		virtual void handle_test4(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("4", "text/plain");
		}

		virtual void handle_testf(const zeep::http::request & /*request*/, const zeep::http::scope & /*scope*/, zeep::http::reply &reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("f", "text/plain");
		}

	} app;

	zeep::http::request req("GET", "/test");
	zeep::http::reply rep;

	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "1");

	req.set_uri("/test/x");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "3");

	req.set_uri("/test/x/x");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "4");

	req.set_uri("iew.x");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

	req.set_uri("x/iew.x");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2");

	req.set_uri("x/x/iew.x");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

	req.set_uri("css/styles/my-style.css");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "f");

	req.set_uri("scripts/x.js");
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "f");
}

class hello_controller : public zeep::http::html_controller
{
  public:
	hello_controller()
		: zeep::http::html_controller("/")
	{
		mount("", &hello_controller::handle_index);
	}

	void handle_index(const zeep::http::request & /*req*/, const zeep::http::scope & /*scope*/, zeep::http::reply &rep)
	{
		rep = zeep::http::reply::stock_reply(zeep::http::ok);
		rep.set_content("Hello", "text/plain");
	}
};

BOOST_AUTO_TEST_CASE(webapp_8)
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

	std::cerr << "started daemon at port " << port << std::endl;

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(2s);

	try
	{
		auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");

		BOOST_TEST(reply.get_status() == zeep::http::ok);
		BOOST_TEST(reply.get_content() == "Hello");
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}

BOOST_AUTO_TEST_CASE(webapp_10)
{
	zeep::http::server srv;

	srv.add_controller(new hello_controller());

	std::thread t([&srv]() mutable
		{
		
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2s);

		srv.stop(); });

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	srv.bind("::", port);
	srv.run(2);

	t.join();
}

// a more generic set of tests, should be in a separate file I guess

BOOST_AUTO_TEST_CASE(split_1)
{
	std::vector<std::string> p;
	zeep::split(p, ",een,twee"s, ",", false);

	BOOST_ASSERT(p.size() == 3);
	BOOST_CHECK_EQUAL(p[0], "");
	BOOST_CHECK_EQUAL(p[1], "een");
	BOOST_CHECK_EQUAL(p[2], "twee");

	zeep::split(p, ",een,twee"s, ",", true);

	BOOST_ASSERT(p.size() == 2);
	BOOST_CHECK_EQUAL(p[0], "een");
	BOOST_CHECK_EQUAL(p[1], "twee");
}

// --------------------------------------------------------------------

// // test various ways of mounting handlers
// BOOST_AUTO_TEST_CASE(webapp_5)
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
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "1");

// 	req.set_uri("/test/x");
// 	app.handle_request(req, rep);
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "3");

// 	req.set_uri("/test/x/x");
// 	app.handle_request(req, rep);
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "4");

// 	req.set_uri("iew.x");
// 	app.handle_request(req, rep);
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

// 	req.set_uri("x/iew.x");
// 	app.handle_request(req, rep);
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "2");

// 	req.set_uri("x/x/iew.x");
// 	app.handle_request(req, rep);
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

// 	req.set_uri("css/styles/my-style.css");
// 	app.handle_request(req, rep);
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "f");

// 	req.set_uri("scripts/x.js");
// 	app.handle_request(req, rep);
// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
// 	BOOST_CHECK_EQUAL(rep.get_content(), "f");
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

BOOST_AUTO_TEST_CASE(controller_2_1)
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

	std::cerr << "started daemon at port " << port << std::endl;

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(2s);

	try
	{
		auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");

		BOOST_TEST(reply.get_status() == zeep::http::ok);
		BOOST_TEST(reply.get_content() == "Hello, world!");

		reply = simple_request(port, "GET /?user=maarten HTTP/1.0\r\n\r\n");

		BOOST_TEST(reply.get_status() == zeep::http::ok);
		BOOST_TEST(reply.get_content() == "Hello, maarten!");

		reply = simple_request(port, "GET /hello/maarten HTTP/1.0\r\n\r\n");

		BOOST_TEST(reply.get_status() == zeep::http::ok);
		BOOST_TEST(reply.get_content() == "Hello, maarten!");

		reply = simple_request(port, "GET /hello/maarten/x HTTP/1.0\r\n\r\n");

		BOOST_TEST(reply.get_status() == zeep::http::ok);
		BOOST_TEST(reply.get_content() == "Hello, maarten!");

		reply = simple_request(port, "GET /hello//x HTTP/1.0\r\n\r\n");

		BOOST_TEST(reply.get_status() == zeep::http::ok);
		BOOST_TEST(reply.get_content() == "Hello, world!");
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}
