// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include <catch2/catch_test_macros.hpp>

#if ZEEP_CXX_MODULE
import zeep;
#else
#include <zeep/el/object.hpp>
#include <zeep/el/serializer.hpp>
#include <zeep/el/processing.hpp>
#include <zeep/http/scope.hpp>
#endif

#if ZEEM_CXX_MODULE
import zeem;
#else
#include <zeem/zeem.hpp>
#endif

#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;
namespace e = zeep::el;

static bool throws_with(auto &&fn, const char *what)
{
	try
	{
		fn();
	}
	catch (const std::exception &e)
	{
		return std::string(e.what()).find(what) != std::string::npos;
	}
	return false;
}

struct Opname
{
	string id;
	map<string, float> standen;

	template <typename Archive>
	void serialize(Archive &ar, uint64_t /*version*/)
	{
		// clang-format off
		ar & zeem::name_value_pair("id", id)
		   & zeem::name_value_pair("standen", standen);
		// clang-format on
	}

	auto operator<=>(const Opname &) const = default;
};

static_assert(not std::is_constructible_v<e::object, Opname>);

using a_map_type = map<string, float>;
static_assert(e::is_serializable_map_type_v<a_map_type>);

static_assert(std::is_constructible_v<e::object, std::string>);

TEST_CASE("test-1")
{
	Opname opn{ "1", { { "een", 0.1f },
						 { "twee", 0.2f } } };

	e::object o = e::serializer<Opname>::serialize(opn);

	std::cout << o << "\n";

	Opname opn2 = e::serializer<Opname>::deserialize(o);

	CHECK(opn == opn2);
}

TEST_CASE("test-2")
{
	std::vector<Opname> opnames{
		{ "1", { { "een", 0.1f },
				   { "twee", 0.2f } } },
		{ "2", { { "drie", 0.3f },
				   { "vier", 0.4f } } },
	};

	e::object o = e::serializer<std::vector<Opname>>::serialize(opnames);

	std::cout << o << "\n";

	auto opn2 = e::serializer<std::vector<Opname>>::deserialize(o);

	CHECK(opnames == opn2);
}

TEST_CASE("test-3")
{
	Opname opn{ "1", { { "een", 0.1f },
						 { "twee", 0.2f } } };

	auto o_opn = std::make_optional<Opname>(opn);

	// static_assert(zeep::el::is_serializable_optional_type_v<std::optional<Opname>>, "");

	e::object o = e::serializer<std::optional<Opname>>::serialize(o_opn);

	std::cout << o << "\n";

	auto opn2 = e::serializer<std::optional<Opname>>::deserialize(o);

	CHECK(*o_opn == opn2);

	// check with empty

	o_opn.reset();

	o = e::serializer<std::optional<Opname>>::serialize(o_opn);

	std::cout << o << "\n";

	opn2 = e::serializer<std::optional<Opname>>::deserialize(o);

	CHECK_FALSE(opn2.has_value());
}

TEST_CASE("test-4")
{
	auto now = std::chrono::system_clock::now();
	auto o = e::serializer<decltype(now)>::serialize(now);
	std::cout << o << '\n';
	auto n = e::serializer<decltype(now)>::deserialize(o);
	CHECK(n == now);
}

TEST_CASE("test-5")
{
	zeep::http::scope scope;

	Opname opn1{ "1", { { "een", 0.1f },
						  { "twee", 0.2f } } };

	// static_assert(e::detail::is_serializable_to_object_v<Opname>);

	auto obj1 = e::to_object(opn1);

	scope.put("o1", obj1);

	auto opn2 = e::from_object<Opname>(obj1);
	CHECK(opn1 == opn2);

	std::vector<Opname> opn_v{ opn1, opn1 };
	scope.put("o2", e::to_object(opn_v));
}

