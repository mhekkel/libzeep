#include "test-main.hpp"

#include <zeep/http/uri.hpp>

namespace z = zeep;

TEST_CASE("cc_1")
{
	for (int ch = 0; ch <= 255; ++ch)
	{
		// std::cout << ch << ' ' << char(ch) << '\n';
		CHECK((std::isalpha(ch) != 0) == z::http::uri::is_scheme_start(ch));
		CHECK((std::isxdigit(ch) != 0) == z::http::uri::is_xdigit(ch));
	}
}

TEST_CASE("uri_1")
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

TEST_CASE("uri_2")
{
	zeep::http::uri url("http://user:pass@[::1]/segment/index.html?query#frag");

	CHECK(url.get_scheme() == "http");
	CHECK(url.get_host() == "[::1]");
	CHECK(url.get_path().string() == "/segment/index.html");
	CHECK(url.get_query(false) == "query");
	CHECK(url.get_fragment(false) == "frag");
}

TEST_CASE("uri_3")
{
	zeep::http::uri url("http://www.example.com/~maarten");

	CHECK(url.get_path().string() == "/~maarten");
}

TEST_CASE("uri_4")
{
	zeep::http::uri url("http://www.example.com/%7Emaarten");

	CHECK(url.get_path().string() == "/~maarten");
}

TEST_CASE("uri_5")
{
	// This is a bit dubious... but it is valid according to RFC3986

	zeep::http::uri uri("http://a/b%0D%0ASet-Cookie:%20false");

	CHECK(uri.get_segments().front() == "b\r\nSet-Cookie: false");
}

TEST_CASE("uri_6a")
{
	zeep::http::uri uri("file:/a/b");
	CHECK(uri.is_absolute());
	CHECK(uri.get_path().string() == "/a/b");
}

TEST_CASE("uri_6b")
{
	zeep::http::uri uri("file://a/b");
	CHECK(uri.is_absolute());
	CHECK(uri.get_host() == "a");
	CHECK(uri.get_path().string() == "/b");
}

TEST_CASE("uri_6c")
{
	CHECK_THROWS_AS(zeep::http::uri("file://a"), zeep::http::uri_parse_error);
	CHECK_THROWS_AS(zeep::http::uri("file://a?b"), zeep::http::uri_parse_error);
	CHECK_THROWS_AS(zeep::http::uri("file://a#c"), zeep::http::uri_parse_error);
}


TEST_CASE("normalize_1")
{
	zeep::http::uri base("http://a/b/c/d;p?q");

	CHECK(zeep::http::uri("g:h"    , base).string() == "g:h");
	CHECK(zeep::http::uri("g"      , base).string() == "http://a/b/c/g");
	CHECK(zeep::http::uri("./g"    , base).string() == "http://a/b/c/g");
	CHECK(zeep::http::uri("g/"     , base).string() == "http://a/b/c/g/");
	CHECK(zeep::http::uri("/g"     , base).string() == "http://a/g");
	CHECK(zeep::http::uri("//g"    , base).string() == "http://g");
	CHECK(zeep::http::uri("?y"     , base).string() == "http://a/b/c/d;p?y");
	CHECK(zeep::http::uri("g?y"    , base).string() == "http://a/b/c/g?y");
	CHECK(zeep::http::uri("#s"     , base).string() == "http://a/b/c/d;p?q#s");
	CHECK(zeep::http::uri("g#s"    , base).string() == "http://a/b/c/g#s");
	CHECK(zeep::http::uri("g?y#s"  , base).string() == "http://a/b/c/g?y#s");
	CHECK(zeep::http::uri(";x"     , base).string() == "http://a/b/c/;x");
	CHECK(zeep::http::uri("g;x"    , base).string() == "http://a/b/c/g;x");
	CHECK(zeep::http::uri("g;x?y#s", base).string() == "http://a/b/c/g;x?y#s");
	CHECK(zeep::http::uri(""       , base).string() == "http://a/b/c/d;p?q");
	CHECK(zeep::http::uri("."      , base).string() == "http://a/b/c/");
	CHECK(zeep::http::uri("./"     , base).string() == "http://a/b/c/");
	CHECK(zeep::http::uri(".."     , base).string() == "http://a/b/");
	CHECK(zeep::http::uri("../"    , base).string() == "http://a/b/");
	CHECK(zeep::http::uri("../g"   , base).string() == "http://a/b/g");
	CHECK(zeep::http::uri("../.."  , base).string() == "http://a/");
	CHECK(zeep::http::uri("../../" , base).string() == "http://a/");
	CHECK(zeep::http::uri("../../g", base).string() == "http://a/g");
}

