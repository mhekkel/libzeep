// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#if ZEEP_CXX_MODULE
import zeep;
#else
# include "zeep/exception.hpp"
# include "zeep/http/reply.hpp"
# include "zeep/http/request.hpp"
# include "zeep/http/security.hpp"
# include "zeep/uri.hpp"
#endif

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

TEST_CASE("sec_3 login rate limiting")
{
	// Use a low iteration count; these tests exercise rate limiting, not KDF cost.
	zeep::http::pbkdf2_sha256_password_encoder enc(1000, 32);
	auto encoded = enc.encode("tiger");

	zeep::http::simple_user_service users({ { "scott", encoded, { "USER" } } });
	zeep::http::security_context sc("1234", users, false);
	sc.set_max_login_attempts(3);

	// initially allowed
	CHECK(sc.login_attempt_allowed("scott"));

	sc.record_login_failure("scott");
	sc.record_login_failure("scott");
	CHECK(sc.login_attempt_allowed("scott")); // 2 failures < 3, still allowed
	sc.record_login_failure("scott");
	CHECK_FALSE(sc.login_attempt_allowed("scott")); // 3 failures -> locked out

	// a successful login clears the failure count
	sc.record_login_success("scott");
	CHECK(sc.login_attempt_allowed("scott"));
}

TEST_CASE("sec_4 login flow enforces rate limit")
{
	// Use a low iteration count; these tests exercise rate limiting, not KDF cost.
	zeep::http::pbkdf2_sha256_password_encoder enc(1000, 32);
	auto encoded = enc.encode("tiger");

	zeep::http::simple_user_service users({ { "scott", encoded, { "USER" } } });
	zeep::http::security_context sc("1234", users, false);
	sc.set_max_login_attempts(2);
	sc.set_dummy_password_iterations(1000);

	zh::reply rep;

	CHECK_THROWS_AS(sc.verify_username_password("scott", "wrong", rep), zeep::http::invalid_password_exception);
	CHECK_THROWS_AS(sc.verify_username_password("scott", "wrong", rep), zeep::http::invalid_password_exception);

	// locked out now, even the correct password is rejected
	CHECK_THROWS_AS(sc.verify_username_password("scott", "tiger", rep), zeep::http::invalid_password_exception);

	// unknown users go through the same (dummy) verification and are not distinguishable
	CHECK_THROWS_AS(sc.verify_username_password("bob", "whatever", rep), zeep::http::invalid_password_exception);
}

TEST_CASE("sec_5 default pbkdf2 iterations is 600k")
{
	// default constructor should use the OWASP-recommended 600k iterations
	zeep::http::pbkdf2_sha256_password_encoder enc;
	auto hash = enc.encode("tiger");

	// format: pbkdf2_sha256$<iterations>$<salt>$<hash>
	auto first = hash.find('$');
	auto second = hash.find('$', first + 1);
	REQUIRE(first != std::string::npos);
	REQUIRE(second != std::string::npos);
	CHECK(hash.substr(first + 1, second - first - 1) == "600000");
}

TEST_CASE("sec_6 tracked login failures are bounded")
{
	zeep::http::simple_user_service users({});
	zeep::http::security_context sc("1234", users, false);
	sc.set_max_login_attempts(1);
	sc.set_max_tracked_login_failures(10);

	// A flood of distinct random usernames must not grow the failure map
	// without bound: only the configured number of entries is kept.
	for (int i = 0; i < 1000; ++i)
		sc.record_login_failure("user-" + std::to_string(i));

	// the most recent -> still tracked (and above the attempt limit)
	CHECK_FALSE(sc.login_attempt_allowed("user-999"));

	// the oldest entries were evicted once the map hit its capacity
	CHECK(sc.login_attempt_allowed("user-0"));
}

TEST_CASE("sec_7 expired failures are pruned")
{
	zeep::http::simple_user_service users({});
	zeep::http::security_context sc("1234", users, false);
	sc.set_max_login_attempts(1);
	sc.set_login_lockout_duration(std::chrono::seconds{ 1 });

	sc.record_login_failure("scott");
	CHECK_FALSE(sc.login_attempt_allowed("scott"));

	// wait out the lockout window
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });

	// the expired entry is removed on the next query
	CHECK(sc.login_attempt_allowed("scott"));
}

TEST_CASE("sec_8 expired entry is re-recorded while others stay active")
{
	zeep::http::simple_user_service users({});
	zeep::http::security_context sc("1234", users, false);
	sc.set_max_login_attempts(1);
	sc.set_max_tracked_login_failures(3);
	sc.set_login_lockout_duration(std::chrono::seconds{ 1 });

	// fill the failure map up to its bound
	for (int i = 0; i < 3; ++i)
		sc.record_login_failure("user-" + std::to_string(i));

	// let those entries expire, then record fresh failures for new users
	// (the expired ones are pruned as a side effect)
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });
	sc.record_login_failure("user-3");
	sc.record_login_failure("user-4");

	// the newest failures are still tracked
	CHECK_FALSE(sc.login_attempt_allowed("user-3"));
	CHECK_FALSE(sc.login_attempt_allowed("user-4"));

	// the dangling-iterator path: re-record a user whose entry is still in
	// the map but has expired, with no intervening prune
	sc.record_login_failure("solo");
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });
	sc.record_login_failure("solo");

	// refreshed in place, counted again as a fresh failed login
	CHECK_FALSE(sc.login_attempt_allowed("solo"));
}