TEST_CASE("test-6")
{
	enum class Status
	{
		RUNNING,
		STOPPED
	};

	zeem::value_serializer<Status>::init({ { Status::RUNNING, "running" },
		{ Status::STOPPED, "stopped" } });

	Status status = Status::RUNNING;

	e::object o = e::serializer<Status>::serialize(status);

	std::cout << o << "\n";

	auto status2 = e::serializer<Status>::deserialize(o);

	CHECK(status == status2);
}

TEST_CASE("test-7")
{
	const e::object obj{
		{ "answer", 42 },
		{ "name", "zeep" }
	};

	const auto &missing = obj["missing"];
	CHECK(missing.is_null());
	CHECK(missing.type() == e::object::value_type::null);
	CHECK(missing.empty());
	CHECK(obj["missing"].get<int64_t>() == 0);

	CHECK_THROWS(obj.at("missing"));

	CHECK(obj["answer"].get<int64_t>() == 42);
	CHECK(obj["name"].get<std::string>() == "zeep");

	const e::object arr{ 1, 2, 3 };
	CHECK(arr.size() == 3);
	CHECK(arr[1].get<int64_t>() == 2);
	CHECK(arr[10].is_null());
	CHECK(arr[10].get<int64_t>() == 0);
	CHECK_THROWS(arr.at(10));
}

TEST_CASE("test-8")
{
	e::object obj{ { "answer", 42 } };
	REQUIRE(obj.is_object());

	auto &missing = obj["missing"];
	REQUIRE(missing.is_null());
	CHECK(obj.contains("missing"));

	missing = 7;
	CHECK(obj["missing"].get<int64_t>() == 7);

	e::object arr{ 1, 2, 3 };
	REQUIRE(arr.is_array());

	auto &el = arr[5];
	REQUIRE(el.is_null());
	CHECK(arr.size() == 6);
	CHECK(arr[3].is_null());
	CHECK(arr[4].is_null());

	el = 42;
	CHECK(arr[5].get<int64_t>() == 42);
}

TEST_CASE("erase object by key")
{
	e::object obj{
		{ "a", 1 },
		{ "b", 2 },
		{ "c", 3 }
	};

	REQUIRE(obj.is_object());

	// erase an existing key
	auto count = obj.erase("b");
	CHECK(count == 1);
	CHECK_FALSE(obj.contains("b"));
	CHECK(obj.size() == 2);
	CHECK(obj.contains("a"));
	CHECK(obj.contains("c"));

	// erasing a missing key returns 0 and does nothing
	count = obj.erase("missing");
	CHECK(count == 0);
	CHECK(obj.size() == 2);
}

TEST_CASE("test-10")
{
	e::object b1(true);
	CHECK(b1.is_true());
	CHECK_FALSE(b1.is_false());

	e::object b2(false);
	CHECK_FALSE(b2.is_true());
	CHECK(b2.is_false());
}

TEST_CASE("erase object by key throws on wrong type")
{
	e::object arr{ 1, 2, 3 };
	REQUIRE(arr.is_array());

	CHECK_THROWS(arr.erase("key"));
}

TEST_CASE("erase array by index")
{
	e::object arr{ 10, 20, 30, 40 };
	REQUIRE(arr.is_array());

	arr.erase(1); // remove 20
	CHECK(arr.size() == 3);
	CHECK(arr[0].get<int64_t>() == 10);
	CHECK(arr[1].get<int64_t>() == 30);
	CHECK(arr[2].get<int64_t>() == 40);

	arr.erase(0); // remove 10
	CHECK(arr.size() == 2);
	CHECK(arr[0].get<int64_t>() == 30);
}

TEST_CASE("erase array by index throws")
{
	e::object arr{ 1, 2, 3 };
	REQUIRE(arr.is_array());

	CHECK_THROWS(arr.erase(3));      // out of range
	CHECK_THROWS(arr.erase(100));    // far out of range

	e::object obj{ { "a", 1 } };
	REQUIRE(obj.is_object());
	CHECK_THROWS(obj.erase(0));      // wrong type
}

