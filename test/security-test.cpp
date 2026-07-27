// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/exception.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/security.hpp"
#include "zeep/uri.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <set>
#include <string>
#include <thread>
#include <tuple>

namespace zh = zeep::http;

TEST_CASE("sec_1")
{
	zh::reply rep;

	CHECK_THROWS_AS(rep = zh::reply::redirect("http://example.com\r\nSet-Cookie: wrong=false;"), zeep::exception);

	CHECK_THROWS_AS(rep = zh::reply::redirect("http://example.com%0D%0ASet-Cookie: wrong=false;"), zeep::exception);

	rep = zh::reply::redirect("http://example.com/%0D%0ASet-Cookie:%20wrong=false;");

	CHECK(rep.get_header("Location") == "http://example.com/%0D%0ASet-Cookie:%20wrong=false;");

	rep = zh::reply::redirect("http://example.com");

	CHECK(rep.get_header("Location") == "http://example.com");

	/*
	    std::clog << rep << '\n';

	    std::ostringstream os;
	    os << rep;

	    zh::reply_parser p;

	    std::string s = os.str();
	    zeep::char_streambuf sb(s.c_str(), s.length());

	    p.parse(sb);
	    auto r2 = p.get_reply();

	    std::clog << r2 << '\n';

	    BOOST_CHECK(r2.get_cookie("wrong").empty());
	*/
}

TEST_CASE("sec_2")
{
	zh::simple_user_service users({ { "scott", "tiger", { "USER" } } });

	zeep::http::security_context sc("1234", users, false);
	sc.add_rule("/**", { "USER" });

	auto user = users.load_user("scott");

	{
		// default expires one year from now

		zh::reply rep;
		sc.add_authorization_headers(rep, user);

		zh::request req{ "GET", "/" };
		req.set_cookie("access_token", rep.get_cookie("access_token"));

		CHECK_NOTHROW(sc.validate_request(req));
	}

	{
		// check with 1 second

		zh::reply rep;
		sc.add_authorization_headers(rep, user, std::chrono::seconds{ 1 });

		zh::request req{ "GET", "/" };
		req.set_cookie("access_token", rep.get_cookie("access_token"));

		std::this_thread::sleep_for(std::chrono::seconds{ 2 });

		CHECK_THROWS_AS(sc.validate_request(req), zeep::exception);
	}
}
