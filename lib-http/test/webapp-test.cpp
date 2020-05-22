#define BOOST_TEST_MODULE WebApp_Test
#include <boost/test/included/unit_test.hpp>

#include <random>

#include <boost/asio.hpp>

#include <zeep/crypto.hpp>
#include <zeep/streambuf.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/http/server.hpp>

using namespace std;
namespace z = zeep;
namespace zx = zeep::xml;

using webapp = zeep::http::html_controller;

void compare(zeep::xml::document& a, zeep::xml::document& b)
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
		my_webapp() {
			// mount("test", &my_webapp::handle_test);
			mount_get("test", &my_webapp::handle_get_test);
			mount_post("test", &my_webapp::handle_post_test);
		}

		virtual void handle_test(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
		}

		virtual void handle_get_test(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("get", "text/plain");
		}

		virtual void handle_post_test(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("post", "text/plain");
		}

	} app;

	zeep::http::request req;
	req.method = zeep::http::method_type::GET;
	req.uri = "/test";

	zeep::http::reply rep;

	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "get");

	req.method = zeep::http::method_type::POST;
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "post");

	req.method = zeep::http::method_type::DELETE;
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::not_found);
}

// BOOST_AUTO_TEST_CASE(webapp_2)
// {
// 	webapp app;

// 	app.mount("test", &zeep::http::controller::handle_file);

// 	zeep::http::request req;
// 	req.method = zeep::http::method_type::GET;
// 	req.uri = "/test";

// 	zeep::http::reply rep;

// 	app.handle_request(req, rep);

// 	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::internal_server_error);
// }

BOOST_AUTO_TEST_CASE(webapp_4)
{
	using namespace zeep::xml::literals;

	webapp app;
	zx::document doc;

	app.load_template("fragment-file :: frag1", doc);

	auto test1 = R"(<?xml version="1.0"?>
<div>fragment-1</div>)"_xml;

	compare(doc, test1);

	doc.clear();

	app.load_template("fragment-file :: #frag2", doc);

	auto test2 = R"(<?xml version="1.0"?>
<div>fragment-2</div>)"_xml;

	compare(doc, test2);
	
}

// test various ways of mounting handlers
BOOST_AUTO_TEST_CASE(webapp_5)
{
	class my_webapp : public webapp
	{
	  public:
		my_webapp() {
			mount("test", &my_webapp::handle_test1);
			mount("*/*.x", &my_webapp::handle_test2);
			mount("**/*.x", &my_webapp::handle_test2b);
			mount("test/*", &my_webapp::handle_test3);
			mount("test/**", &my_webapp::handle_test4);

			mount("{css,scripts}/", &my_webapp::handle_testf);
		}

		virtual void handle_test1(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("1", "text/plain");
		}

		virtual void handle_test2(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("2", "text/plain");
		}

		virtual void handle_test2b(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("2b", "text/plain");
		}

		virtual void handle_test3(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("3", "text/plain");
		}

		virtual void handle_test4(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("4", "text/plain");
		}

		virtual void handle_testf(const zeep::http::request& request, const zeep::http::scope& scope, zeep::http::reply& reply)
		{
			reply = zeep::http::reply::stock_reply(zeep::http::ok);
			reply.set_content("f", "text/plain");
		}


	} app;

	zeep::http::request req;
	req.method = zeep::http::method_type::GET;

	zeep::http::reply rep;

	req.uri = "/test";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "1");

	req.uri = "/test/x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "3");

	req.uri = "/test/x/x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "4");

	req.uri = "iew.x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

	req.uri = "x/iew.x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2");

	req.uri = "x/x/iew.x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

	req.uri = "css/styles/my-style.css";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "f");

	req.uri = "scripts/x.js";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zeep::http::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "f");
}

zeep::http::reply simple_request(uint16_t port, const std::string_view& req)
{
	using boost::asio::ip::tcp;

	boost::asio::io_context io_context;

	tcp::resolver resolver(io_context);
	tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", std::to_string(port));

	tcp::socket socket(io_context);
	boost::asio::connect(socket, endpoints);

	boost::system::error_code ignored_error;
	boost::asio::write(socket, boost::asio::buffer(req), ignored_error);

	zeep::http::reply result;
	zeep::http::reply_parser p;

	for (;;)
	{
		boost::array<char, 128> buf;
		boost::system::error_code error;

		size_t len = socket.read_some(boost::asio::buffer(buf), error);

		if (error == boost::asio::error::eof)
			break; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		zeep::char_streambuf sb(buf.data(), len);

		auto r = p.parse(result, sb);
		if (r == true)
			break;
	}

	return result;
}

zeep::http::reply simple_request(uint16_t port, zeep::http::request& req)
{
	std::ostringstream os;
	os << req;

	return simple_request(port, os.str());
}

BOOST_AUTO_TEST_CASE(webapp_8)
{
	// start up a http server with a html_controller and stop it again

	class my_html_controller : public zeep::http::html_controller
	{
	  public:
		my_html_controller()
			: zeep::http::html_controller("/", "")
		{
			mount("", &my_html_controller::handle_index);
		}

		void handle_index(const zeep::http::request& req, const zeep::http::scope& scope, zeep::http::reply& rep)
		{
			rep = zeep::http::reply::stock_reply(zeep::http::ok);
			rep.set_content("Hello", "text/plain");
		}
	};

	zeep::http::daemon d([]() {
		auto server = new zeep::http::server;
		server->add_controller(new my_html_controller());
		return server;
	}, "zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zeep::http::daemon::run_foreground, d, "127.0.0.1", port));

	std::cerr << "started daemon at port " << port << std::endl;

	sleep(1);

	auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");

	pthread_kill(t.native_handle(), SIGHUP);

	t.join();

	BOOST_TEST(reply.get_status() == zeep::http::ok);
	BOOST_TEST(reply.get_content() == "Hello");
}