TEST_CASE("normalize_2")
{
	zeep::http::uri base("http://a/b/c/d;p?q");


	CHECK(zeep::http::uri("../../../g"   , base).string() == "http://a/g");
	CHECK(zeep::http::uri("../../../../g", base).string() == "http://a/g");
	CHECK(zeep::http::uri("/./g"         , base).string() == "http://a/g");
	CHECK(zeep::http::uri("/../g"        , base).string() == "http://a/g");
	CHECK(zeep::http::uri("g."           , base).string() == "http://a/b/c/g.");
	CHECK(zeep::http::uri(".g"           , base).string() == "http://a/b/c/.g");
	CHECK(zeep::http::uri("g.."          , base).string() == "http://a/b/c/g..");
	CHECK(zeep::http::uri("..g"          , base).string() == "http://a/b/c/..g");
	CHECK(zeep::http::uri("./../g"       , base).string() == "http://a/b/g");
	CHECK(zeep::http::uri("./g/."        , base).string() == "http://a/b/c/g/");
	CHECK(zeep::http::uri("g/./h"        , base).string() == "http://a/b/c/g/h");
	CHECK(zeep::http::uri("g/../h"       , base).string() == "http://a/b/c/h");
	CHECK(zeep::http::uri("g;x=1/./y"    , base).string() == "http://a/b/c/g;x=1/y");
	CHECK(zeep::http::uri("g;x=1/../y"   , base).string() == "http://a/b/c/y");
	CHECK(zeep::http::uri("g?y/./x"      , base).string() == "http://a/b/c/g?y/./x");
	CHECK(zeep::http::uri("g?y/../x"     , base).string() == "http://a/b/c/g?y/../x");
	CHECK(zeep::http::uri("g#s/./x"      , base).string() == "http://a/b/c/g#s/./x");
	CHECK(zeep::http::uri("g#s/../x"     , base).string() == "http://a/b/c/g#s/../x");
	        //    ; for strict parsers
	CHECK(zeep::http::uri("http:g"       , base).string() == "http:g");
                    //   /  "http://a/b/c/g" ; for backward compatibility
}

TEST_CASE("path_1")
{
	zeep::http::uri t("http://a/b");

	t.set_path("c");			CHECK(t.get_path().string() == "c");
	t.set_path("/c");			CHECK(t.get_path().string() == "/c");
	t.set_path("/c/");			CHECK(t.get_path().string() == "/c/");
	t.set_path("c/d");			CHECK(t.get_path().string() == "c/d");
	t.set_path("/c/d");			CHECK(t.get_path().string() == "/c/d");
	t.set_path("/c/d/");		CHECK(t.get_path().string() == "/c/d/");
}

TEST_CASE("path_2")
{
	zeep::http::uri t("http://a/b");
	zeep::http::uri u;

	u = t / zeep::http::uri("c");			CHECK(u.string() == "http://a/b/c");
	u = t / zeep::http::uri("/c");			CHECK(u.string() == "http://a/b/c");
	u = t / zeep::http::uri("/c/");			CHECK(u.string() == "http://a/b/c/");
	u = t / zeep::http::uri("c/d");			CHECK(u.string() == "http://a/b/c/d");
	u = t / zeep::http::uri("/c/d");		CHECK(u.string() == "http://a/b/c/d");
	u = t / zeep::http::uri("/c/d/");		CHECK(u.string() == "http://a/b/c/d/");
}

