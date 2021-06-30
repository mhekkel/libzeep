#define BOOST_TEST_MODULE URI_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/http/uri.hpp>

namespace z = zeep;

BOOST_AUTO_TEST_CASE(uri_1)
{
	zeep::http::is_valid_uri("http://a/");

	zeep::http::is_valid_uri("http://a:80/");

	zeep::http::is_valid_uri("http://a.b/");
	zeep::http::is_valid_uri("http://a/b");

	zeep::http::is_valid_uri("http://user@a/b");
	zeep::http::is_valid_uri("http://user:pass@a/b");
	zeep::http::is_valid_uri("http://user:pass@a:80/b");

	zeep::http::is_valid_uri("http://a?q");
	zeep::http::is_valid_uri("http://a#f");

	zeep::http::is_valid_uri("http://a/b?q");
	zeep::http::is_valid_uri("http://a/b#f");

	zeep::http::is_valid_uri("http://a/b/c?q");
	zeep::http::is_valid_uri("http://a/b/c#f");

	zeep::http::is_valid_uri("http://a/b/c.d?q");
	zeep::http::is_valid_uri("http://a/b/c.d#f");

	zeep::http::is_valid_uri("http://user@localhost/segment/index.html#frag");
	zeep::http::is_valid_uri("http://user@[::1]/segment/index.html#frag");
	zeep::http::is_valid_uri("http://user:pass@[::1]/segment/index.html#frag");

	zeep::http::is_valid_uri("http://user@localhost/segment/index.html?query");
	zeep::http::is_valid_uri("http://user@[::1]/segment/index.html?query");
	zeep::http::is_valid_uri("http://user:pass@[::1]/segment/index.html?query");

	zeep::http::is_valid_uri("http://user@localhost/segment/index.html?query#frag");
	zeep::http::is_valid_uri("http://user@[::1]/segment/index.html?query#frag");
	zeep::http::is_valid_uri("http://user:pass@[::1]/segment/index.html?query#frag");
}

BOOST_AUTO_TEST_CASE(uri_2)
{
	zeep::http::uri url("http://user:pass@[::1]/segment/index.html?query#frag");

	BOOST_CHECK_EQUAL(url.get_scheme(), "http");
	BOOST_CHECK_EQUAL(url.get_host(), "::1");
	BOOST_CHECK_EQUAL(url.get_path().string(), "segment/index.html");
	BOOST_CHECK_EQUAL(url.get_query(), "query");
	BOOST_CHECK_EQUAL(url.get_fragment(), "frag");
}

BOOST_AUTO_TEST_CASE(uri_3)
{
	zeep::http::uri url("http://www.example.com/~maarten");

	BOOST_CHECK_EQUAL(url.get_path().string(), "~maarten");
}

BOOST_AUTO_TEST_CASE(uri_4)
{
	zeep::http::uri url("http://www.example.com/%7Emaarten");

	BOOST_CHECK_EQUAL(url.get_path().string(), "~maarten");
}

BOOST_AUTO_TEST_CASE(uri_5)
{
	// This is a bit dubious... but it is valid according to RFC3986

	zeep::http::uri uri("http://a/b%0D%0ASet-Cookie:%20false");

	BOOST_CHECK_EQUAL(uri.get_path().string(), "b\r\nSet-Cookie: false");
}

BOOST_AUTO_TEST_CASE(uri_6)
{
	zeep::http::uri uri("file:/a/b");
	BOOST_CHECK(uri.is_absolute());

	zeep::http::uri uri2("file://a/b");
	BOOST_CHECK(not uri2.is_absolute());
}