TEST_CASE("erase array by iterator")
{
	e::object arr{ 1, 2, 3, 4 };
	REQUIRE(arr.is_array());

	auto it = std::next(arr.begin(), 2); // points at 3
	auto next = arr.erase(it);

	CHECK(arr.size() == 3);
	CHECK(arr[0].get<int64_t>() == 1);
	CHECK(arr[1].get<int64_t>() == 2);
	CHECK(arr[2].get<int64_t>() == 4);

	// the returned iterator points at the element following the erased one
	CHECK(next != arr.end());
	CHECK(next->get<int64_t>() == 4);

	// erase the last element; the returned iterator equals end()
	auto last = arr.erase(std::next(arr.begin(), 2));
	CHECK(last == arr.end());
}

TEST_CASE("erase object by iterator")
{
	e::object obj{
		{ "a", 1 },
		{ "b", 2 },
		{ "c", 3 }
	};
	REQUIRE(obj.is_object());

	auto it = std::next(obj.begin());
	REQUIRE(it->get<int64_t>() == 2);

	auto next = obj.erase(it);

	CHECK_FALSE(obj.contains("b"));
	CHECK(obj.size() == 2);
	CHECK(next != obj.end());
}

TEST_CASE("erase range")
{
	e::object arr{ 1, 2, 3, 4, 5 };
	REQUIRE(arr.is_array());

	auto first = std::next(arr.begin(), 1); // 2
	auto last = std::next(arr.begin(), 4);  // 5 (exclusive)
	auto next = arr.erase(first, last);

	CHECK(arr.size() == 2);
	CHECK(arr[0].get<int64_t>() == 1);
	CHECK(arr[1].get<int64_t>() == 5);
	CHECK(next != arr.end());
	CHECK(next->get<int64_t>() == 5);

	// erase the entire array
	auto end_it = arr.erase(arr.begin(), arr.end());
	CHECK(arr.empty());
	CHECK(end_it == arr.end());
}

TEST_CASE("erase range on object")
{
	e::object obj{
		{ "a", 1 },
		{ "b", 2 },
		{ "c", 3 }
	};
	REQUIRE(obj.is_object());

	obj.erase(std::next(obj.begin()), std::prev(obj.end()));

	CHECK(obj.size() == 2);
	CHECK(obj.contains("a"));
	CHECK(obj.contains("c"));
	CHECK_FALSE(obj.contains("b"));
}

TEST_CASE("erase resets scalar to null")
{
	// strings, numbers, and booleans reset to null when erased via iterator
	std::vector<e::object> scalars{
		e::object("hello"),
		e::object(42),
		e::object(3.14),
		e::object(true)
	};

	for (auto &obj : scalars)
	{
		REQUIRE_FALSE(obj.is_null());

		auto result = obj.erase(obj.begin());

		CHECK(obj.is_null());
		CHECK(obj.empty());
		CHECK(result == obj.end());
	}
}

TEST_CASE("erase throws on null")
{
	e::object obj;
	REQUIRE(obj.is_null());

	CHECK_THROWS(obj.erase(obj.begin()));
	CHECK_THROWS(obj.erase(obj.begin(), obj.end()));
}

TEST_CASE("erase with iterator from another object throws")
{
	e::object arr1{ 1, 2, 3 };
	e::object arr2{ 4, 5, 6 };

	auto foreign = std::next(arr2.begin());
	CHECK_THROWS(arr1.erase(foreign));

	e::object obj1{ { "a", 1 } };
	auto foreign_obj = std::next(obj1.begin());
	CHECK_THROWS(arr1.erase(foreign_obj, std::next(arr1.begin())));
}

TEST_CASE("array index out of range / negative yields null")
{
	zeep::http::scope scope;
	scope.put("arr", e::object{ "a", "b", "c" });

	CHECK(zeep::http::evaluate_el(scope, "*{arr[99]}").is_null());
	CHECK(zeep::http::evaluate_el(scope, "*{arr[-1]}").is_null());

	auto ok = zeep::http::evaluate_el(scope, "*{arr[1]}");
	REQUIRE(ok.is_string());
	CHECK(ok.get<std::string>() == "b");
}

