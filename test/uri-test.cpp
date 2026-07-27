// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include <catch2/catch_test_macros.hpp>

#include <zeep/uri.hpp>

#include <cctype>
#include <string>
#include <vector>

TEST_CASE("cc_1")
{
	for (int ch = 0; ch <= 255; ++ch)
	{
		// std::cout << ch << ' ' << char(ch) << '\n';
		CHECK((std::isalpha(ch) != 0) == zeep::uri::is_scheme_start(ch));
		CHECK((std::isxdigit(ch) != 0) == zeep::uri::is_xdigit(ch));
	}
}

TEST_CASE("uri_1")
{
	zeep::is_valid_uri("http://a/");

	zeep::is_valid_uri("http://a:80/");

	zeep::is_valid_uri("http://a.b/");
	zeep::is_valid_uri("http://a/b");

	zeep::is_valid_uri("http://user@a/b");
	zeep::is_valid_uri("http://user:pass@a/b");
	zeep::is_valid_uri("http://user:pass@a:80/b");

	zeep::is_valid_uri("http://a?q");
	zeep::is_valid_uri("http://a#f");

	zeep::is_valid_uri("http://a/b?q");
	zeep::is_valid_uri("http://a/b#f");

	zeep::is_valid_uri("http://a/b/c?q");
	zeep::is_valid_uri("http://a/b/c#f");

	zeep::is_valid_uri("http://a/b/c.d?q");
	zeep::is_valid_uri("http://a/b/c.d#f");

	zeep::is_valid_uri("http://user@localhost/segment/index.html#frag");
	zeep::is_valid_uri("http://user@[::1]/segment/index.html#frag");
	zeep::is_valid_uri("http://user:pass@[::1]/segment/index.html#frag");

	zeep::is_valid_uri("http://user@localhost/segment/index.html?query");
	zeep::is_valid_uri("http://user@[::1]/segment/index.html?query");
	zeep::is_valid_uri("http://user:pass@[::1]/segment/index.html?query");

	zeep::is_valid_uri("http://user@localhost/segment/index.html?query#frag");
	zeep::is_valid_uri("http://user@[::1]/segment/index.html?query#frag");
	zeep::is_valid_uri("http://user:pass@[::1]/segment/index.html?query#frag");
}

TEST_CASE("uri_2")
{
	zeep::uri url("http://user:pass@[::1]/segment/index.html?query#frag");

	CHECK(url.get_scheme() == "http");
	CHECK(url.get_host() == "[::1]");
	CHECK(url.get_path().string() == "/segment/index.html");
	CHECK(url.get_query(false) == "query");
	CHECK(url.get_fragment(false) == "frag");
}

TEST_CASE("uri_3")
{
	zeep::uri url("http://www.example.com/~maarten");

	CHECK(url.get_path().string() == "/~maarten");
}

TEST_CASE("uri_4")
{
	zeep::uri url("http://www.example.com/%7Emaarten");

	CHECK(url.get_path().string() == "/~maarten");
}

TEST_CASE("uri_5")
{
	// This is a bit dubious... but it is valid according to RFC3986

	zeep::uri uri("http://a/b%0D%0ASet-Cookie:%20false");

	CHECK(uri.get_segments().front() == "b\r\nSet-Cookie: false");
}

TEST_CASE("uri_6a")
{
	zeep::uri uri("file:/a/b");
	CHECK(uri.is_absolute());
	CHECK(uri.get_path().string() == "/a/b");
}

TEST_CASE("uri_6b")
{
	zeep::uri uri("file://a/b");
	CHECK(uri.is_absolute());
	CHECK(uri.get_host() == "a");
	CHECK(uri.get_path().string() == "/b");
}

TEST_CASE("uri_6c")
{
	CHECK_THROWS_AS(zeep::uri("file://a"), zeep::uri_parse_error);
	CHECK_THROWS_AS(zeep::uri("file://a?b"), zeep::uri_parse_error);
	CHECK_THROWS_AS(zeep::uri("file://a#c"), zeep::uri_parse_error);
}

