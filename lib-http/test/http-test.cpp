#include <boost/asio.hpp>

#define BOOST_TEST_MODULE HTTP_Test
#include <boost/test/included/unit_test.hpp>

#include <random>
#include <sstream>

#include <zeep/crypto.hpp>
#include <zeep/exception.hpp>
#include <zeep/streambuf.hpp>

#include <zeep/http/controller.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/server.hpp>

#include "client-test-code.hpp"
#include "../src/signals.hpp"

namespace z = zeep;
namespace zx = zeep::xml;
namespace zh = zeep::http;

BOOST_AUTO_TEST_CASE(http_base64_1)
{
	using namespace std::literals;

	auto in = R"(Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.)"s;
	
	auto out = R"(TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz
IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg
dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu
dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo
ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=
)"s;

	auto test = zeep::encode_base64(in, 76);

	BOOST_TEST(test == out);

	auto s = zeep::decode_base64(test);

	BOOST_TEST(s == in);
}

BOOST_AUTO_TEST_CASE(http_base64_2)
{
	using namespace std::literals;

	const std::string tests[] =
	{
		"1", "12", "123", "1234",
		{ '\0' }, { '\0', '\001' }, { '\0', '\001', '\002' }
	};

	for (const auto& test: tests)
	{
		auto enc = zeep::encode_base64(test, 76);

		auto dec = zeep::decode_base64(enc);

		BOOST_TEST(dec == test);
	}
}

BOOST_AUTO_TEST_CASE(connection_read)
{
#pragma message("write test for avail/used")
}

BOOST_AUTO_TEST_CASE(request_params_1)
{
	zh::request req{ "GET", "http://www.example.com/index?a=A;b=B&c=C%24"};

	BOOST_CHECK_EQUAL(req.get_parameter("a"), "A");
	BOOST_CHECK_EQUAL(req.get_parameter("b"), "B");
	BOOST_CHECK_EQUAL(req.get_parameter("c"), "C$");
}

BOOST_AUTO_TEST_CASE(webapp_6)
{
	zh::request req("GET", "/", {1, 0}, { { "Content-Type", "multipart/form-data; boundary=xYzZY" } },
		"--xYzZY\r\n"
		"Content-Disposition: form-data; name=\"pdb-file\"; filename=\"1cbs.cif.gz\"\r\n"
		"Content-Encoding: gzip\r\n"
		"Content-Type: chemical/x-cif\r\n\r\nello, world!\n\r\n"
		"--xYzZY\r\n"
		"Content-Disposition: form-data; name=\"mtz-file\"; filename=\"1cbs_map.mtz\"\r\n"
		"Content-Type: text/plain\r\n\r\nnd again, hello!\n\r\n"
		"--xYzZY--\r\n");

	auto fp1 = req.get_file_parameter("pdb-file");
	BOOST_CHECK_EQUAL(fp1.filename, "1cbs.cif.gz");
	BOOST_CHECK_EQUAL(fp1.mimetype, "chemical/x-cif");
	BOOST_CHECK_EQUAL(std::string(fp1.data, fp1.data + fp1.length), "hello, world!\n");

	auto fp2 = req.get_file_parameter("mtz-file");
	BOOST_CHECK_EQUAL(fp2.filename, "1cbs_map.mtz");
	BOOST_CHECK_EQUAL(fp2.mimetype, "text/plain");
	BOOST_CHECK_EQUAL(std::string(fp2.data, fp2.data + fp2.length), "And again, hello!\n");
}

// a very simple controller, serving only /test/one and /test/three
class my_controller : public zeep::http::controller
{
	public:
	my_controller() : zeep::http::controller("/test") {}

	virtual bool handle_request(zeep::http::request& req, zeep::http::reply& rep)
	{
		bool result = false;
		if (req.get_uri() == "/test/one" or req.get_uri() == "/test/three")
		{
			rep = zeep::http::reply::stock_reply(zeep::http::ok);
			result = true;
		}
		
		return result;
	}
};

BOOST_AUTO_TEST_CASE(webapp_7)
{
	// start up a http server and stop it again

	zh::daemon d([]() {
		auto s = new zh::server;
		s->add_controller(new my_controller());
		return s;
	}, "zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zh::daemon::run_foreground, d, "::", port));

	std::cerr << "started daemon at port " << port << std::endl;

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	try
	{
		auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::not_found);

		reply = simple_request(port, "XXX / HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::bad_request);

		reply = simple_request(port, "GET /test/one HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::ok);

		reply = simple_request(port, "GET /test/two HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::not_found);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}

// authentication test
BOOST_AUTO_TEST_CASE(server_with_security_1)
{
	class my_user_service : public zeep::http::user_service
	{
		virtual zeep::http::user_details load_user(const std::string& username) const
		{
			if (username != "scott")
				throw zeep::http::user_unknown_exception();

			return {
				username,
				m_pwenc.encode("tiger"),
				{ "admin" }
			};
		}

		zeep::http::pbkdf2_sha256_password_encoder m_pwenc;
	} users;

	std::string secret = "geheim";

	zh::daemon d([&]() {
		auto s = new zh::server(new zeep::http::security_context(secret, users, new zeep::http::pbkdf2_sha256_password_encoder()));
		s->add_controller(new my_controller());
		s->add_controller(new zeep::http::login_controller());

		auto& sec = s->get_security_context();

		sec.add_rule("/test/three", "admin");
		sec.add_rule("/**", {});

		return s;
		}, "zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zh::daemon::run_foreground, d, "::", port));

	std::cerr << "started daemon at port " << port << std::endl;

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	try
	{

		auto reply = simple_request(port, "XXX / HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::bad_request);

		reply = simple_request(port, "GET /test/one HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::ok);

		reply = simple_request(port, "GET /test/two HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::not_found);

		reply = simple_request(port, "GET /test/three HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::unauthorized);

		// now try to log in and see if we can access all of the above

		// we use a request object now, to store cookies
		zeep::http::request req{ "POST", "/login", {1, 0}, { { "Content-Type", "application/x-www-form-urlencoded" } }, "username=scott&password=tiger" };

		// first test is to send a POST to login, but without the csrf token
		reply = simple_request(port, req);
		BOOST_TEST(reply.get_status() == zh::forbidden);

		// OK, fetch the login form then and pry the csrf token out of it
		req.set_method("GET");
		reply = simple_request(port, req);
		BOOST_TEST(reply.get_status() == zh::ok);

		// copy the cookie
		auto csrfCookie = reply.get_cookie("csrf-token");
		req.set_cookie("csrf-token", csrfCookie);

		zeep::xml::document form(reply.get_content());
		auto csrf = form.find_first("//input[@name='_csrf']");
		BOOST_REQUIRE(csrf != nullptr);

		BOOST_TEST(form.find_first("//input[@name='username']") != nullptr);
		BOOST_TEST(form.find_first("//input[@name='password']") != nullptr);

		BOOST_TEST(csrf->get_attribute("value") == csrfCookie);

		// try again to authenticate
		req.set_method("POST");
		req.set_content("username=scott&password=tiger&_csrf=" + csrfCookie, "application/x-www-form-urlencoded");
		reply = simple_request(port, req);
		BOOST_TEST(reply.get_status() == zh::moved_temporarily);

		auto accessToken = reply.get_cookie("access_token");
		req.set_cookie("access_token", accessToken);

		// now try that admin page again
		req.set_uri("/test/three");
		req.set_method("GET");
		reply = simple_request(port, req);
		BOOST_TEST(reply.get_status() == zh::ok);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}