TEST_CASE("JSON surrogate pair decoding")
{
	// A proper high+low surrogate pair must be combined into a single
	// supplementary-plane code point encoded as valid UTF-8, not emitted
	// as two separate (or lone) surrogates.
	auto decode = [](const char *json) -> std::string
	{
		auto obj = e::object::parse_JSON(json);
		REQUIRE(obj.is_string());
		return obj.get<std::string>();
	};

	// U+1F600 GRINNING FACE: \uD83D\uDE00 -> F0 9F 98 80
	CHECK(decode(R"("\uD83D\uDE00")") == "\xF0\x9F\x98\x80");

	// U+1F680 ROCKET: \uD83D\uDE80 -> F0 9F 9A 80
	CHECK(decode(R"("\uD83D\uDE80")") == "\xF0\x9F\x9A\x80");

	// U+10000 (first supplementary plane code point) -> F0 90 80 80
	CHECK(decode(R"("\uD800\uDC00")") == "\xF0\x90\x80\x80");

	// U+10FFFF (last valid unicode code point) -> F4 8F BF BF
	CHECK(decode(R"("\uDBFF\uDFFF")") == "\xF4\x8F\xBF\xBF");

	// Boundary: last valid BMP code point before the surrogate zone U+D7FF -> ED 9F BF
	CHECK(decode(R"("\uD7FF")") == "\xED\x9F\xBF");

	// U+00E9 LATIN SMALL LETTER E WITH ACUTE -> C3 A9
	CHECK(decode(R"("\u00E9")") == "\xC3\xA9");

	// BMP code point in the 3-byte range U+0800 -> E0 A0 80
	CHECK(decode(R"("\u0800")") == "\xE0\xA0\x80");

	// A high surrogate not followed by a low surrogate is invalid
	CHECK_THROWS_AS(e::object::parse_JSON(R"("\uD800")"), std::exception);
	CHECK_THROWS_AS(e::object::parse_JSON(R"("\uD83D")"), std::exception);
	CHECK_THROWS_AS(e::object::parse_JSON(R"("\uD800\uD800")"), std::exception);
	// A low surrogate must be matched by a preceding high surrogate
	CHECK_THROWS_AS(e::object::parse_JSON(R"("\uDC00")"), std::exception);
}