TEST_CASE("normalize_1")
{
	zeep::uri base("http://a/b/c/d;p?q");

	CHECK(zeep::uri("g:h", base).string() == "g:h");
	CHECK(zeep::uri("g", base).string() == "http://a/b/c/g");
	CHECK(zeep::uri("./g", base).string() == "http://a/b/c/g");
	CHECK(zeep::uri("g/", base).string() == "http://a/b/c/g/");
	CHECK(zeep::uri("/g", base).string() == "http://a/g");
	CHECK(zeep::uri("//g", base).string() == "http://g");
	CHECK(zeep::uri("?y", base).string() == "http://a/b/c/d;p?y");
	CHECK(zeep::uri("g?y", base).string() == "http://a/b/c/g?y");
	CHECK(zeep::uri("#s", base).string() == "http://a/b/c/d;p?q#s");
	CHECK(zeep::uri("g#s", base).string() == "http://a/b/c/g#s");
	CHECK(zeep::uri("g?y#s", base).string() == "http://a/b/c/g?y#s");
	CHECK(zeep::uri(";x", base).string() == "http://a/b/c/;x");
	CHECK(zeep::uri("g;x", base).string() == "http://a/b/c/g;x");
	CHECK(zeep::uri("g;x?y#s", base).string() == "http://a/b/c/g;x?y#s");
	CHECK(zeep::uri("", base).string() == "http://a/b/c/d;p?q");
	CHECK(zeep::uri(".", base).string() == "http://a/b/c/");
	CHECK(zeep::uri("./", base).string() == "http://a/b/c/");
	CHECK(zeep::uri("..", base).string() == "http://a/b/");
	CHECK(zeep::uri("../", base).string() == "http://a/b/");
	CHECK(zeep::uri("../g", base).string() == "http://a/b/g");
	CHECK(zeep::uri("../..", base).string() == "http://a/");
	CHECK(zeep::uri("../../", base).string() == "http://a/");
	CHECK(zeep::uri("../../g", base).string() == "http://a/g");
}

TEST_CASE("normalize_2")
{
	zeep::uri base("http://a/b/c/d;p?q");

	CHECK(zeep::uri("../../../g", base).string() == "http://a/g");
	CHECK(zeep::uri("../../../../g", base).string() == "http://a/g");
	CHECK(zeep::uri("/./g", base).string() == "http://a/g");
	CHECK(zeep::uri("/../g", base).string() == "http://a/g");
	CHECK(zeep::uri("g.", base).string() == "http://a/b/c/g.");
	CHECK(zeep::uri(".g", base).string() == "http://a/b/c/.g");
	CHECK(zeep::uri("g..", base).string() == "http://a/b/c/g..");
	CHECK(zeep::uri("..g", base).string() == "http://a/b/c/..g");
	CHECK(zeep::uri("./../g", base).string() == "http://a/b/g");
	CHECK(zeep::uri("./g/.", base).string() == "http://a/b/c/g/");
	CHECK(zeep::uri("g/./h", base).string() == "http://a/b/c/g/h");
	CHECK(zeep::uri("g/../h", base).string() == "http://a/b/c/h");
	CHECK(zeep::uri("g;x=1/./y", base).string() == "http://a/b/c/g;x=1/y");
	CHECK(zeep::uri("g;x=1/../y", base).string() == "http://a/b/c/y");
	CHECK(zeep::uri("g?y/./x", base).string() == "http://a/b/c/g?y/./x");
	CHECK(zeep::uri("g?y/../x", base).string() == "http://a/b/c/g?y/../x");
	CHECK(zeep::uri("g#s/./x", base).string() == "http://a/b/c/g#s/./x");
	CHECK(zeep::uri("g#s/../x", base).string() == "http://a/b/c/g#s/../x");
	//    ; for strict parsers
	CHECK(zeep::uri("http:g", base).string() == "http:g");
	//   /  "http://a/b/c/g" ; for backward compatibility
}

TEST_CASE("path_1")
{
	zeep::uri t("http://a/b");

	t.set_path("c");
	CHECK(t.get_path().string() == "c");
	t.set_path("/c");
	CHECK(t.get_path().string() == "/c");
	t.set_path("/c/");
	CHECK(t.get_path().string() == "/c/");
	t.set_path("c/d");
	CHECK(t.get_path().string() == "c/d");
	t.set_path("/c/d");
	CHECK(t.get_path().string() == "/c/d");
	t.set_path("/c/d/");
	CHECK(t.get_path().string() == "/c/d/");
}

