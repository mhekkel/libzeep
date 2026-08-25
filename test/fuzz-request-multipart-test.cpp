// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/request.hpp"

#include <catch2/catch_test_macros.hpp>

#include <random>
#include <string>
#include <string_view>

namespace zh = zeep::http;

// --------------------------------------------------------------------
// Helper: create a request with multipart/form-data content type
// --------------------------------------------------------------------

static zh::request make_multipart_request(std::string_view boundary, std::string_view payload)
{
	std::string ct = "multipart/form-data; boundary=";
	ct += boundary;
	return zh::request("POST", "/", { 1, 0 }, { { "Content-Type", std::move(ct) } }, std::string(payload));
}

// --------------------------------------------------------------------
// Fuzz helper: call get_parameter and verify no crash
// --------------------------------------------------------------------

struct fuzz_outcome
{
	bool threw_std_exception = false;
	bool threw_other = false;
	std::optional<std::string> result;
};

static fuzz_outcome fuzz_get_parameter(const zh::request &req, std::string_view name)
{
	fuzz_outcome r;
	try
	{
		r.result = req.get_parameter(name);
	}
	catch (const std::exception &)
	{
		r.threw_std_exception = true;
	}
	catch (...)
	{
		r.threw_other = true;
	}
	return r;
}

static void must_not_crash(const fuzz_outcome &r)
{
	CHECK_FALSE(r.threw_other);
}

// --------------------------------------------------------------------
// Build a valid multipart body
// --------------------------------------------------------------------

static std::string build_multipart_body(std::string_view boundary, std::string_view name, std::string_view value)
{
	std::string body;
	body += "--";
	body += boundary;
	body += "\r\n";
	body += "Content-Disposition: form-data; name=\"";
	body += name;
	body += "\"\r\n";
	body += "\r\n";
	body += value;
	body += "\r\n";
	body += "--";
	body += boundary;
	body += "--\r\n";
	return body;
}

// --------------------------------------------------------------------
// 1. Random byte sequences with fixed boundary
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_random_bytes")
{
	std::mt19937 rng(42);

	for (int len = 0; len < 256; ++len)
	{
		std::string payload(static_cast<size_t>(len), '\0');
		for (auto &ch : payload)
			ch = static_cast<char>(rng() & 0xFF);

		auto req = make_multipart_request("xYzZY", payload);
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}

TEST_CASE("fuzz_multipart_random_bytes_longer")
{
	std::mt19937 rng(54321);

	for (int i = 0; i < 64; ++i)
	{
		size_t len = 256 + (rng() % 2048);
		std::string payload(len, '\0');
		for (auto &ch : payload)
			ch = static_cast<char>(rng() & 0xFF);

		auto req = make_multipart_request("xYzZY", payload);
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}

// --------------------------------------------------------------------
// 2. Edge cases: empty, single bytes, whitespace, nulls
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_empty_input")
{
	auto req = make_multipart_request("xYzZY", "");
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_single_bytes")
{
	for (int i = 0; i < 256; ++i)
	{
		char ch = static_cast<char>(i);
		auto req = make_multipart_request("xYzZY", std::string_view(&ch, 1));
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}

TEST_CASE("fuzz_multipart_all_whitespace")
{
	std::string ws = " \t\r\n ";
	for (int len = 1; len <= 32; ++len)
	{
		std::string buf;
		for (int i = 0; i < len; ++i)
			buf += ws[i % ws.size()];
		auto req = make_multipart_request("xYzZY", buf);
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}

TEST_CASE("fuzz_multipart_null_bytes")
{
	std::string buf(128, '\0');
	auto req = make_multipart_request("xYzZY", buf);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

// --------------------------------------------------------------------
// 3. Valid multipart bodies — should parse without crash
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_valid_single_field")
{
	auto body = build_multipart_body("xYzZY", "username", "alice");
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "username"));
	auto result = fuzz_get_parameter(req, "username");
	CHECK_FALSE(result.threw_std_exception);
	CHECK(result.result.has_value());
	if (result.result)
		CHECK(*result.result == "alice");
}

TEST_CASE("fuzz_multipart_valid_multiple_fields")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"first\"\r\n";
	body += "\r\n";
	body += "Alice\r\n";
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"last\"\r\n";
	body += "\r\n";
	body += "Smith\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "first"));
	must_not_crash(fuzz_get_parameter(req, "last"));
}

TEST_CASE("fuzz_multipart_valid_empty_value")
{
	auto body = build_multipart_body("xYzZY", "empty", "");
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "empty"));
}

TEST_CASE("fuzz_multipart_valid_empty_name")
{
	auto body = build_multipart_body("xYzZY", "", "no-name");
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, ""));
}

