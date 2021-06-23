#define BOOST_TEST_MODULE URI_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/http/uri.hpp>

namespace z = zeep;

BOOST_AUTO_TEST_CASE(uri_1)
{
	zeep::http::is_valid_uri("http://a/");

	zeep::http::is_valid_uri("http://a.b/");
	zeep::http::is_valid_uri("http://a/b");
	zeep::http::is_valid_uri("http://a/b%0A%0DSet-Cookie:%20false");
	zeep::http::is_valid_uri("http://user:pass@[::1]/segment/index.html?query#frag");
}

BOOST_AUTO_TEST_CASE(uri_2)
{
	zeep::http::uri url("http://user:pass@[::1]/segment/index.html?query#frag");

	BOOST_CHECK_EQUAL(url.get_scheme(), "http");
	BOOST_CHECK_EQUAL(url.get_host(), "::1");
	BOOST_CHECK_EQUAL(url.get_path().string(), "segment/index.html");
	BOOST_CHECK_EQUAL(url.get_query(), "query");
}