TEST_CASE("path_2")
{
	zeep::uri t("http://a/b");
	zeep::uri u;

	u = t / zeep::uri("c");
	CHECK(u.string() == "http://a/b/c");
	u = t / zeep::uri("/c");
	CHECK(u.string() == "http://a/b/c");
	u = t / zeep::uri("/c/");
	CHECK(u.string() == "http://a/b/c/");
	u = t / zeep::uri("c/d");
	CHECK(u.string() == "http://a/b/c/d");
	u = t / zeep::uri("/c/d");
	CHECK(u.string() == "http://a/b/c/d");
	u = t / zeep::uri("/c/d/");
	CHECK(u.string() == "http://a/b/c/d/");
}

TEST_CASE("relative_1")
{
	zeep::uri base("http://a/b/c/d;p?q");
	zeep::uri u;

	CHECK(zeep::uri("g:h").relative(base).string() == "g:h");
	CHECK(zeep::uri("http://a/b/c/g").relative(base).string() == "g");
	CHECK(zeep::uri("http://a/b/c/g/").relative(base).string() == "g/");
	CHECK(zeep::uri("http://a/g").relative(base).string() == "/g");
	CHECK(zeep::uri("http://g").relative(base).string() == "//g");
	CHECK(zeep::uri("http://a/b/c/d;p?y").relative(base).string() == "?y");
	CHECK(zeep::uri("http://a/b/c/g?y").relative(base).string() == "g?y");
	CHECK(zeep::uri("http://a/b/c/d;p?q#s").relative(base).string() == "#s");
	CHECK(zeep::uri("http://a/b/c/g#s").relative(base).string() == "g#s");
	CHECK(zeep::uri("http://a/b/c/g?y#s").relative(base).string() == "g?y#s");
	CHECK(zeep::uri("http://a/b/c/;x").relative(base).string() == ";x");
	CHECK(zeep::uri("http://a/b/c/g;x").relative(base).string() == "g;x");
	CHECK(zeep::uri("http://a/b/c/g;x?y#s").relative(base).string() == "g;x?y#s");
	CHECK(zeep::uri("http://a/b/c/d;p?q").relative(base).string() == "");
	CHECK(zeep::uri("http://a/b/c/").relative(base).string() == ".");
	CHECK(zeep::uri("http://a/b/").relative(base).string() == "..");
	CHECK(zeep::uri("http://a/b/g").relative(base).string() == "../g");
}

TEST_CASE("relative_2")
{
	zeep::uri base("http://a/b/c/d;p?q");
	zeep::uri u;

	CHECK(zeep::uri(zeep::uri("g:h").relative(base).string(), base).string() == "g:h");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/g").relative(base).string(), base).string() == "http://a/b/c/g");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/g/").relative(base).string(), base).string() == "http://a/b/c/g/");
	CHECK(zeep::uri(zeep::uri("http://a/g").relative(base).string(), base).string() == "http://a/g");
	CHECK(zeep::uri(zeep::uri("http://g").relative(base).string(), base).string() == "http://g");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/d;p?y").relative(base).string(), base).string() == "http://a/b/c/d;p?y");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/g?y").relative(base).string(), base).string() == "http://a/b/c/g?y");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/d;p?q#s").relative(base).string(), base).string() == "http://a/b/c/d;p?q#s");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/g#s").relative(base).string(), base).string() == "http://a/b/c/g#s");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/g?y#s").relative(base).string(), base).string() == "http://a/b/c/g?y#s");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/;x").relative(base).string(), base).string() == "http://a/b/c/;x");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/g;x").relative(base).string(), base).string() == "http://a/b/c/g;x");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/g;x?y#s").relative(base).string(), base).string() == "http://a/b/c/g;x?y#s");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/d;p?q").relative(base).string(), base).string() == "http://a/b/c/d;p?q");
	CHECK(zeep::uri(zeep::uri("http://a/b/c/").relative(base).string(), base).string() == "http://a/b/c/");
	CHECK(zeep::uri(zeep::uri("http://a/b/").relative(base).string(), base).string() == "http://a/b/");
	CHECK(zeep::uri(zeep::uri("http://a/b/g").relative(base).string(), base).string() == "http://a/b/g");
}

TEST_CASE("encoding_1")
{
	// http://a/höken/Ðuh?¤
	zeep::uri u("http://a/h%C3%B6ken/%C3%90uh?%C2%A4");

	CHECK(zeep::decode_url(u.get_path().string()) == "/höken/Ðuh");
	CHECK(zeep::decode_url(u.get_query(false)) == "¤");
	CHECK(u.get_query(true) == "¤");
}