TEST_CASE("relative_1")
{
	zeep::http::uri base("http://a/b/c/d;p?q");
	zeep::http::uri u;

	CHECK(zeep::http::uri("g:h"                 ).relative(base).string() == "g:h");
	CHECK(zeep::http::uri("http://a/b/c/g"      ).relative(base).string() == "g");
	CHECK(zeep::http::uri("http://a/b/c/g/"     ).relative(base).string() == "g/");
	CHECK(zeep::http::uri("http://a/g"          ).relative(base).string() == "/g");
	CHECK(zeep::http::uri("http://g"            ).relative(base).string() == "//g");
	CHECK(zeep::http::uri("http://a/b/c/d;p?y"  ).relative(base).string() == "?y");
	CHECK(zeep::http::uri("http://a/b/c/g?y"    ).relative(base).string() == "g?y");
	CHECK(zeep::http::uri("http://a/b/c/d;p?q#s").relative(base).string() == "#s");
	CHECK(zeep::http::uri("http://a/b/c/g#s"    ).relative(base).string() == "g#s");
	CHECK(zeep::http::uri("http://a/b/c/g?y#s"  ).relative(base).string() == "g?y#s");
	CHECK(zeep::http::uri("http://a/b/c/;x"     ).relative(base).string() == ";x");
	CHECK(zeep::http::uri("http://a/b/c/g;x"    ).relative(base).string() == "g;x");
	CHECK(zeep::http::uri("http://a/b/c/g;x?y#s").relative(base).string() == "g;x?y#s");
	CHECK(zeep::http::uri("http://a/b/c/d;p?q"  ).relative(base).string() == "");
	CHECK(zeep::http::uri("http://a/b/c/"       ).relative(base).string() == ".");
	CHECK(zeep::http::uri("http://a/b/"         ).relative(base).string() == "..");
	CHECK(zeep::http::uri("http://a/b/g"        ).relative(base).string() == "../g");
}

TEST_CASE("relative_2")
{
	zeep::http::uri base("http://a/b/c/d;p?q");
	zeep::http::uri u;

	CHECK(zeep::http::uri(zeep::http::uri("g:h"                 ).relative(base).string(), base).string() == "g:h"                 );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/g"      ).relative(base).string(), base).string() == "http://a/b/c/g"      );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/g/"     ).relative(base).string(), base).string() == "http://a/b/c/g/"     );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/g"          ).relative(base).string(), base).string() == "http://a/g"          );
	CHECK(zeep::http::uri(zeep::http::uri("http://g"            ).relative(base).string(), base).string() == "http://g"            );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/d;p?y"  ).relative(base).string(), base).string() == "http://a/b/c/d;p?y"  );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/g?y"    ).relative(base).string(), base).string() == "http://a/b/c/g?y"    );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/d;p?q#s").relative(base).string(), base).string() == "http://a/b/c/d;p?q#s");
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/g#s"    ).relative(base).string(), base).string() == "http://a/b/c/g#s"    );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/g?y#s"  ).relative(base).string(), base).string() == "http://a/b/c/g?y#s"  );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/;x"     ).relative(base).string(), base).string() == "http://a/b/c/;x"     );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/g;x"    ).relative(base).string(), base).string() == "http://a/b/c/g;x"    );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/g;x?y#s").relative(base).string(), base).string() == "http://a/b/c/g;x?y#s");
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/d;p?q"  ).relative(base).string(), base).string() == "http://a/b/c/d;p?q"  );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/c/"       ).relative(base).string(), base).string() == "http://a/b/c/"       );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/"         ).relative(base).string(), base).string() == "http://a/b/"         );
	CHECK(zeep::http::uri(zeep::http::uri("http://a/b/g"        ).relative(base).string(), base).string() == "http://a/b/g"        );
}

TEST_CASE("encoding_1")
{
	// http://a/höken/Ðuh?¤
	zeep::http::uri u("http://a/h%C3%B6ken/%C3%90uh?%C2%A4");

	CHECK(zeep::http::decode_url(u.get_path().string()) == "/höken/Ðuh");
	CHECK(zeep::http::decode_url(u.get_query(false)) == "¤");
	CHECK(u.get_query(true) == "¤");
}