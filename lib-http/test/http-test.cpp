#include "zeep/http/asio.hpp"

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

// BOOST_AUTO_TEST_CASE(connection_read)
// {
// #pragma message("write test for avail/used")
// }

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
		"Content-Type: chemical/x-cif\r\n"
		"\r\n"
		"hello, world!\n\r\n"
		"--xYzZY\r\n"
		"Content-Disposition: form-data; name=\"mtz-file\"; filename=\"1cbs_map.mtz\"\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"And again, hello!\n\r\n"
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

	std::clog << "started daemon at port " << port << '\n';

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
		std::clog << e.what() << '\n';
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

	std::clog << "started daemon at port " << port << '\n';

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
		BOOST_REQUIRE(reply.get_status() == zh::ok);

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
		BOOST_TEST(reply.get_status() == zh::see_other);

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
		std::clog << e.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(long_filename_test_1)
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

	std::clog << "started daemon at port " << port << '\n';

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	try
	{
		auto reply = simple_request(port, "GET /%E3%80%82%E7%84%B6%E8%80%8C%EF%BC%8C%E9%9C%80%E8%A6%81%E6%B3%A8%E6%84%8F%E7%9A%84%E6%98%AF%EF%BC%8C%E8%AF%A5%E7%BD%91%E7%AB%99%E5%B7%B2%E7%BB%8F%E5%BE%88%E4%B9%85%E6%B2%A1%E6%9C%89%E6%9B%B4%E6%96%B0%E4%BA%86%EF%BC%8C%E5%9B%A0%E6%AD%A4%E5%8F%AF%E8%83%BD%E6%97%A0%E6%B3%95%E6%8F%90%E4%BE%9B%E6%9C%80%E6%96%B0%E7%9A%84%E8%BD%AF%E4%BB%B6%E7%89%88%E6%9C%AC%E5%92%8C%E7%9B%B8%E5%85%B3%E8%B5%84%E6%BA%90%E3%80%82 HTTP/1.1\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::not_found);
	}
	catch (const std::exception& e)
	{
		std::clog << e.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(pen_test_resilience_1)
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

	std::clog << "started daemon at port " << port << '\n';

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	try
	{
		auto reply = simple_request(port, "GET //plus/mytag_js.php?aid=9999&nocache=90sec HTTP/1.1\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::not_found);


		reply = simple_request(port, "GET //plus/erraddsave.php?dopost=saveedit&a=b&arrs1[]=99&c=d&arrs1[]=102&arrs1[]=103&arrs1[]=95&arrs1[]=100&arrs1[]=98&arrs1[]=112&arrs1[]=114&arrs1[]=101&arrs1[]=102&arrs1[]=105&arrs1[]=120&arrs2[]=109&arrs2[]=121&arrs2[]=97&arrs2[]=100&arrs2[]=96&arrs2[]=32&arrs2[]=40&arrs2[]=97&arrs2[]=105&arrs2[]=100&arrs2[]=44&arrs2[]=110&arrs2[]=111&arrs2[]=114&arrs2[]=109&arrs2[]=98&arrs2[]=111&arrs2[]=100&arrs2[]=121&arrs2[]=41&arrs2[]=32&arrs2[]=86&arrs2[]=65&arrs2[]=76&arrs2[]=85&arrs2[]=69&arrs2[]=83&arrs2[]=40&arrs2[]=56&arrs2[]=56&arrs2[]=56&arrs2[]=56&arrs2[]=44&arrs2[]=39&arrs2[]=60&arrs2[]=63&arrs2[]=112&arrs2[]=104&arrs2[]=112&arrs2[]=32&arrs2[]=105&arrs2[]=102&arrs2[]=40&arrs2[]=105&arrs2[]=115&arrs2[]=115&arrs2[]=101&arrs2[]=116&arrs2[]=40&arrs2[]=36&arrs2[]=95&arrs2[]=80&arrs2[]=79&arrs2[]=83&arrs2[]=84&arrs2[]=91&arrs2[]=39&arrs2[]=39&arrs2[]=108&arrs2[]=101&arrs2[]=109&arrs2[]=111&arrs2[]=110&arrs2[]=39&arrs2[]=39&arrs2[]=93&arrs2[]=41&arrs2[]=41&arrs2[]=123&arrs2[]=36&arrs2[]=97&arrs2[]=61&arrs2[]=115&arrs2[]=116&arrs2[]=114&arrs2[]=114&arrs2[]=101&arrs2[]=118&arrs2[]=40&arrs2[]=39&arrs2[]=39&arrs2[]=101&arrs2[]=99&arrs2[]=97&arrs2[]=108&arrs2[]=112&arrs2[]=101&arrs2[]=114&arrs2[]=95&arrs2[]=103&arrs2[]=101&arrs2[]=114&arrs2[]=112&arrs2[]=39&arrs2[]=39&arrs2[]=41&arrs2[]=59&arrs2[]=36&arrs2[]=98&arrs2[]=61&arrs2[]=115&arrs2[]=116&arrs2[]=114&arrs2[]=114&arrs2[]=101&arrs2[]=118&arrs2[]=40&arrs2[]=39&arrs2[]=39&arrs2[]=101&arrs2[]=100&arrs2[]=111&arrs2[]=99&arrs2[]=101&arrs2[]=100&arrs2[]=95&arrs2[]=52&arrs2[]=54&arrs2[]=101&arrs2[]=115&arrs2[]=97&arrs2[]=98&arrs2[]=39&arrs2[]=39&arrs2[]=41&arrs2[]=59&arrs2[]=36&arrs2[]=97&arrs2[]=40&arrs2[]=39&arrs2[]=39&arrs2[]=47&arrs2[]=94&arrs2[]=47&arrs2[]=101&arrs2[]=39&arrs2[]=39&arrs2[]=44&arrs2[]=36&arrs2[]=98&arrs2[]=40&arrs2[]=39&arrs2[]=39&arrs2[]=90&arrs2[]=88&arrs2[]=90&arrs2[]=104&arrs2[]=98&arrs2[]=67&arrs2[]=104&arrs2[]=105&arrs2[]=89&arrs2[]=88&arrs2[]=78&arrs2[]=108&arrs2[]=78&arrs2[]=106&arrs2[]=82&arrs2[]=102&arrs2[]=90&arrs2[]=71&arrs2[]=86&arrs2[]=106&arrs2[]=98&arrs2[]=50&arrs2[]=82&arrs2[]=108&arrs2[]=75&arrs2[]=67&arrs2[]=82&arrs2[]=102&arrs2[]=85&arrs2[]=107&arrs2[]=86&arrs2[]=82&arrs2[]=86&arrs2[]=85&arrs2[]=86&arrs2[]=84&arrs2[]=86&arrs2[]=70&arrs2[]=116&arrs2[]=54&arrs2[]=77&arrs2[]=70&arrs2[]=48&arrs2[]=112&arrs2[]=75&arrs2[]=81&arrs2[]=61&arrs2[]=61&arrs2[]=39&arrs2[]=39&arrs2[]=41&arrs2[]=44&arrs2[]=48&arrs2[]=41&arrs2[]=59&arrs2[]=125&arrs2[]=63&arrs2[]=62&arrs2[]=39&arrs2[]=41&arrs2[]=59&arrs2[]=0 HTTP/1.1\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::bad_request);
	}
	catch (const std::exception& e)
	{
		std::clog << e.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();


}