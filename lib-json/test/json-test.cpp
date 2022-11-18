#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/json/element.hpp>
#include <zeep/json/parser.hpp>
#include <zeep/json/serializer.hpp>

#include <zeep/nvp.hpp>

#include <zeep/exception.hpp>

using namespace std;
using json = zeep::json::element;

using namespace zeep::json::literals;

// -----------------------------------------------------------------------

struct MyPOD2
{
	float f = -1.5;
	std::vector<int> v = { 1, 2, 3, 4 };

	bool operator==(const MyPOD2& rhs) const
	{
		return f == rhs.f and v == rhs.v;
	}

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_nvp("f-f", f)
		   & zeep::make_nvp("v", v)
		   ;
	}

};

struct MyPOD
{
	std::string				s;
	int						i;
	std::optional<int>		o{13};
	std::vector<MyPOD2>		fp{2, MyPOD2()};

	bool operator==(const MyPOD& rhs) const
	{
		return s == rhs.s and i == rhs.i and o == rhs.o and fp == rhs.fp;
	}

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_nvp("s-s", s)
		   & zeep::make_nvp("i-i", i)
		   & zeep::make_nvp("opt", o)
		   & zeep::make_nvp("fp", fp)
		   ;
	}
};

// -----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(j_1)
{
	json jnull = R"(null)"_json;

	BOOST_TEST(jnull.is_null());
}

BOOST_AUTO_TEST_CASE(j_2)
{
	json jint = R"(1)"_json;
	BOOST_TEST(jint.is_number());
	BOOST_TEST(jint.is_number_int());
	BOOST_TEST(jint.as<int>() == 1);
	BOOST_TEST(jint.as<float>() == 1.0);
	BOOST_TEST(jint.as<string>() == "1");
	BOOST_TEST(jint.as<bool>() == true);
#ifndef _MSC_VER
	BOOST_CHECK_THROW(jint.as<vector<int>>(), std::exception);
#endif
}

BOOST_AUTO_TEST_CASE(j_3)
{
	json jint = R"(-1)"_json;
	BOOST_TEST(jint.is_number());
	BOOST_TEST(jint.is_number_int());
	BOOST_TEST(jint.as<int>() == -1);
	BOOST_TEST(jint.as<float>() == -1.0);
	BOOST_TEST(jint.as<string>() == "-1");
	BOOST_TEST(jint.as<bool>() == true);
#ifndef _MSC_VER
	BOOST_CHECK_THROW(jint.as<vector<int>>(), std::exception);
#endif
}

BOOST_AUTO_TEST_CASE(j_4)
{
	json jfloat = R"(1.0)"_json;
	BOOST_TEST(jfloat.is_number());
	BOOST_TEST(jfloat.is_number_float());
	BOOST_TEST(jfloat.as<int>() == 1);
	BOOST_TEST(jfloat.as<float>() == 1.0);
	BOOST_TEST(jfloat.as<string>() == "1");
	BOOST_TEST(jfloat.as<bool>() == true);
#ifndef _MSC_VER
	BOOST_CHECK_THROW(jfloat.as<vector<int>>(), std::exception);
#endif
}

BOOST_AUTO_TEST_CASE(j_5)
{
	json jfloat = R"(-1.0)"_json;
	BOOST_TEST(jfloat.is_number());
	BOOST_TEST(jfloat.is_number_float());
	BOOST_TEST(jfloat.as<int>() == -1);
	BOOST_TEST(jfloat.as<float>() == -1.0);
	BOOST_TEST(jfloat.as<string>() == "-1");
	BOOST_TEST(jfloat.as<bool>() == true);
#ifndef _MSC_VER
	BOOST_CHECK_THROW(jfloat.as<vector<int>>(), std::exception);
#endif
}

BOOST_AUTO_TEST_CASE(j_6)
{
	for (string fs: { "1e3", "1.0e3", "10.0", "1.0", "1.0e-2", "0.1", 
					  "-1e3", "-1.0e3", "-10.0", "-1.0", "-1.0e-2", "-0.1" })
	{
		json jfloat;
		parse_json(fs, jfloat);

		BOOST_TEST(jfloat.is_number());
		BOOST_TEST(jfloat.is_number_float());
		BOOST_TEST(jfloat.as<int>() == static_cast<int>(stof(fs)));
		BOOST_TEST(jfloat.as<float>() == stof(fs));

		std::ostringstream s;
		s << stof(fs);
		fs = s.str();

		BOOST_TEST(jfloat.as<string>() == fs);
		BOOST_TEST(jfloat.as<bool>() == true);
	}
}

