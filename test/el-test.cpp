#include <zeep/exception.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/rest-controller.hpp>
#include <zeep/el/serializer.hpp>

#include "../src/signals.hpp"

#include "test-main.hpp"

#include "client-test-code.hpp"

#include <iostream>
#include <random>

using namespace std;
namespace z = zeep;
namespace zh = zeep::http;
namespace e = zeep::el;

struct Opname
{
	string id;
	map<string, float> standen;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long /*version*/)
	{
		// clang-format off
		ar & zeep::name_value_pair("id", id)
		   & zeep::name_value_pair("standen", standen);
		// clang-format on
	}

	auto operator<=>(const Opname &) const = default;
};

static_assert(not std::is_constructible_v<e::object, Opname>, "");

using a_map_type = map<string, float>;
static_assert(e::is_serializable_map_type_v<a_map_type>, "");

static_assert(std::is_constructible_v<e::object, std::string>, "");

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
	zh::scope scope;

	Opname opn{ "1", { { "een", 0.1f },
						 { "twee", 0.2f } } };

	static_assert(e::is_serializable_to_object_v<Opname>, "");

	scope.put("o1", e::to_object(opn));

	std::vector<Opname> opn_v{ opn, opn };

	scope.put("o2", e::to_object(opn_v));
}