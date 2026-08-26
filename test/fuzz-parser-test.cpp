// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/message-parser.hpp"
#include "zeep/streambuf.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <string_view>

namespace zh = zeep::http;

// --------------------------------------------------------------------
// Fuzz helper: feed bytes into a parser and verify invariants
// --------------------------------------------------------------------

struct fuzz_result
{
	bool threw = false;
	zeep::http::parse_result result;
};

static fuzz_result fuzz_parse_request(std::string_view data)
{
	fuzz_result fr;
	zeep::http::request_parser parser;
	zeep::char_streambuf buf(data.data(), data.size());

	try
	{
		fr.result = parser.parse(buf);
	}
	catch (...)
	{
		fr.threw = true;
	}

	return fr;
}

static fuzz_result fuzz_parse_reply(std::string_view data)
{
	fuzz_result fr;
	zeep::http::reply_parser parser;
	zeep::char_streambuf buf(data.data(), data.size());

	try
	{
		fr.result = parser.parse(buf);
	}
	catch (...)
	{
		fr.threw = true;
	}

	return fr;
}

static void check_invariant(const fuzz_result &fr)
{
	CHECK_FALSE(fr.threw);

	switch (fr.result.m_value)
	{
		case zeep::http::parse_result::true_value:
		case zeep::http::parse_result::false_value:
		case zeep::http::parse_result::indeterminate_value:
			break;
		default:
			FAIL("invalid parse_result value");
	}
}

