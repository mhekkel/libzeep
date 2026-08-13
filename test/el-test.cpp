// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include <catch2/catch_test_macros.hpp>

#include <zeep/el/object.hpp>
#include <zeep/el/serializer.hpp>
#include <zeep/http/scope.hpp>

#include <zeem/zeem.hpp>

#include <chrono>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;
namespace e = zeep::el;

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

	static_assert(e::detail::is_serializable_to_object_v<Opname>);

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