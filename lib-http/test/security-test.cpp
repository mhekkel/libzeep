#include "zeep/http/asio.hpp"

#define BOOST_TEST_MODULE Security_Test
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

BOOST_AUTO_TEST_CASE(sec_1)
{
	zh::reply rep;

	BOOST_CHECK_THROW(rep = zh::reply::redirect("http://example.com\r\nSet-Cookie: wrong=false;"), zeep::exception);

	BOOST_CHECK_THROW(rep = zh::reply::redirect("http://example.com%0D%0ASet-Cookie: wrong=false;"), zeep::exception);

	rep = zh::reply::redirect("http://example.com/%0D%0ASet-Cookie:%20wrong=false;");

	BOOST_CHECK_EQUAL(rep.get_header("Location"), "http://example.com/%0D%0ASet-Cookie:%20wrong=false;");

	rep = zh::reply::redirect("http://example.com");

	BOOST_CHECK_EQUAL(rep.get_header("Location"), "http://example.com");

/*
	std::cerr << rep << std::endl;

	std::ostringstream os;
	os << rep;

	zh::reply_parser p;

	std::string s = os.str();
	zeep::char_streambuf sb(s.c_str(), s.length());

	p.parse(sb);
	auto r2 = p.get_reply();

	std::cerr << r2 << std::endl;

	BOOST_CHECK(r2.get_cookie("wrong").empty());
*/
}

BOOST_AUTO_TEST_CASE(sec_2)
{
	zh::simple_user_service users({
		{ "scott", "tiger", { "USER" } }
	});

	zeep::http::security_context sc("1234", users, false);
	sc.add_rule("/**", { "USER" });

	auto user = users.load_user("scott");

	{
		// default expires one year from now

		zh::reply rep;
		sc.add_authorization_headers(rep, user);

		zh::request req{ "GET", "/" };
		req.set_cookie("access_token", rep.get_cookie("access_token"));

		BOOST_CHECK_NO_THROW(sc.validate_request(req));
	}


	{
		// check with 1 second

		zh::reply rep;
		sc.add_authorization_headers(rep, user, std::chrono::seconds{1});

		zh::request req{ "GET", "/" };
		req.set_cookie("access_token", rep.get_cookie("access_token"));

		std::this_thread::sleep_for(std::chrono::seconds{2});

		BOOST_CHECK_THROW(sc.validate_request(req), zeep::exception);
	}

}