// --------------------------------------------------------------------
// Core fuzz test: random byte sequences
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_random_bytes")
{
	std::mt19937 rng(42);

	for (int len = 0; len < 256; ++len)
	{
		std::string buf(static_cast<size_t>(len), '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		auto fr_req = fuzz_parse_request(buf);
		check_invariant(fr_req);

		auto fr_rep = fuzz_parse_reply(buf);
		check_invariant(fr_rep);
	}
}

TEST_CASE("fuzz_parser_random_bytes_longer")
{
	std::mt19937 rng(12345);

	for (int i = 0; i < 64; ++i)
	{
		size_t len = 256 + (rng() % 2048);
		std::string buf(len, '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		auto fr_req = fuzz_parse_request(buf);
		check_invariant(fr_req);

		auto fr_rep = fuzz_parse_reply(buf);
		check_invariant(fr_rep);
	}
}

// --------------------------------------------------------------------
// Edge-case inputs
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_empty_input")
{
	CHECK_FALSE(fuzz_parse_request({}).threw);
	CHECK_FALSE(fuzz_parse_reply({}).threw);
}

TEST_CASE("fuzz_parser_single_bytes")
{
	for (int i = 0; i < 256; ++i)
	{
		char ch = static_cast<char>(i);
		auto fr_req = fuzz_parse_request({ &ch, 1 });
		check_invariant(fr_req);

		auto fr_rep = fuzz_parse_reply({ &ch, 1 });
		check_invariant(fr_rep);
	}
}

TEST_CASE("fuzz_parser_all_whitespace")
{
	std::string ws = " \t\r\n ";
	for (int len = 1; len <= 32; ++len)
	{
		std::string buf;
		for (int i = 0; i < len; ++i)
			buf += ws[i % ws.size()];
		check_invariant(fuzz_parse_request(buf));
		check_invariant(fuzz_parse_reply(buf));
	}
}

TEST_CASE("fuzz_parser_null_bytes")
{
	std::string buf(128, '\0');
	check_invariant(fuzz_parse_request(buf));
	check_invariant(fuzz_parse_reply(buf));
}

TEST_CASE("fuzz_parser_partial_request_lines")
{
	auto inputs = {
		"G",
		"GE",
		"GET",
		"GET ",
		"GET /",
		"GET / HTTP/",
		"GET / HTTP/1.",
		"GET / HTTP/1.1",
		"GET / HTTP/1.1\r",
		"POST /path?query=1 HTTP/1.1\r\n",
		"PUT /resource HTTP/1.0\r",
		"DELETE /item/123",
	};

	for (auto input : inputs)
	{
		auto fr_req = fuzz_parse_request(input);
		check_invariant(fr_req);

		auto fr_rep = fuzz_parse_reply(input);
		check_invariant(fr_rep);
	}
}

TEST_CASE("fuzz_parser_partial_reply_lines")
{
	auto inputs = {
		"H",
		"HT",
		"HTTP",
		"HTTP/",
		"HTTP/1.",
		"HTTP/1.1 ",
		"HTTP/1.1 2",
		"HTTP/1.1 20",
		"HTTP/1.1 200",
		"HTTP/1.1 200 ",
		"HTTP/1.0 404 ",
	};

	for (auto input : inputs)
	{
		auto fr_rep = fuzz_parse_reply(input);
		check_invariant(fr_rep);
	}
}

// --------------------------------------------------------------------
// Malformed headers
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_bad_request_lines")
{
	auto inputs = {
		"GET\r\n\r\n",
		"GET \r\r\r\r\r\r\r",
		"GET / \r\n\r\n",
		"123 / HTTP/1.1\r\n\r\n",
		"GET / HTTP/1.1\n\n",
		"GET / HTTP/2.0\r\n\r\n",
		"GET / HTTP/1.1\r\nHost:\r\n\r\n",
		"GET / HTTP/1.1\r\n:value\r\n\r\n",
		"\r\n\r\n",
		"\n\n\n\n",
	};

	for (auto input : inputs)
	{
		check_invariant(fuzz_parse_request(input));
	}
}

TEST_CASE("fuzz_parser_bad_reply_lines")
{
	auto inputs = {
		"HTTP/1.1\r\n\r\n",
		"HTTP/1.1 A\r\n\r\n",
		"HTTP/1.1 20\r\n\r\n",
		"HTTP/1.1 2000\r\n\r\n",
		"HTTP/1.1 200\r\n\r\n",
		"HTTP/1.1 -200 OK\r\n\r\n",
		"HTTP/1.1 999\r\n\r\n",
		"HTTP/1.1 200 OK\n\n",
	};

	for (auto input : inputs)
	{
		check_invariant(fuzz_parse_reply(input));
	}
}

// --------------------------------------------------------------------
// Chunked transfer encoding edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_partial_chunked")
{
	std::string req =
		"POST / HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n";

	auto r1 = fuzz_parse_request(req);
	check_invariant(r1);
	CHECK(r1.result == zeep::http::parse_result::indeterminate_value);

	// valid hex followed by garbage
	std::string req2 = req + "5\r\nhello\r\n0\r\n\r\n";
	auto r2 = fuzz_parse_request(req2);
	check_invariant(r2);

	// corrupted chunk size
	std::string req3 = req + "ZZZ\r\nhello";
	auto r3 = fuzz_parse_request(req3);
	check_invariant(r3);
}

// --------------------------------------------------------------------
// Long/malicious inputs
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_very_long_uri")
{
	std::string uri(4096, 'A');
	std::string req = "GET /" + uri + " HTTP/1.1\r\n\r\n";
	check_invariant(fuzz_parse_request(req));
}

TEST_CASE("fuzz_parser_very_long_header_value")
{
	std::string val(8192, 'X');
	std::string req = "GET / HTTP/1.1\r\nX-Large: " + val + "\r\n\r\n";
	check_invariant(fuzz_parse_request(req));
}

TEST_CASE("fuzz_parser_content_length_overflow")
{
	std::string req =
		"POST / HTTP/1.1\r\n"
		"Content-Length: 99999999999999999999\r\n"
		"\r\n";
	check_invariant(fuzz_parse_request(req));

	std::string rep =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 99999999999999999999\r\n"
		"\r\n";
	check_invariant(fuzz_parse_reply(rep));
}

TEST_CASE("fuzz_parser_empty_chunked")
{
	std::string req =
		"POST / HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"0\r\n"
		"\r\n";
	auto fr = fuzz_parse_request(req);
	check_invariant(fr);
	if (not fr.threw and fr.result == zeep::http::parse_result::true_value)
	{
		CHECK_NOTHROW(zeep::http::request_parser().parse(
			*std::make_unique<zeep::char_streambuf>(req.data(), req.size())));
	}
}

// --------------------------------------------------------------------
// Valid but unusual inputs that should still parse correctly
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_valid_unusual_requests")
{
	auto inputs = {
		"OPTIONS * HTTP/1.1\r\nHost: example.com\r\n\r\n",
		"CONNECT example.com:443 HTTP/1.1\r\nHost: example.com\r\n\r\n",
		"TRACE / HTTP/1.1\r\nHost: example.com\r\n\r\n",
		"PATCH /resource HTTP/1.1\r\nContent-Type: application/json\r\nContent-Length: 2\r\n\r\n{}",
		"HEAD / HTTP/1.0\r\n\r\n",
	};

	for (auto input : inputs)
	{
		auto fr = fuzz_parse_request(input);
		check_invariant(fr);
	}
}

TEST_CASE("fuzz_parser_valid_unusual_replies")
{
	auto inputs = {
		"HTTP/1.1 204 No Content\r\n\r\n",
		"HTTP/1.1 301 Moved Permanently\r\nLocation: /new\r\nContent-Length: 0\r\n\r\n",
		"HTTP/1.1 418 I'm a Teapot\r\nContent-Length: 0\r\n\r\n",
		"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\r\n",
	};

	for (auto input : inputs)
	{
		auto fr = fuzz_parse_reply(input);
		check_invariant(fr);
	}
}

// --------------------------------------------------------------------
// Stress: reset and reuse parsers
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_reset_reuse")
{
	zeep::http::request_parser rp;
	zeep::http::reply_parser rep;

	std::mt19937 rng(9999);

	for (int i = 0; i < 128; ++i)
	{
		size_t len = rng() % 512;
		std::string buf(len, '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		rp.reset();
		rep.reset();

		{
			zeep::char_streambuf sb(buf.data(), buf.size());
			CHECK_NOTHROW(rp.parse(sb));
		}
		{
			zeep::char_streambuf sb(buf.data(), buf.size());
			CHECK_NOTHROW(rep.parse(sb));
		}
	}
}

// --------------------------------------------------------------------
// Symbolic execution style: walk the parser with known-bad token patterns
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_header_name_edge_cases")
{
	auto inputs = {
		// header names with special chars that should be rejected
		"GET / HTTP/1.1\r\nHeader{Name}: value\r\n\r\n",
		"GET / HTTP/1.1\r\nHeader(Name): value\r\n\r\n",
		"GET / HTTP/1.1\r\nHeader<Name>: value\r\n\r\n",
		"GET / HTTP/1.1\r\nHeader=Name: value\r\n\r\n",
		// header continuation without a prior header
		"GET / HTTP/1.1\r\n value\r\n\r\n",
		// empty header name
		"GET / HTTP/1.1\r\n: value\r\n\r\n",
		// header with only whitespace after colon
		"GET / HTTP/1.1\r\nX-Empty: \r\n\r\n",
		// header spanning multiple lines (obs-fold)
		"GET / HTTP/1.1\r\nX-Fold: line1\r\n line2\r\n\r\n",
	};

	for (auto input : inputs)
	{
		check_invariant(fuzz_parse_request(input));
	}
}

// --------------------------------------------------------------------
// High-byte / binary payloads
// --------------------------------------------------------------------

TEST_CASE("fuzz_parser_binary_payload")
{
	std::string req =
		"POST / HTTP/1.1\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Length: 256\r\n"
		"\r\n";

	for (int i = 0; i < 256; ++i)
		req += static_cast<char>(i);

	check_invariant(fuzz_parse_request(req));
}

// --------------------------------------------------------------------
// Chunked transfer encoding — thorough tests
// --------------------------------------------------------------------

static const char kChunkedReqHeaders[] =
	"POST / HTTP/1.1\r\n"
	"Transfer-Encoding: chunked\r\n"
	"\r\n";

static const char kChunkedRepHeaders[] =
	"HTTP/1.1 200 OK\r\n"
	"Transfer-Encoding: chunked\r\n"
	"\r\n";

static fuzz_result fuzz_parse_request_chunked(std::string_view body)
{
	std::string req(kChunkedReqHeaders);
	req.append(body.data(), body.size());
	return fuzz_parse_request(req);
}

static fuzz_result fuzz_parse_reply_chunked(std::string_view body)
{
	std::string rep(kChunkedRepHeaders);
	rep.append(body.data(), body.size());
	return fuzz_parse_reply(rep);
}

struct chunked_request_result
{
	fuzz_result fr;
	std::string payload;
};

static chunked_request_result parse_chunked_request(std::string_view body)
{
	std::string msg(kChunkedReqHeaders);
	msg.append(body.data(), body.size());

	chunked_request_result cr;
	cr.fr = fuzz_parse_request(msg);

	if (cr.fr.result == zeep::http::parse_result::true_value and not cr.fr.threw)
	{
		zeep::http::request_parser p;
		zeep::char_streambuf sb(msg.data(), msg.size());
		p.parse(sb);
		cr.payload = p.get_request().get_payload();
	}

	return cr;
}

struct chunked_reply_result
{
	fuzz_result fr;
	std::string content;
};

static chunked_reply_result parse_chunked_reply(std::string_view body)
{
	std::string msg(kChunkedRepHeaders);
	msg.append(body.data(), body.size());

	chunked_reply_result cr;
	cr.fr = fuzz_parse_reply(msg);

	if (cr.fr.result == zeep::http::parse_result::true_value and not cr.fr.threw)
	{
		zeep::http::reply_parser p;
		zeep::char_streambuf sb(msg.data(), msg.size());
		p.parse(sb);
		cr.content = p.get_reply().get_content();
	}

	return cr;
}

// -- valid cases --

TEST_CASE("chunked_request_single_chunk")
{
	auto cr = parse_chunked_request("5\r\nhello\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "hello");
}

TEST_CASE("chunked_request_multiple_chunks")
{
	auto cr = parse_chunked_request(
		"5\r\nhello\r\n"
		"1\r\n \r\n"
		"5\r\nworld\r\n"
		"0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "hello world");
}

TEST_CASE("chunked_request_empty_body")
{
	auto cr = parse_chunked_request("0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload.empty());
}

TEST_CASE("chunked_request_leading_zeros")
{
	auto cr = parse_chunked_request("005\r\nhello\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "hello");
}

TEST_CASE("chunked_request_lowercase_hex")
{
	auto cr = parse_chunked_request("a\r\n0123456789\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "0123456789");
}

TEST_CASE("chunked_request_uppercase_hex")
{
	auto cr = parse_chunked_request("A\r\nABCDEFGHIJ\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "ABCDEFGHIJ");
}

TEST_CASE("chunked_request_mixed_case_hex")
{
	auto cr = parse_chunked_request("14\r\n01234567890123456789\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "01234567890123456789");
}

TEST_CASE("chunked_request_chunk_extension")
{
	// Extension after ';' — '=' is a tspecial so use simple extension
	auto cr = parse_chunked_request("5;ext\r\nhello\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "hello");
}

TEST_CASE("chunked_request_binary_payload")
{
	// chunk containing all 256 byte values (0x100 = 256)
	std::string body;
	body += "100\r\n";
	for (int i = 0; i < 256; ++i)
		body += static_cast<char>(i);
	body += "\r\n0\r\n\r\n";

	auto cr = parse_chunked_request(body);
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	std::string expected_payload;
	for (int i = 0; i < 256; ++i)
		expected_payload += static_cast<char>(i);
	CHECK(cr.payload == expected_payload);
}

TEST_CASE("chunked_request_crlf_in_data")
{
	// chunk data containing \r\n which should NOT be treated as delimiters
	auto cr = parse_chunked_request("4\r\na\r\nb\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "a\r\nb");
}

TEST_CASE("chunked_request_many_small_chunks")
{
	std::string body;
	for (int i = 0; i < 20; ++i)
		body += "1\r\nX\r\n";
	body += "0\r\n\r\n";

	auto cr = parse_chunked_request(body);
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == std::string(20, 'X'));
}

TEST_CASE("chunked_reply_single_chunk")
{
	auto cr = parse_chunked_reply("5\r\nhello\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.content == "hello");
}

TEST_CASE("chunked_reply_multiple_chunks")
{
	auto cr = parse_chunked_reply(
		"3\r\nfoo\r\n"
		"3\r\nbar\r\n"
		"0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.content == "foobar");
}

// -- invalid cases --

TEST_CASE("chunked_request_non_hex_size")
{
	auto r = fuzz_parse_request_chunked("G\r\nhello\r\n0\r\n\r\n");
	check_invariant(r);
	// 'G' is not a valid hex digit in state 0, should reject
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_non_hex_mid_size")
{
	// valid start '5', then 'G' which is not hex
	auto r = fuzz_parse_request_chunked("5G\r\nhello\r\n0\r\n\r\n");
	check_invariant(r);
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_space_in_size")
{
	auto r = fuzz_parse_request_chunked("5 \r\nhello\r\n0\r\n\r\n");
	check_invariant(r);
	// space is not hex, not ';', not '\r' — rejected in state 1
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_missing_crlf_after_data")
{
	// chunk data not followed by \r\n
	auto r = fuzz_parse_request_chunked("5\r\nhello0\r\n\r\n");
	check_invariant(r);
	// '0' after "hello" is treated as part of data (state 4 counts down),
	// but only 5 bytes expected so after "hello" chunk_size=0, then state 5
	// expects '\r' but gets '0'
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_data_too_short")
{
	// only 3 bytes in a 5-byte chunk, then hits the 0 terminator
	auto r = fuzz_parse_request_chunked("5\r\nhel\r\n0\r\n\r\n");
	check_invariant(r);
	// After "hel" (3 bytes), expects more data but gets '\r' in state 4
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_missing_trailing_crlf")
{
	// zero chunk without trailing \r\n
	auto r = fuzz_parse_request_chunked("0\r\n");
	check_invariant(r);
	// state 10 expects \r, state 11 expects \n — input ends mid-parse
	CHECK(r.result == zeep::http::parse_result::indeterminate_value);
}

TEST_CASE("chunked_request_zero_chunk_no_crlf_at_all")
{
	auto r = fuzz_parse_request_chunked("0");
	check_invariant(r);
	CHECK(r.result == zeep::http::parse_result::indeterminate_value);
}

TEST_CASE("chunked_request_missing_final_newline")
{
	// missing the final \n after \r
	auto r = fuzz_parse_request_chunked("5\r\nhello\r\n0\r\n\r");
	check_invariant(r);
	CHECK(r.result == zeep::http::parse_result::indeterminate_value);
}

TEST_CASE("chunked_request_extension_with_control_char")
{
	// ';' then control char (0x01) — rejected in state 2
	auto r = fuzz_parse_request_chunked("5;\x01\r\nhello\r\n0\r\n\r\n");
	check_invariant(r);
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_extension_with_tspecial")
{
	// ';' then '(' — tspecial, rejected in state 2
	auto r = fuzz_parse_request_chunked("5;(bad\r\nhello\r\n0\r\n\r\n");
	check_invariant(r);
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_only_size_no_data")
{
	// chunk size 5 but only 3 bytes of data before next chunk-size line
	auto r = fuzz_parse_request_chunked("5\r\nhel\r\n0\r\n\r\n");
	check_invariant(r);
	// After "hel" (3 bytes), expects 2 more data bytes but gets '\r' in state 4
	CHECK(r.result == zeep::http::parse_result::false_value);
}

// -- partial / incremental parsing --

TEST_CASE("chunked_request_incremental_parse")
{
	std::string msg = std::string(kChunkedReqHeaders) +
		"5\r\nhello\r\n0\r\n\r\n";

	zeep::http::request_parser p;
	zeep::http::parse_result result = zeep::http::indeterminate;

	// feed one byte at a time
	for (size_t i = 0; i < msg.size(); ++i)
	{
		zeep::char_streambuf sb(&msg[i], 1);
		result = p.parse(sb);
		if (result == zeep::http::parse_result::false_value)
			break;
	}

	REQUIRE(result == zeep::http::parse_result::true_value);
	CHECK(p.get_request().get_payload() == "hello");
}

TEST_CASE("chunked_request_incremental_multi_chunk")
{
	std::string msg = std::string(kChunkedReqHeaders) +
		"3\r\nfoo\r\n"
		"3\r\nbar\r\n"
		"0\r\n\r\n";

	zeep::http::request_parser p;
	zeep::http::parse_result result = zeep::http::indeterminate;

	// feed in chunks of 4 bytes
	for (size_t i = 0; i < msg.size(); i += 4)
	{
		size_t len = std::min<size_t>(4, msg.size() - i);
		zeep::char_streambuf sb(&msg[i], len);
		result = p.parse(sb);
		if (result == zeep::http::parse_result::false_value)
			FAIL("parse failed at byte " + std::to_string(i));
	}

	REQUIRE(result == zeep::http::parse_result::true_value);
	CHECK(p.get_request().get_payload() == "foobar");
}

// -- large chunk sizes (but reasonable) --

TEST_CASE("chunked_request_large_chunk")
{
	// 1000-byte chunk
	std::string data(1000, 'X');
	std::string body = "3e8\r\n" + data + "\r\n0\r\n\r\n";

	auto cr = parse_chunked_request(body);
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == data);
}

// -- edge cases around the state machine transitions --

TEST_CASE("chunked_request_zero_chunk_with_extension")
{
	auto r = fuzz_parse_request_chunked("0;ext\r\n\r\n");
	check_invariant(r);
	REQUIRE(r.result == zeep::http::parse_result::true_value);
}

TEST_CASE("chunked_request_hex_FF_chunk")
{
	std::string data(255, 'A');
	std::string body = "ff\r\n" + data + "\r\n0\r\n\r\n";

	auto cr = parse_chunked_request(body);
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == data);
}

TEST_CASE("chunked_request_just_terminator")
{
	// Only the zero chunk, no data chunks before it
	auto cr = parse_chunked_request("0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload.empty());
}

TEST_CASE("chunked_request_data_with_null_bytes")
{
	// 4-byte chunk containing null bytes
	std::string data = {'\0', 'a', '\0', 'b'};
	std::string body = "4\r\n" + data + "\r\n0\r\n\r\n";

	auto cr = parse_chunked_request(body);
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == data);
}

TEST_CASE("chunked_request_size_with_only_semicolon")
{
	// Just ';' — no hex digits before it
	auto r = fuzz_parse_request_chunked(";\r\nhello\r\n0\r\n\r\n");
	check_invariant(r);
	// ';' is not a hex digit, rejected in state 0
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_multiple_extensions")
{
	// Multiple semicolons — first ';' transitions to state 2, subsequent ';' are tspecials
	auto r = fuzz_parse_request_chunked("5;ext1;ext2\r\nhello\r\n0\r\n\r\n");
	check_invariant(r);
	// After first ';' → state 2, second ';' is tspecial → rejected
	CHECK(r.result == zeep::http::parse_result::false_value);
}

TEST_CASE("chunked_request_empty_extension")
{
	// ';' immediately followed by \r\n
	auto cr = parse_chunked_request("5;\r\nhello\r\n0\r\n\r\n");
	check_invariant(cr.fr);
	REQUIRE(cr.fr.result == zeep::http::parse_result::true_value);
	CHECK(cr.payload == "hello");
}
