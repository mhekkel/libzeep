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
	BOOST_CHECK_EQUAL(url.get_host(), "[::1]");
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

// BOOST_AUTO_TEST_CASE(uri_5)
// {
// 	// This is a bit dubious... but it is valid according to RFC3986

// 	zeep::http::uri uri("http://a/b%0D%0ASet-Cookie:%20false");

// 	BOOST_CHECK_EQUAL(uri.get_path().string(), "b\r\nSet-Cookie: false");
// }

BOOST_AUTO_TEST_CASE(uri_6a)
{
	zeep::http::uri uri("file:/a/b");
	BOOST_CHECK(uri.is_absolute());
	BOOST_TEST(uri.get_path().string() == "/a/b");
}

BOOST_AUTO_TEST_CASE(uri_6b)
{
	zeep::http::uri uri("file://a/b");
	BOOST_CHECK(not uri.is_absolute());
	BOOST_TEST(uri.get_path().string() == "b");
}

BOOST_AUTO_TEST_CASE(normalize_1)
{
	zeep::http::uri base("http://a/b/c/d;p?q");

	BOOST_TEST(zeep::http::uri("g:h"    , base).string() == "g:h");
	BOOST_TEST(zeep::http::uri("g"      , base).string() == "http://a/b/c/g");
	BOOST_TEST(zeep::http::uri("./g"    , base).string() == "http://a/b/c/g");
	BOOST_TEST(zeep::http::uri("g/"     , base).string() == "http://a/b/c/g/");
	BOOST_TEST(zeep::http::uri("/g"     , base).string() == "http://a/g");
	BOOST_TEST(zeep::http::uri("//g"    , base).string() == "http://g");
	BOOST_TEST(zeep::http::uri("?y"     , base).string() == "http://a/b/c/d;p?y");
	BOOST_TEST(zeep::http::uri("g?y"    , base).string() == "http://a/b/c/g?y");
	BOOST_TEST(zeep::http::uri("#s"     , base).string() == "http://a/b/c/d;p?q#s");
	BOOST_TEST(zeep::http::uri("g#s"    , base).string() == "http://a/b/c/g#s");
	BOOST_TEST(zeep::http::uri("g?y#s"  , base).string() == "http://a/b/c/g?y#s");
	BOOST_TEST(zeep::http::uri(";x"     , base).string() == "http://a/b/c/;x");
	BOOST_TEST(zeep::http::uri("g;x"    , base).string() == "http://a/b/c/g;x");
	BOOST_TEST(zeep::http::uri("g;x?y#s", base).string() == "http://a/b/c/g;x?y#s");
	BOOST_TEST(zeep::http::uri(""       , base).string() == "http://a/b/c/d;p?q");
	BOOST_TEST(zeep::http::uri("."      , base).string() == "http://a/b/c/");
	BOOST_TEST(zeep::http::uri("./"     , base).string() == "http://a/b/c/");
	BOOST_TEST(zeep::http::uri(".."     , base).string() == "http://a/b/");
	BOOST_TEST(zeep::http::uri("../"    , base).string() == "http://a/b/");
	BOOST_TEST(zeep::http::uri("../g"   , base).string() == "http://a/b/g");
	BOOST_TEST(zeep::http::uri("../.."  , base).string() == "http://a/");
	BOOST_TEST(zeep::http::uri("../../" , base).string() == "http://a/");
	BOOST_TEST(zeep::http::uri("../../g", base).string() == "http://a/g");
}

BOOST_AUTO_TEST_CASE(normalize_2)
{
	zeep::http::uri base("http://a/b/c/d;p?q");


	BOOST_TEST(zeep::http::uri("../../../g"   , base).string() == "http://a/g");
	BOOST_TEST(zeep::http::uri("../../../../g", base).string() == "http://a/g");
	BOOST_TEST(zeep::http::uri("/./g"         , base).string() == "http://a/g");
	BOOST_TEST(zeep::http::uri("/../g"        , base).string() == "http://a/g");
	BOOST_TEST(zeep::http::uri("g."           , base).string() == "http://a/b/c/g.");
	BOOST_TEST(zeep::http::uri(".g"           , base).string() == "http://a/b/c/.g");
	BOOST_TEST(zeep::http::uri("g.."          , base).string() == "http://a/b/c/g..");
	BOOST_TEST(zeep::http::uri("..g"          , base).string() == "http://a/b/c/..g");
	BOOST_TEST(zeep::http::uri("./../g"       , base).string() == "http://a/b/g");
	BOOST_TEST(zeep::http::uri("./g/."        , base).string() == "http://a/b/c/g/");
	BOOST_TEST(zeep::http::uri("g/./h"        , base).string() == "http://a/b/c/g/h");
	BOOST_TEST(zeep::http::uri("g/../h"       , base).string() == "http://a/b/c/h");
	BOOST_TEST(zeep::http::uri("g;x=1/./y"    , base).string() == "http://a/b/c/g;x=1/y");
	BOOST_TEST(zeep::http::uri("g;x=1/../y"   , base).string() == "http://a/b/c/y");
	BOOST_TEST(zeep::http::uri("g?y/./x"      , base).string() == "http://a/b/c/g?y/./x");
	BOOST_TEST(zeep::http::uri("g?y/../x"     , base).string() == "http://a/b/c/g?y/../x");
	BOOST_TEST(zeep::http::uri("g#s/./x"      , base).string() == "http://a/b/c/g#s/./x");
	BOOST_TEST(zeep::http::uri("g#s/../x"     , base).string() == "http://a/b/c/g#s/../x");
	        //    ; for strict parsers
	BOOST_TEST(zeep::http::uri("http:g"       , base).string() == "http:g");
                    //   /  "http://a/b/c/g" ; for backward compatibility
}