TEST_CASE("JSON nesting depth is bounded")
{
	// A deeply nested document must be rejected with a normal exception
	// instead of exhausting the stack (stack overflow / crash). An attacker
	// can supply such input unauthenticated (e.g. a JWT header or a request
	// body), so the recursion must be bounded.
	auto nested = [](int depth)
	{
		std::string j;
		for (int i = 0; i < depth; ++i)
			j += "{\"a\":";
		j += "1";
		for (int i = 0; i < depth; ++i)
			j += "}";
		return j;
	};

	// within the limit: parses successfully
	CHECK(e::object::parse_JSON(nested(500)).size() == 1);

	// beyond the limit: rejected cleanly, no crash
	CHECK_THROWS_AS(e::object::parse_JSON(nested(200'000)), std::exception);

	// arrays are bounded identically
	std::string arr;
	for (int i = 0; i < 200'000; ++i)
		arr += "[";
	arr += "1";
	for (int i = 0; i < 200'000; ++i)
		arr += "]";
	CHECK_THROWS_AS(e::object::parse_JSON(arr), std::exception);
}

TEST_CASE("get<NumberType> rejects out-of-range conversions")
{
	// Converting a floating-point value that is not representable in the
	// requested integer type (or is NaN/inf) is undefined behavior; it must
	// be rejected with a normal exception instead.
	using e::object;

	// in-range values convert (with truncation for float -> int)
	CHECK(object::parse_JSON("3.0").get<int>() == 3);
	CHECK(object::parse_JSON("3.9").get<int>() == 3);
	CHECK(object::parse_JSON("42.0").get<int>() == 42);
	{
		object i = int64_t{ 7 };
		CHECK(i.get<short>() == 7);
		CHECK(i.get<unsigned>() == 7u);
		object d = int64_t{ 5 };
		CHECK(d.get<double>() == 5.0);
		object f = 5.0;
		CHECK(f.get<double>() == 5.0);
		object neg = -1.0;
		CHECK_THROWS_AS(neg.get<unsigned>(), std::exception);
		object wide = int64_t{ 100'000 };
		CHECK_THROWS_AS(wide.get<short>(), std::exception);
		object inf = std::numeric_limits<double>::infinity();
		CHECK_THROWS_AS(inf.get<int64_t>(), std::exception);
		object ninf = -std::numeric_limits<double>::infinity();
		CHECK_THROWS_AS(ninf.get<int64_t>(), std::exception);
		object nan = std::numeric_limits<double>::quiet_NaN();
		CHECK_THROWS_AS(nan.get<int64_t>(), std::exception);
	}

	// float value too large / too small for int64_t
	CHECK_THROWS_AS(object::parse_JSON("1e19").get<int64_t>(), std::exception);
	CHECK_THROWS_AS(object::parse_JSON("-1e19").get<int64_t>(), std::exception);
	CHECK_THROWS_AS(object::parse_JSON("1.0e300").get<int64_t>(), std::exception);
	CHECK_THROWS_AS(object::parse_JSON("3e9").get<int>(), std::exception);
}

TEST_CASE("integer arithmetic overflow is rejected")
{
	using e::object;

	constexpr int64_t IMAX = std::numeric_limits<int64_t>::max();
	constexpr int64_t IMIN = std::numeric_limits<int64_t>::min();

	// operator +
	CHECK_THROWS_AS(object(IMAX) + object(int64_t{ 1 }), e::object_error);
	CHECK(throws_with([&] { (void)(object(IMAX) + object(IMAX)); }, "Integer overflow in operator +"));
	CHECK(throws_with([&] { (void)(object(IMIN) + object(int64_t{ -1 })); }, "Integer overflow in operator +"));
	CHECK(throws_with([&] { (void)(object(IMAX) + object(1.0)); }, "Integer overflow in operator +"));

	CHECK((object(IMAX) + object(int64_t{ 0 })).get<int64_t>() == IMAX);
	CHECK((object(IMAX) + object(IMIN)).get<int64_t>() == -1);
	CHECK((object(IMIN) + object(int64_t{ 1 })).get<int64_t>() == IMIN + 1);
	CHECK((object(IMIN) + object(IMAX)).get<int64_t>() == -1);

	// operator -
	CHECK_THROWS_AS(object(IMIN) - object(int64_t{ 1 }), e::object_error);
	CHECK(throws_with([&] { (void)(object(IMAX) - object(int64_t{ -1 })); }, "Integer overflow in operator -"));
	CHECK(throws_with([&] { (void)(object(IMIN) - object(IMAX)); }, "Integer overflow in operator -"));
	CHECK(throws_with([&] { (void)(object(IMIN) - object(1.0)); }, "Integer overflow in operator -"));

	CHECK((object(IMIN) - object(IMIN)).get<int64_t>() == 0);
	CHECK((object(IMAX) - object(IMAX)).get<int64_t>() == 0);
	CHECK((object(IMIN) - object(int64_t{ 0 })).get<int64_t>() == IMIN);
	CHECK((object(IMAX) - object(int64_t{ 1 })).get<int64_t>() == IMAX - 1);

	// operator *
	CHECK_THROWS_AS(object(IMAX) * object(int64_t{ 2 }), e::object_error);
	CHECK(throws_with([&] { (void)(object(IMAX) * object(IMAX)); }, "Integer overflow in operator *"));
	CHECK(throws_with([&] { (void)(object(IMIN) * object(int64_t{ -1 })); }, "Integer overflow in operator *"));
	CHECK(throws_with([&] { (void)(object(IMIN) * object(int64_t{ 2 })); }, "Integer overflow in operator *"));
	CHECK(throws_with([&] { (void)(object(IMIN) * object(int64_t{ -2 })); }, "Integer overflow in operator *"));
	CHECK(throws_with([&] { (void)(object(IMAX) * object(int64_t{ -2 })); }, "Integer overflow in operator *"));
	CHECK(throws_with([&] { (void)(object(IMIN) * object(2.0)); }, "Integer overflow in operator *"));

	CHECK((object(IMAX) * object(int64_t{ 1 })).get<int64_t>() == IMAX);
	CHECK((object(IMAX) * object(int64_t{ -1 })).get<int64_t>() == -IMAX);
	CHECK((object(IMIN) * object(int64_t{ 1 })).get<int64_t>() == IMIN);
	CHECK((object(int64_t{ 0 }) * object(IMIN)).get<int64_t>() == 0);
	CHECK((object(int64_t{ 2 }) * object(int64_t{ 3 })).get<int64_t>() == 6);

	// operator /
	CHECK_THROWS_AS(object(IMIN) / object(int64_t{ -1 }), e::object_error);
	CHECK(throws_with([&] { (void)(object(IMIN) / object(int64_t{ -1 })); }, "Integer overflow in operator /"));
	CHECK(throws_with([&] { (void)(object(IMIN) / object(-1.0)); }, "Integer overflow in operator /"));

	CHECK((object(IMAX) / object(int64_t{ -1 })).get<int64_t>() == -IMAX);
	CHECK((object(IMIN) / object(int64_t{ 1 })).get<int64_t>() == IMIN);
	CHECK((object(int64_t{ 7 }) / object(int64_t{ 2 })).get<int64_t>() == 3);

	// operator %
	CHECK_THROWS_AS(object(IMIN) % object(int64_t{ -1 }), e::object_error);
	CHECK(throws_with([&] { (void)(object(IMIN) % object(int64_t{ -1 })); }, "Integer overflow in operator %"));
	CHECK(throws_with([&] { (void)(object(IMIN) % object(-1.0)); }, "Integer overflow in operator %"));

	CHECK((object(IMIN) % object(int64_t{ 1 })).get<int64_t>() == 0);
	CHECK((object(IMAX) % object(int64_t{ 2 })).get<int64_t>() == 1);
	CHECK((object(int64_t{ -7 }) % object(int64_t{ 3 })).get<int64_t>() == -1);
}

TEST_CASE("division and modulo by zero is rejected")
{
	using e::object;

	CHECK((object(int64_t{ 0 }) / object(int64_t{ 5 })).get<int64_t>() == 0);
	CHECK((object(int64_t{ 0 }) % object(int64_t{ 5 })).get<int64_t>() == 0);

	CHECK_THROWS_AS(object(int64_t{ 5 }) / object(int64_t{ 0 }), e::object_error);
	CHECK(throws_with([&] { (void)(object(int64_t{ 5 }) / object(int64_t{ 0 })); }, "Division by zero"));
	CHECK_THROWS_AS(object(int64_t{ 5 }) / object(0.0), e::object_error);
	CHECK(throws_with([&] { (void)(object(int64_t{ 5 }) / object(0.0)); }, "Division by zero"));
	CHECK_THROWS_AS(object(int64_t{ 5 }) % object(int64_t{ 0 }), e::object_error);
	CHECK(throws_with([&] { (void)(object(int64_t{ 5 }) % object(int64_t{ 0 })); }, "Modulo by zero"));
	CHECK_THROWS_AS(object(int64_t{ 5 }) % object(0.0), e::object_error);
	CHECK(throws_with([&] { (void)(object(int64_t{ 5 }) % object(0.0)); }, "Modulo by zero"));
}
