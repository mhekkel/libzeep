// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/el/object.hpp"
#include "zeep/el/processing.hpp"
#include "zeep/http/scope.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <exception>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace e = zeep::el;
namespace zh = zeep::http;

// --------------------------------------------------------------------
// Fuzz helpers: each wrapper catches all exceptions and records
// whether it was a std::exception (well-formed) or something worse.
// --------------------------------------------------------------------

struct fuzz_outcome
{
	bool threw_std_exception = false;
	bool threw_other = false;
};

static fuzz_outcome fuzz_evaluate_el(zh::scope &scope, std::string_view data)
{
	fuzz_outcome r;
	try
	{
		zh::evaluate_el(scope, std::string(data));
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

static fuzz_outcome fuzz_evaluate_el_attr(zh::scope &scope, std::string_view data)
{
	fuzz_outcome r;
	try
	{
		zh::evaluate_el_attr(scope, std::string(data));
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

static fuzz_outcome fuzz_evaluate_el_assert(zh::scope &scope, std::string_view data)
{
	fuzz_outcome r;
	try
	{
		zh::evaluate_el_assert(scope, std::string(data));
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

static fuzz_outcome fuzz_evaluate_el_with(zh::scope &scope, std::string_view data)
{
	fuzz_outcome r;
	try
	{
		zh::evaluate_el_with(scope, std::string(data));
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

static fuzz_outcome fuzz_evaluate_el_link(zh::scope &scope, std::string_view data)
{
	fuzz_outcome r;
	try
	{
		zh::evaluate_el_link(scope, std::string(data));
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

static fuzz_outcome fuzz_process_el(zh::scope &scope, std::string_view data)
{
	fuzz_outcome r;
	try
	{
		std::string s(data);
		zh::process_el(scope, s);
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

static fuzz_outcome fuzz_process_el_2(zh::scope &scope, std::string_view data)
{
	fuzz_outcome r;
	try
	{
		zh::process_el_2(scope, std::string(data));
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

static fuzz_outcome fuzz_parse_json(std::string_view data)
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

// --------------------------------------------------------------------

static void must_not_crash(const fuzz_outcome &r)
{
	CHECK_FALSE(r.threw_other);
}

// evaluate_el and process_el have internal try-catch -> should never throw at all
static void must_not_throw(const fuzz_outcome &r)
{
	must_not_crash(r);
	CHECK_FALSE(r.threw_std_exception);
}

// --------------------------------------------------------------------
// Random byte sequences
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_random_bytes")
{
	std::mt19937 rng(42);

	for (int len = 0; len < 128; ++len)
	{
		std::string buf(static_cast<size_t>(len), '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		zh::scope scope;

		must_not_throw(fuzz_evaluate_el(scope, buf));
		must_not_throw(fuzz_process_el(scope, buf));
		must_not_throw(fuzz_process_el_2(scope, buf));

		must_not_crash(fuzz_evaluate_el_attr(scope, buf));
		must_not_crash(fuzz_evaluate_el_assert(scope, buf));
		{
			zh::scope ws;
			must_not_crash(fuzz_evaluate_el_with(ws, buf));
		}
		must_not_crash(fuzz_evaluate_el_link(scope, buf));
		must_not_crash(fuzz_parse_json(buf));
	}
}

TEST_CASE("fuzz_el_random_bytes_longer")
{
	std::mt19937 rng(54321);

	for (int i = 0; i < 32; ++i)
	{
		size_t len = 128 + (rng() % 1024);
		std::string buf(len, '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		zh::scope scope;
		must_not_throw(fuzz_evaluate_el(scope, buf));
		must_not_throw(fuzz_process_el(scope, buf));
		must_not_throw(fuzz_process_el_2(scope, buf));

		must_not_crash(fuzz_evaluate_el_attr(scope, buf));
		must_not_crash(fuzz_evaluate_el_assert(scope, buf));
		{
			zh::scope ws;
			must_not_crash(fuzz_evaluate_el_with(ws, buf));
		}
		must_not_crash(fuzz_evaluate_el_link(scope, buf));
		must_not_crash(fuzz_parse_json(buf));
	}
}

// --------------------------------------------------------------------
// Edge cases: empty, single bytes, whitespace, nulls
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_empty_input")
{
	zh::scope scope;
	must_not_throw(fuzz_evaluate_el(scope, ""));
	must_not_throw(fuzz_process_el(scope, ""));
	must_not_throw(fuzz_process_el_2(scope, ""));

	must_not_crash(fuzz_evaluate_el_attr(scope, ""));
	must_not_crash(fuzz_evaluate_el_assert(scope, ""));
	{
		zh::scope ws;
		must_not_crash(fuzz_evaluate_el_with(ws, ""));
	}
	must_not_crash(fuzz_evaluate_el_link(scope, ""));
	must_not_crash(fuzz_parse_json(""));
}

TEST_CASE("fuzz_el_single_bytes")
{
	zh::scope scope;
	for (int i = 0; i < 256; ++i)
	{
		char ch = static_cast<char>(i);
		std::string_view buf(&ch, 1);

		must_not_throw(fuzz_evaluate_el(scope, buf));
		must_not_crash(fuzz_evaluate_el_attr(scope, buf));
		must_not_crash(fuzz_evaluate_el_assert(scope, buf));
		must_not_crash(fuzz_evaluate_el_link(scope, buf));
		must_not_crash(fuzz_parse_json(buf));
	}
}

TEST_CASE("fuzz_el_all_whitespace")
{
	std::string ws = " \t\r\n ";
	for (int len = 1; len <= 32; ++len)
	{
		std::string buf;
		for (int i = 0; i < len; ++i)
			buf += ws[i % ws.size()];

		zh::scope scope;
		must_not_throw(fuzz_evaluate_el(scope, buf));
		must_not_crash(fuzz_evaluate_el_attr(scope, buf));
		must_not_crash(fuzz_evaluate_el_assert(scope, buf));
		{
			zh::scope ws2;
			must_not_crash(fuzz_evaluate_el_with(ws2, buf));
		}
		must_not_crash(fuzz_evaluate_el_link(scope, buf));
	}
}

TEST_CASE("fuzz_el_null_bytes")
{
	std::string buf(128, '\0');
	zh::scope scope;
	must_not_throw(fuzz_evaluate_el(scope, buf));
	must_not_crash(fuzz_parse_json(buf));
}

// --------------------------------------------------------------------
// Invalid UTF-8 sequences
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_invalid_utf8")
{
	auto inputs = {
		std::string_view("\xC0\x80", 2),
		std::string_view("\xE0\x80\x80", 3),
		std::string_view("\xF0\x80\x80\x80", 4),
		std::string_view("\xC0", 1),
		std::string_view("\xE0\x80", 2),
		std::string_view("\xF0\x80\x80", 3),
		std::string_view("\xC0\xC0", 2),
		std::string_view("\xE0\x80\xC0", 3),
		std::string_view("\xF8\x80\x80\x80\x80", 5),
		std::string_view("\xFC\x80\x80\x80\x80\x80", 6),
		std::string_view("\x80", 1),
		std::string_view("\xBF", 1),
		std::string_view("abc\xFE\xFF\x80xyz", 9),
		std::string_view("hello \xC0\xAF world", 16),
		std::string_view("\xF0\xA0\x80", 3),
		std::string_view("\xE0\xA0", 2),
	};

	for (auto input : inputs)
	{
		zh::scope scope;
		must_not_throw(fuzz_evaluate_el(scope, input));
		must_not_throw(fuzz_process_el(scope, input));
		must_not_throw(fuzz_process_el_2(scope, input));
		must_not_crash(fuzz_parse_json(input));
	}
}

// --------------------------------------------------------------------
// EL expression edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_expression_edge_cases")
{
	auto inputs = {
		"${}",
		"${ }",
		"${1}",
		"${1.5}",
		"${true}",
		"${false}",
		"${null}",
		"${'hello'}",
		"${''}",
		"${1 + 2}",
		"${1 - 2}",
		"${1 * 2}",
		"${1 / 2}",
		"${1 % 2}",
		"${1 div 2}",
		"${1 mod 2}",
		"${1 eq 2}",
		"${1 ne 2}",
		"${1 lt 2}",
		"${1 le 2}",
		"${1 gt 2}",
		"${1 ge 2}",
		"${1 and 2}",
		"${1 or 2}",
		"${not true}",
		"${1 == 2}",
		"${1 != 2}",
		"${1 < 2}",
		"${1 <= 2}",
		"${1 > 2}",
		"${1 >= 2}",
		"${1 ? 2 : 3}",
		"${1 ?: 3}",
		"${x}",
		"${x.y}",
		"${x[0]}",
		"${func()}",
		"${func(1,2,3)}",
		"${#dates}",
		"${#numbers}",
		"${#request}",
		"${#security}",
		"${\n}",
		"${\t}",
		"${  }",
	};

	zh::scope scope;
	for (auto input : inputs)
	{
		must_not_throw(fuzz_evaluate_el(scope, input));
		must_not_throw(fuzz_process_el(scope, input));
	}
}

TEST_CASE("fuzz_el_nested_and_unclosed_templates")
{
	auto inputs = {
		"${${}}",
		"${${${}}}",
		"${ ${ x } }",
		"$",
		"${",
		"${ ",
		"${x",
		"${} trailing",
		"leading ${} trailing",
		"${}${}${}",
		"*{x}",
		"#{x}",
		"@{x}",
		"~{x}",
		"|text ${var}|",
		"${not not not x}",
		"${1 + 2 + 3 + 4 + 5}",
		"${(1)}",
		"${((1))}",
	};

	zh::scope scope;
	for (auto input : inputs)
	{
		must_not_throw(fuzz_evaluate_el(scope, input));
		must_not_throw(fuzz_process_el(scope, input));
		must_not_throw(fuzz_process_el_2(scope, input));
	}
}

TEST_CASE("fuzz_el_attr_edge_cases")
{
	auto inputs = {
		"x=1",
		"x=1, y=2",
		"name=${'value'}",
		"a=1,b=2,c=3",
		"x=",
		"x=,",
		"=1",
		"x=${}",
		"x=${y z}",
	};

	zh::scope scope;
	for (auto input : inputs)
	{
		must_not_crash(fuzz_evaluate_el_attr(scope, input));
	}
}

TEST_CASE("fuzz_el_with_edge_cases")
{
	auto inputs = {
		"x=1",
		"x=1, y=2",
		"name=${'value'}",
		"a=1,b=2,c=3",
		"x=",
		"x=,",
		"=1",
	};

	for (auto input : inputs)
	{
		zh::scope scope;
		must_not_crash(fuzz_evaluate_el_with(scope, input));
	}
}

TEST_CASE("fuzz_el_assert_edge_cases")
{
	auto inputs = {
		"true",
		"false",
		"1",
		"0",
		"true, false",
		"1, 2, 3",
		"${x}",
		"${1 > 2}",
	};

	zh::scope scope;
	for (auto input : inputs)
	{
		must_not_crash(fuzz_evaluate_el_assert(scope, input));
	}
}

TEST_CASE("fuzz_el_link_edge_cases")
{
	auto inputs = {
		"/path",
		"/path/${id}",
		"/path?x=${y}",
		"//host/path",
		"relative/path",
		"/",
		"${empty}",
		"/path/${a}/${b}/${c}",
	};

	zh::scope scope;
	for (auto input : inputs)
	{
		must_not_crash(fuzz_evaluate_el_link(scope, input));
	}
}

// --------------------------------------------------------------------
// JSON parsing edge cases
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_json_edge_cases")
{
	auto inputs = {
		"{}",
		"[]",
		"true",
		"false",
		"null",
		"\"\"",
		"0",
		"-0",
		"1.5",
		"-1.5",
		"1e10",
		"1.5e-3",
		"\"hello \\n world\"",
		"\"escaped: \\\" \\\\ \\/ \\b \\f \\n \\r \\t\"",
		"\"unicode: \\u0041\"",
		"{\"a\":1,\"b\":2}",
		"[1,2,3]",
		"{\"nested\":{\"deep\":[1,2,3]}}",
		"\x01\x02\x03",
		"\"truncated \\u",
		"\"truncated \\u0",
		"\"truncated \\u00",
		"\"truncated \\u000",
		"\"lone backslash: \\",
		"{missing quotes}",
		"{,}",
		"[,]",
		"[1,]",
		"{trailing comma}",
	};

	for (auto input : inputs)
	{
		must_not_crash(fuzz_parse_json(input));
	}
}

// --------------------------------------------------------------------
// Stress: repeated parsing with reuse
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_reset_reuse")
{
	std::mt19937 rng(7777);

	for (int i = 0; i < 64; ++i)
	{
		size_t len = rng() % 256;
		std::string buf(len, '\0');
		for (auto &ch : buf)
			ch = static_cast<char>(rng() & 0xFF);

		zh::scope scope;
		must_not_throw(fuzz_evaluate_el(scope, buf));
		must_not_throw(fuzz_process_el(scope, buf));
		must_not_crash(fuzz_evaluate_el_attr(scope, buf));
		must_not_crash(fuzz_evaluate_el_assert(scope, buf));
		{
			zh::scope ws;
			must_not_crash(fuzz_evaluate_el_with(ws, buf));
		}
		must_not_crash(fuzz_evaluate_el_link(scope, buf));
		must_not_crash(fuzz_parse_json(buf));
	}
}

// --------------------------------------------------------------------
// Very long and deeply nested inputs
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_very_long_input")
{
	std::string buf(8192, 'A');

	zh::scope scope;
	must_not_throw(fuzz_evaluate_el(scope, buf));
	must_not_throw(fuzz_process_el(scope, buf));
	must_not_crash(fuzz_parse_json(buf));

	std::string nested = "${" + std::string(4096, '(') + "1" + std::string(4096, ')') + "}";
	must_not_throw(fuzz_evaluate_el(scope, nested));
	must_not_throw(fuzz_process_el(scope, nested));
}

TEST_CASE("fuzz_el_deeply_nested_arrays_and_objects")
{
	std::string arr(512, '[');
	arr += std::string(512, ']');
	must_not_crash(fuzz_parse_json(arr));

	std::string obj;
	for (int i = 0; i < 128; ++i)
		obj += "\"k" + std::to_string(i) + "\":{";
	obj += "}";
	for (int i = 0; i < 128; ++i)
		obj += "}";

	must_not_crash(fuzz_parse_json(obj));
}

// --------------------------------------------------------------------
// Scope with populated variables
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_with_populated_scope")
{
	zh::scope scope;
	scope.put("str", "hello");
	scope.put("num", 42);
	scope.put("flag", true);
	scope.put("items", std::vector<zeep::el::object>{ e::object(1), e::object(2), e::object(3) });

	auto inputs = {
		"${str}",
		"${num}",
		"${flag}",
		"${items}",
		"${items[0]}",
		"${str + ' world'}",
		"${num + 1}",
		"${num == 42}",
		"${items.size()}",
		"${not flag}",
		"${str.contains('ell')}",
		"${str.substring(1, 3)}",
	};

	for (auto input : inputs)
	{
		must_not_throw(fuzz_evaluate_el(scope, input));
		must_not_throw(fuzz_process_el(scope, input));
		must_not_throw(fuzz_process_el_2(scope, input));
	}
}

// --------------------------------------------------------------------
// Half-open template expressions (truncated in the middle of tokens)
// --------------------------------------------------------------------

TEST_CASE("fuzz_el_truncated_expressions")
{
	auto inputs = {
		"${'",
		"${'hello",
		"${1",
		"${1.",
		"${1.5",
		"${tr",
		"${tr u",
		"${x.",
		"${x.y.",
		"${func(",
		"${func(1",
		"${func(1,",
		"${func(1,2",
		"${1 +",
		"${1 + ",
		"${1 ?",
		"${1 ? ",
		"${1 ?:",
		"${1 ?: ",
		"${1 ? 2",
		"${1 ? 2 ",
		"${1 ? 2 :",
		"${1 ? 2 : ",
		"${#",
		"${#d",
		"${#da",
		"${#dat",
		"*{",
		"*{x",
		"#{",
		"#{x",
		"@{",
		"@{x",
		"~{",
		"~{x",
		"|text ${",
		"|text ${v",
		"|text ${var",
	};

	zh::scope scope;
	for (auto input : inputs)
	{
		must_not_throw(fuzz_evaluate_el(scope, input));
		must_not_throw(fuzz_process_el(scope, input));
	}
}
