#include "zeep/http/asio.hpp"

#include "test-main.hpp"

#include <zeep/exception.hpp>
#include <zeep/json-serializer.hpp>
#include <zeep/nvp.hpp>

// -----------------------------------------------------------------------

struct MyPOD2
{
	float f = -1.5;
	std::vector<int> v = { 1, 2, 3, 4 };

	bool operator==(const MyPOD2 &rhs) const
	{
		return f == rhs.f and v == rhs.v;
	}

	template <typename Archive>
	void serialize(Archive &ar, unsigned long /*version*/)
	{
		ar &zeep::make_nvp("f-f", f) & zeep::make_nvp("v", v);
	}
};

struct MyPOD
{
	std::string s;
	int i;
	std::optional<int> o{ 13 };
	std::vector<MyPOD2> fp{ 2, MyPOD2() };

	bool operator==(const MyPOD &rhs) const
	{
		return s == rhs.s and i == rhs.i and o == rhs.o and fp == rhs.fp;
	}

	template <typename Archive>
	void serialize(Archive &ar, unsigned long /*version*/)
	{
		ar &zeep::make_nvp("s-s", s) & zeep::make_nvp("i-i", i) & zeep::make_nvp("opt", o) & zeep::make_nvp("fp", fp);
	}
};

// -----------------------------------------------------------------------

enum class MyEnum
{
	aap,
	noot,
	mies
};

std::ostream &operator<<(std::ostream &os, MyEnum e)
{
	os << zeep::value_serializer<MyEnum>::to_string(e);
	return os;
}

NLOHMANN_JSON_SERIALIZE_ENUM(MyEnum,
	{
		{ MyEnum::aap, "aap" },
		{ MyEnum::noot, "noot" },
		{ MyEnum::mies, "mies" },
	});

struct MyPOD3
{
	MyEnum a;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long /*version*/)
	{
		ar &zeep::make_nvp("a", a);
	}
};

TEST_CASE("j_12")
{
	MyPOD p1{ "1", 2 }, p1a;

	static_assert(zeep::has_serialize_v<MyPOD, zeep::json::serializer>, "oh oh");

	json e = p1;

	p1a = e.get<MyPOD>();

	CHECK((p1 == p1a));

	MyPOD3 p3{ MyEnum::noot }, p3a;
	e = p3;

	p3a = e.get<MyPOD3>();

	CHECK(p3.a == p3a.a);
}