BOOST_AUTO_TEST_CASE(j_7)
{
	for (string fs: { "01", "-01" })
	{
		json jf;
		BOOST_CHECK_THROW(parse_json(fs, jf), zeep::exception);
	}
}

BOOST_AUTO_TEST_CASE(j_8)
{
	json j = {
		{ "aap", 1 },
		{ "noot", 2.0 }
	};

	size_t i = 0;
	for (auto& [key, value]: j.items())
	{
		switch (i++)
		{
			case 0:
				BOOST_TEST(key == "aap");
				BOOST_TEST(value.is_number_int());
				BOOST_TEST(value.as<int>() == 1);
				break;

			case 1:
				BOOST_TEST(key == "noot");
				BOOST_TEST(value.is_number_float());
				BOOST_TEST(value.as<int>() == 2.0);
				break;
		}
	}
}

BOOST_AUTO_TEST_CASE(j_9)
{
	json j;

	zeep::json::serializer<json>::serialize(j, true);
	BOOST_TEST(j.is_boolean());
	BOOST_TEST(j == true);

	zeep::json::serializer<json>::serialize(j, false);
	BOOST_TEST(j.is_boolean());
	BOOST_TEST(j == false);


	zeep::json::serializer<json>::serialize(j, 1);
	BOOST_TEST(j.is_number_int());
	BOOST_TEST(j == 1);

	zeep::json::serializer<json>::serialize(j, 1.0);
	BOOST_TEST(j.is_number_float());
	BOOST_TEST(j == 1.0);

	zeep::json::serializer<json>::serialize(j, "aap");
	BOOST_TEST(j.is_string());
	BOOST_TEST(j == "aap");

	std::optional<int> i;

	zeep::json::serializer<json>::serialize(j, i);
	BOOST_TEST(j.is_null());

	i = 1;
	zeep::json::serializer<json>::serialize(j, i);
	BOOST_TEST(j.is_number_int());
	BOOST_TEST(j == 1);

}

BOOST_AUTO_TEST_CASE(j_10)
{
	static_assert(std::is_constructible<json, const char*>::value, "oi");
}

enum class MyEnum {
	aap, noot, mies
};

std::ostream& operator<<(std::ostream& os, MyEnum e)
{
	os << zeep::value_serializer<MyEnum>::to_string(e);
	return os;
}

BOOST_AUTO_TEST_CASE(j_11)
{
	zeep::value_serializer<MyEnum>::instance()
		("aap", MyEnum::aap)
		("noot", MyEnum::noot)
		("mies", MyEnum::mies);
		
	json e = MyEnum::aap;
	BOOST_TEST(e.as<std::string>() == "aap");
	// BOOST_TEST(e.as<MyEnum>() == MyEnum::aap);

	// reinit the enum serializer
	zeep::value_serializer<MyEnum>::init({
		{ MyEnum::aap, "aap" },
		{ MyEnum::noot, "noot" },
		{ MyEnum::mies, "mies" }
	});

	e = MyEnum::noot;
	BOOST_TEST(e.as<std::string>() == "noot");
}

struct MyPOD3
{
	MyEnum a;
	
	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_nvp("a", a)
		   ;
	}
};

BOOST_AUTO_TEST_CASE(j_12)
{
	MyPOD p1{ "1", 2 }, p1a;

	json e;
	to_element(e, p1);

	from_element(e, p1a);

	BOOST_TEST((p1 == p1a));

	MyPOD3 p3{MyEnum::noot}, p3a;
	to_element(e, p3);

	from_element(e, p3a);

	BOOST_TEST(p3.a == p3a.a);
}

// TODO: re-enable
// struct POD3
// {
// 	boost::posix_time::ptime now;

// 	template<typename Archive>
// 	void serialize(Archive& ar, unsigned long version)
// 	{
// 		ar & zeep::make_nvp("now", now);
// 	}
// };

// BOOST_AUTO_TEST_CASE(j_13)
// {
// 	POD3 p{ boost::posix_time::second_clock::local_time() }, pa;

// 	json e;
// 	to_element(e, p);

// 	from_element(e, pa);

// 	BOOST_TEST((p.now == pa.now));
// }



enum class E { aap, noot, mies };

struct Se
{
	E m_e;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & zeep::make_nvp("e", m_e);
	}

	bool operator==(const Se& se) const
	{
		return m_e == se.m_e;
	}
};


BOOST_AUTO_TEST_CASE(j_test_array_1)
{
	using array_type = std::vector<Se>;

	array_type v{ { E::aap }, { E::noot }, { E::mies }}, v2;

	json e;
	to_element(e, v);
	from_element(e, v2);

	BOOST_TEST(v == v2);
}

