#define BOOST_TEST_MODULE URI_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/http/uri.hpp>

namespace z = zeep;

BOOST_AUTO_TEST_CASE(uri_1)
{
	zeep::http::uri url1("http://a/");

	zeep::http::uri url2("http://a.b/");
	zeep::http::uri url3("http://a/b");
	zeep::http::uri url4("http://a/b%0A%0DSet-Cookie:%20false");
	zeep::http::uri url5("http://user:pass@[::1]/segment/index.html?query#frag");
}
