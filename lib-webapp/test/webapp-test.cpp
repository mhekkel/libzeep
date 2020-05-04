#define BOOST_TEST_MODULE WebApp_Test
#include <boost/test/included/unit_test.hpp>

#include <boost/random/random_device.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/exception.hpp>
#include <zeep/crypto.hpp>

using namespace std;
namespace z = zeep;
namespace zx = zeep::xml;
namespace zh = zeep::http;

using webapp = zh::file_based_webapp;

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

		virtual void handle_test(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
		}

		virtual void handle_get_test(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("get", "text/plain");
		}

		virtual void handle_post_test(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("post", "text/plain");
		}

	} app;

	zh::request req;
	req.method = zh::method_type::GET;
	req.uri = "/test";

	zh::reply rep;

	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "get");

	req.method = zh::method_type::POST;
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "post");

	req.method = zh::method_type::DELETE;
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::not_found);
}

BOOST_AUTO_TEST_CASE(webapp_2)
{
	webapp app;

	app.mount("test", "my-realm", &webapp::handle_file);

	zh::request req;
	req.method = zh::method_type::GET;
	req.uri = "/test";

	zh::reply rep;

	app.handle_request(req, rep);

	BOOST_CHECK_EQUAL(rep.get_status(), zh::internal_server_error);
}

// digest authentication test
BOOST_AUTO_TEST_CASE(webapp_3)
{
	class my_webapp : public webapp
	{
	  public:
		virtual void handle_test(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
		}
	} app;

	auto validator = new zh::simple_digest_authentication_validation("mijn-realm", {
		{ "scott", "tiger" }
	});

	app.set_authenticator(validator, "mijn-realm");
	app.mount("test", "mijn-realm", &my_webapp::handle_test);

	zh::request req;
	req.method = zh::method_type::GET;
	req.uri = "/test";

	zh::reply rep;

	app.handle_request(req, rep);

	BOOST_CHECK_EQUAL(rep.get_status(), zh::unauthorized);

	auto wwwAuth = rep.get_header("WWW-Authenticate");

	std::regex rx(R"xx(Digest realm="mijn-realm", qop="auth", nonce="(.+)")xx");
	std::smatch m;

	BOOST_CHECK(std::regex_match(wwwAuth, m, rx));

	auto nonce = m[1].str();

	boost::random::random_device rng;
	auto nc = "1";

	auto ha1 = validator->get_hashed_password("scott");

	std::string cnonce = "x";

	std::string ha2 = zeep::encode_hex(zeep::md5("GET:/test"));
	std::string hash = zeep::encode_hex(zeep::md5(
								   ha1 + ':' +
								   nonce + ':' +
								   nc + ':' +
								   cnonce + ':' +
								   "auth" + ':' +
								   ha2));

	req.set_header("Authorization",
		"nonce=" + nonce + "," +
		"cnonce=x" + "," +
		"username=scott" + "," +
		"response=" + hash + "," +
		"qop=auth" + "," +
		"realm='mijn-realm'" + ","
		"nc=" + nc + ","
		"uri='/test'");

	zh::reply rep2;

	app.handle_request(req, rep2);

	BOOST_CHECK_EQUAL(rep2.get_status(), zh::ok);
}

// jws authentication test
BOOST_AUTO_TEST_CASE(webapp_3a)
{
	class my_webapp : public webapp
	{
	  public:
		virtual void handle_test(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
		}
	} app;

	auto secret = zeep::encode_hex(zeep::random_hash());

	auto validator = new zh::simple_jws_authentication_validation("mijn-realm", secret, {
		{ "scott", "tiger" }
	});

	app.set_authenticator(validator, true);
	app.mount("test", "mijn-realm", &my_webapp::handle_test);

	zh::request req;
	req.method = zh::method_type::GET;
	req.uri = "/test";

	zh::reply rep = {};

	app.handle_request(req, rep);

	BOOST_CHECK_EQUAL(rep.get_status(), zh::unauthorized);

	auto csrf = rep.get_cookie("csrf-token");

	// check if the login contains all the fields
	zeep::xml::document loginDoc(rep.get_content());
	BOOST_CHECK(loginDoc.find_first("//input[@name='username']") != nullptr);
	BOOST_CHECK(loginDoc.find_first("//input[@name='password']") != nullptr);
	BOOST_CHECK(loginDoc.find_first("//input[@name='_csrf']") != nullptr);
	if (loginDoc.find_first("//input[@name='_csrf']"))
		BOOST_CHECK_EQUAL(loginDoc.find_first("//input[@name='_csrf']")->attr("value"), csrf);

	req.method = zh::method_type::POST;
	req.uri = "/login";
	req.set_header("content-type", "application/x-www-form-urlencoded");
	req.payload = "username=scott&password=tiger&_csrf=" + csrf;

	rep = {};

	app.handle_request(req, rep);

	BOOST_CHECK_EQUAL(rep.get_status(), zh::moved_temporarily);
	auto cookie = rep.get_cookie("access_token");

	BOOST_CHECK(not cookie.empty());

	zh::request req2;
	
	req2.method = zh::method_type::GET;
	req2.uri = "/test";
	req2.set_cookie("access_token", cookie);

	zh::reply rep2;

	app.handle_request(req2, rep2);

	BOOST_CHECK_EQUAL(rep2.get_status(), zh::ok);
}


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

		virtual void handle_test1(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("1", "text/plain");
		}

		virtual void handle_test2(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("2", "text/plain");
		}

		virtual void handle_test2b(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("2b", "text/plain");
		}

		virtual void handle_test3(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("3", "text/plain");
		}

		virtual void handle_test4(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("4", "text/plain");
		}

		virtual void handle_testf(const zh::request& request, const zh::scope& scope, zh::reply& reply)
		{
			reply = zh::reply::stock_reply(zh::ok);
			reply.set_content("f", "text/plain");
		}


	} app;

	zh::request req;
	req.method = zh::method_type::GET;

	zh::reply rep;

	req.uri = "/test";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "1");

	req.uri = "/test/x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "3");

	req.uri = "/test/x/x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "4");

	req.uri = "iew.x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

	req.uri = "x/iew.x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2");

	req.uri = "x/x/iew.x";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "2b");

	req.uri = "css/styles/my-style.css";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "f");

	req.uri = "scripts/x.js";
	app.handle_request(req, rep);
	BOOST_CHECK_EQUAL(rep.get_status(), zh::ok);
	BOOST_CHECK_EQUAL(rep.get_content(), "f");
}