TEST_CASE("fuzz_multipart_valid_with_file_fields")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n";
	body += "Content-Type: text/plain\r\n";
	body += "\r\n";
	body += "file content here\r\n";
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"description\"\r\n";
	body += "\r\n";
	body += "a test file\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "description"));
}

// --------------------------------------------------------------------
// 4. Boundary edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_boundary_empty")
{
	auto body = build_multipart_body("xYzZY", "field", "value");
	auto req = make_multipart_request("", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_boundary_single_char")
{
	auto body = build_multipart_body("x", "field", "value");
	auto req = make_multipart_request("x", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_boundary_with_special_chars")
{
	auto body = build_multipart_body("--==$$%%", "field", "value");
	auto req = make_multipart_request("--==$$%%", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_boundary_very_long")
{
	std::string boundary(128, 'B');
	auto body = build_multipart_body(boundary, "field", "value");
	auto req = make_multipart_request(boundary, body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_boundary_in_payload_mismatch")
{
	// payload uses one boundary but request header specifies another
	std::string body;
	body += "--WRONG\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "--WRONG--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_no_boundary_in_content_type")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "--xYzZY--\r\n";

	zh::request req("POST", "/", { 1, 0 }, { { "Content-Type", "multipart/form-data" } }, std::move(body));
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_garbage_after_boundary")
{
	auto body = build_multipart_body("xYzZY", "field", "value");
	body += "GARBAGE DATA AFTER BOUNDARY";
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

// --------------------------------------------------------------------
// 5. Malformed Content-Disposition headers
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_missing_closing_boundary")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_double_closing_boundary")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "--xYzZY--\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_malformed_header")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition INVALID LINE\r\n";
	body += "name=\"field\"\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_header_without_quotes")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=field\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_header_single_quotes")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name='field'\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_header_empty_name")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"\"\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, ""));
}

// --------------------------------------------------------------------
// 6. Line ending edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_lf_only")
{
	std::string body;
	body += "--xYzZY\n";
	body += "Content-Disposition: form-data; name=\"field\"\n";
	body += "\n";
	body += "value\n";
	body += "--xYzZY--\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_cr_only")
{
	std::string body;
	body += "--xYzZY\r";
	body += "Content-Disposition: form-data; name=\"field\"\r";
	body += "\r";
	body += "value\r";
	body += "--xYzZY--\r";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_mixed_line_endings")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\n\r";
	body += "\r\n";
	body += "value\r";
	body += "\n--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_no_trailing_crlf")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value";
	body += "\r\n--xYzZY--";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

// --------------------------------------------------------------------
// 7. Payload content edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_very_long_value")
{
	std::string value(8192, 'X');
	auto body = build_multipart_body("xYzZY", "big", value);
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "big"));
}

TEST_CASE("fuzz_multipart_binary_value")
{
	std::string value;
	for (int i = 0; i < 256; ++i)
		value += static_cast<char>(i);
	auto body = build_multipart_body("xYzZY", "binary", value);
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "binary"));
}

TEST_CASE("fuzz_multipart_value_contains_boundary")
{
	std::string value = "--xYzZY\r\ninjected header\r\n\r\ninjected content\r\n--xYzZY--";
	auto body = build_multipart_body("xYzZY", "sneaky", value);
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "sneaky"));
}

TEST_CASE("fuzz_multipart_value_contains_crlf")
{
	std::string value = "line1\r\nline2\r\nline3";
	auto body = build_multipart_body("xYzZY", "multiline", value);
	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "multiline"));
}

