// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/el/object.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <exception>
#include <random>
#include <string>
#include <string_view>

namespace e = zeep::el;

namespace
{

// --------------------------------------------------------------------
// Fuzz helper: feed bytes into the JSON deserializer and verify
// it only throws std::exception (never crashes or throws unknown types)
// --------------------------------------------------------------------

struct fuzz_outcome
{
	bool threw_std_exception = false;
	bool threw_other = false;
};

fuzz_outcome fuzz_parse_json(std::string_view data)
{
	fuzz_outcome r;
	try
	{
		e::object::parse_JSON(data);
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

void must_not_crash(const fuzz_outcome &r)
{
	CHECK_FALSE(r.threw_other);
}

} // namespace

// --------------------------------------------------------------------
// 1. Random byte sequences
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_random_bytes")
{
	std::mt19937 rng(42);

	for (int len = 0; len < 256; ++len)
	{
		std::string buf(static_cast<size_t>(len), '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		must_not_crash(fuzz_parse_json(buf));
	}
}

TEST_CASE("fuzz_json_random_bytes_longer")
{
	std::mt19937 rng(54321);

	for (int i = 0; i < 64; ++i)
	{
		size_t len = 256 + (rng() % 2048);
		std::string buf(len, '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		must_not_crash(fuzz_parse_json(buf));
	}
}

// --------------------------------------------------------------------
// 2. Edge cases: empty, single bytes, whitespace, nulls
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_empty_input")
{
	must_not_crash(fuzz_parse_json(""));
}

TEST_CASE("fuzz_json_single_bytes")
{
	for (int i = 0; i < 256; ++i)
	{
		char ch = static_cast<char>(i);
		must_not_crash(fuzz_parse_json({ &ch, 1 }));
	}
}

TEST_CASE("fuzz_json_all_whitespace")
{
	std::string ws = " \t\r\n ";
	for (int len = 1; len <= 32; ++len)
	{
		std::string buf;
		for (int i = 0; i < len; ++i)
			buf += ws[i % ws.size()];
		must_not_crash(fuzz_parse_json(buf));
	}
}

TEST_CASE("fuzz_json_null_bytes")
{
	std::string buf(128, '\0');
	must_not_crash(fuzz_parse_json(buf));
}

// --------------------------------------------------------------------
// 3. Number parsing edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_numbers_valid")
{
	auto inputs = {
		"0",
		"1",
		"-0",
		"-1",
		"1234567890",
		"0.0",
		"1.5",
		"-1.5",
		"0.5",
		"123.456",
		"1e10",
		"1E10",
		"1e+10",
		"1e-10",
		"1.5e10",
		"1.5E10",
		"1.5e+10",
		"1.5e-10",
		"1e0",
		"1e1",
		"1e9",
		"1.0e0",
		"1.0e1",
		"100",
		"999999999999999",
		"-999999999999999",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

TEST_CASE("fuzz_json_numbers_invalid")
{
	auto inputs = {
		"",      // empty (not really a number, but tests empty input path)
		"-",     // just minus
		"+",     // just plus
		"+1",    // leading plus
		"0123",  // leading zero
		"00",    // double zero
		"01",    // leading zero
		"00.5",  // leading zero with decimal
		"1.",    // trailing dot
		"1.2.3", // multiple dots
		".5",    // no leading digit
		"-.5",   // minus with no leading digit
		"--1",   // double minus
		"1e1.5", // float in exponent
		"0x10",  // hex
		"0b10",  // binary
		"Infinity",
		"not a json literal",
		"1e",  // missing exponent digits
		"1e-", // missing exponent digits after sign
		"1e+", // missing exponent digits after sign
		"1E",  // capital E, no digits
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

TEST_CASE("fuzz_json_number_overflow")
{
	auto inputs = {
		"999999999999999999999999999999999999",
		"-999999999999999999999999999999999999",
		"1e999999999999999999999999999999999999999",
		"1e-999999999999999999999999999999999999",
		"99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 4. String parsing edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_strings_valid")
{
	auto inputs = {
		R"("")",
		R"("hello")",
		R"("hello world")",
		R"("escaped: \" \\ \/ \b \f \n \r \t")",
		R"("unicode: \u0041 \u0042 \u0043")",
		R"("mixed: \n\t\r")",
		R"("123")",
		R"("true")",
		R"("null")",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

TEST_CASE("fuzz_json_strings_invalid")
{
	auto inputs = {
		"\"",                // unclosed
		"\"\\",              // lone backslash
		"\"\\x",             // bad escape
		"\"\\u",             // truncated unicode
		"\"\\u0",            // truncated
		"\"\\u00",           // truncated
		"\"\\u000",          // truncated
		"\"\\u000G",         // bad hex digit
		"\"\\uD800",         // lone high surrogate in escape
		"\"\\uDC00",         // lone low surrogate in escape
		"\"\\uFFFF",         // valid hex, codepoint U+FFFF
		"\"\\uFFFE",         // codepoint U+FFFE
		"\"line1\nline2\"",  // literal newline (illegal in JSON)
		"\"tab\there\"",     // literal tab
		"\"null byte\0in\"", // null byte in string
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 5. UTF-8 edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_invalid_utf8")
{
	auto inputs = {
		// Overlong sequences
		std::string_view("\xC0\x80", 2),
		std::string_view("\xE0\x80\x80", 3),
		std::string_view("\xF0\x80\x80\x80", 4),
		// Missing continuation bytes
		std::string_view("\xC0", 1),
		std::string_view("\xE0\x80", 2),
		std::string_view("\xF0\x80\x80", 3),
		// Invalid continuation bytes
		std::string_view("\xC0\xC0", 2),
		std::string_view("\xE0\x80\xC0", 3),
		// Values beyond U+10FFFF
		std::string_view("\xF8\x80\x80\x80\x80", 5),
		std::string_view("\xFC\x80\x80\x80\x80\x80", 6),
		// Lone continuation bytes
		std::string_view("\x80", 1),
		std::string_view("\xBF", 1),
		// High bytes not part of valid UTF-8
		std::string_view("\xFE", 1),
		std::string_view("\xFF", 1),
		// Rejected codepoints U+FFFF and U+FFFE in UTF-8
		std::string_view("\xEF\xBF\xBF", 3), // U+FFFF
		std::string_view("\xEF\xBF\xBE", 3), // U+FFFE
		// Surrogates encoded as raw UTF-8 (illegal)
		std::string_view("\xED\xA0\x80", 3), // U+D800 high surrogate
		std::string_view("\xED\xB0\x80", 3), // U+DC00 low surrogate
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 6. Structural edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_valid_structures")
{
	auto inputs = {
		"{}",
		"[]",
		"true",
		"false",
		"null",
		"\"\"",
		"0",
		"{\"a\":1}",
		"[1]",
		"{\"a\":1,\"b\":2}",
		"[1,2,3]",
		"{\"nested\":{\"deep\":[1,2,3]}}",
		"[{\"a\":1},{\"b\":2}]",
		"{\"\":null}",
		"[true,false,null]",
		"[[[]]]",
		"{\"a\":{\"b\":{\"c\":{}}}}",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

TEST_CASE("fuzz_json_invalid_structures")
{
	auto inputs = {
		"{",          // unclosed object
		"[",          // unclosed array
		"{,}",        // empty key
		"[,]",        // empty element
		"[1,]",       // trailing comma in array
		"{\"a\":1,}", // trailing comma in object
		"{1:2}",      // non-string key
		"{\"a\"}",    // missing value
		"{\"a\":}",   // empty value
		"[1 2]",      // missing comma
		"{\"a\" 1}",  // missing colon
		"{\"a\"::1}", // double colon
		"[1,,2]",     // double comma
		"{1}",        // number as key
		"{}}",        // extra closing brace
		"[]]",        // extra closing bracket
		"{]",
		"[}",
		"{null:1}",          // null as key
		"{\"a\":1 \"b\":2}", // missing comma between pairs
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 7. Literal edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_literals")
{
	auto inputs = {
		"true",
		"false",
		"null",
		"True", // wrong case
		"False",
		"NULL",
		"tru", // truncated
		"fals",
		"nul",
		"TRUE",
		"FALSE",
		"t",
		"f",
		"n",
		"truefalse",
		"truE",
		"falsE",
		"nulL",
		"true ",
		"false ",
		"null ",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 8. Deeply nested structures (stack depth testing)
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_deep_nesting_arrays")
{
	for (int depth = 1; depth <= 512; depth *= 2)
	{
		std::string input;
		for (int i = 0; i < depth; ++i)
			input += '[';
		input += '1';
		for (int i = 0; i < depth; ++i)
			input += ']';

		must_not_crash(fuzz_parse_json(input));
	}
}

TEST_CASE("fuzz_json_deep_nesting_objects")
{
	for (int depth = 1; depth <= 256; depth *= 2)
	{
		std::string input;
		for (int i = 0; i < depth; ++i)
			input += "\"k\":{";
		input += "}";
		for (int i = 0; i < depth; ++i)
			input += "}";

		must_not_crash(fuzz_parse_json(input));
	}
}

TEST_CASE("fuzz_json_alternating_nesting")
{
	std::string input;
	for (int i = 0; i < 256; ++i)
		input += (i % 2 == 0) ? "[" : "{";
	input += "1";
	for (int i = 255; i >= 0; --i)
		input += (i % 2 == 0) ? "]" : "}";

	must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 9. Very long inputs
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_long_string")
{
	std::string input = "\"" + std::string(4096, 'A') + "\"";
	must_not_crash(fuzz_parse_json(input));
}

TEST_CASE("fuzz_json_long_array")
{
	std::string input = "[";
	for (int i = 0; i < 1000; ++i)
		input += "1,";
	input += "1]";
	must_not_crash(fuzz_parse_json(input));
}

TEST_CASE("fuzz_json_long_object")
{
	std::string input = "{";
	for (int i = 0; i < 500; ++i)
		input += "\"k" + std::to_string(i) + "\":null,";
	input += "\"last\":null}";
	must_not_crash(fuzz_parse_json(input));
}

TEST_CASE("fuzz_json_long_numeric_string")
{
	// Very long string of digits (not a number, actual string)
	std::string input = "\"" + std::string(8192, '9') + "\"";
	must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 10. Escape sequence edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_escape_sequences")
{
	auto inputs = {
		R"("\u0000")",       // null escape
		R"("\u001F")",       // control char via escape
		R"("\u007F")",       // DEL via escape
		R"("\u0080")",       // extended ASCII
		R"("\u00FF")",       // Latin-1
		R"("\u0100")",       // 2-byte UTF-8
		R"("\u0800")",       // 3-byte UTF-8
		R"("\uFFFF")",       // valid codepoint but rejected
		R"("\uFFFE")",       // rejected codepoint
		R"("\uD7FF")",       // last valid BMP before surrogates
		R"("\uD800")",       // lone high surrogate (will become invalid UTF-8 in output)
		R"("\uDFFF")",       // lone low surrogate
		R"("\uDBFF\uDFFF")", // valid surrogate pair? handled as two separate escapes?
		R"("\uD800\uDC00")", // proper surrogate pair
		R"("\uD83D\uDE00")", // grinning face emoji
		R"("\n\t\r")",       // common escapes mixed
		"\"\\\"\"",          // escaped quote
		"\"\\\\\"",          // escaped backslash
		"\"\\/\"",           // escaped slash
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 11. BOM and encoding edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_bom_and_prefixes")
{
	auto make_bom = []() -> std::string
	{
		return std::string("\xEF\xBB\xBF", 3);
	};

	auto test = [](std::string_view s)
	{
		must_not_crash(fuzz_parse_json(s));
	};

	test(make_bom() + "{}");
	test(make_bom() + "[]");
	test(make_bom() + "\"hello\"");
	test(make_bom() + "123");

	auto inputs = {
		"undefined",
		"NaN",
		"[NaN]",
		"nullundefined",
		"---",
		"***",
		"<!DOCTYPE html>",
		"<?xml version=\"1.0\"?>",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 12. Mix of valid JSON with trailing garbage
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_trailing_garbage")
{
	auto inputs = {
		"{}trailing",
		"[] after",
		"42 extra",
		"\"string\" and more",
		"true false",
		"null\nnull",
		"{}\n[]",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}

// --------------------------------------------------------------------
// 13. Stress: reuse parser via parse_JSON repeated calls
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_reset_reuse")
{
	std::mt19937 rng(7777);

	for (int i = 0; i < 128; ++i)
	{
		size_t len = rng() % 256;
		std::string buf(len, '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		must_not_crash(fuzz_parse_json(buf));
	}
}

// --------------------------------------------------------------------
// 14. Valid JSON with all value types in combination
// --------------------------------------------------------------------

TEST_CASE("fuzz_json_complex_valid")
{
	auto inputs = {
		R"({"string":"hello","number":42,"float":3.14,"bool":true,"null":null,"array":[1,2,3],"object":{"a":1}})",
		R"([{"x":1,"y":2},{"x":3,"y":4}])",
		R"({"deep":{"deeper":{"deepest":{"value":42}}}})",
		R"([[1,2],[3,4],[5,6]])",
		R"({"":null," ":true,"key":"value"})",
		R"({"escape":"tab\there","newline":"not\nreally","unicode":"\u00e9"})",
	};

	for (auto input : inputs)
		must_not_crash(fuzz_parse_json(input));
}