TEST_CASE("fuzz_multipart_empty_body")
{
	auto req = make_multipart_request("xYzZY", "");
	must_not_crash(fuzz_get_parameter(req, "field"));
}

// --------------------------------------------------------------------
// 8. Missing or extra boundary markers
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_only_opening_boundary")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_only_closing_boundary")
{
	std::string body;
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_extra_dashes")
{
	std::string body;
	body += "---xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "---xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

TEST_CASE("fuzz_multipart_boundary_with_no_dash_prefix")
{
	std::string body;
	body += "xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"field\"\r\n";
	body += "\r\n";
	body += "value\r\n";
	body += "xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "field"));
}

// --------------------------------------------------------------------
// 9. Many fields (stress the parser)
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_many_fields")
{
	std::string body;
	for (int i = 0; i < 100; ++i)
	{
		body += "--xYzZY\r\n";
		body += "Content-Disposition: form-data; name=\"field" + std::to_string(i) + "\"\r\n";
		body += "\r\n";
		body += "value" + std::to_string(i) + "\r\n";
	}
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	for (int i = 0; i < 100; ++i)
		must_not_crash(fuzz_get_parameter(req, "field" + std::to_string(i)));
}

TEST_CASE("fuzz_multipart_duplicate_names")
{
	std::string body;
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"dup\"\r\n";
	body += "\r\n";
	body += "first\r\n";
	body += "--xYzZY\r\n";
	body += "Content-Disposition: form-data; name=\"dup\"\r\n";
	body += "\r\n";
	body += "second\r\n";
	body += "--xYzZY--\r\n";

	auto req = make_multipart_request("xYzZY", body);
	must_not_crash(fuzz_get_parameter(req, "dup"));
}

// --------------------------------------------------------------------
// 10. Random boundaries with random payloads (combined fuzz)
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_random_boundary_and_payload")
{
	std::mt19937 rng(9999);

	for (int i = 0; i < 128; ++i)
	{
		// generate a short random boundary
		size_t blen = 2 + rng() % 16;
		std::string boundary(blen, '\0');
		for (auto &ch : boundary)
			ch = static_cast<char>('A' + (rng() % 26));

		// generate random payload
		size_t plen = rng() % 512;
		std::string payload(plen, '\0');
		for (auto &ch : payload)
			ch = static_cast<char>(rng() & 0xFF);

		auto req = make_multipart_request(boundary, payload);
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}

TEST_CASE("fuzz_multipart_random_boundary_valid_body")
{
	std::mt19937 rng(7777);

	for (int i = 0; i < 64; ++i)
	{
		// generate a short random boundary
		size_t blen = 2 + rng() % 16;
		std::string boundary(blen, '\0');
		for (auto &ch : boundary)
			ch = static_cast<char>('A' + (rng() % 26));

		auto body = build_multipart_body(boundary, "field", "value");
		auto req = make_multipart_request(boundary, body);
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}

// --------------------------------------------------------------------
// 11. Payload with boundary substring embedded at random positions
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_boundary_injected_at_random_positions")
{
	std::mt19937 rng(31415);
	std::string boundary = "xYzZY";
	auto body = build_multipart_body(boundary, "field", "value");

	for (int i = 0; i < 64; ++i)
	{
		std::string modified = body;
		size_t pos = rng() % (modified.size() + 1);
		modified.insert(pos, boundary);
		auto req = make_multipart_request(boundary, modified);
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}

// --------------------------------------------------------------------
// 12. Stress: reset and reuse with different payloads
// --------------------------------------------------------------------

TEST_CASE("fuzz_multipart_reuse_request")
{
	std::mt19937 rng(8888);

	for (int i = 0; i < 128; ++i)
	{
		size_t len = rng() % 256;
		std::string payload(len, '\0');
		for (auto &ch : payload)
			ch = static_cast<char>(rng() & 0xFF);

		auto req = make_multipart_request("xYzZY", payload);
		must_not_crash(fuzz_get_parameter(req, "field"));
	}
}
