#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/el/element.hpp>
#include <zeep/el/parser.hpp>
#include <zeep/el/serializer.hpp>
#include <zeep/exception.hpp>

using namespace std;
using json = zeep::el::element;

using namespace zeep::el::literals;

// -----------------------------------------------------------------------

struct MyPOD2
{
	float f = -1.5;
	std::vector<int> v = { 1, 2, 3, 4 };

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
	BOOST_CHECK_THROW(jint.as<vector<int>>(), std::exception);
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
	BOOST_CHECK_THROW(jint.as<vector<int>>(), std::exception);
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
	BOOST_CHECK_THROW(jfloat.as<vector<int>>(), std::exception);
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
	BOOST_CHECK_THROW(jfloat.as<vector<int>>(), std::exception);
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

	zeep::el::serializer<json>::serialize(j, true);
	BOOST_TEST(j.is_boolean());
	BOOST_TEST(j == true);

	zeep::el::serializer<json>::serialize(j, false);
	BOOST_TEST(j.is_boolean());
	BOOST_TEST(j == false);


	zeep::el::serializer<json>::serialize(j, 1);
	BOOST_TEST(j.is_number_int());
	BOOST_TEST(j == 1);

	zeep::el::serializer<json>::serialize(j, 1.0);
	BOOST_TEST(j.is_number_float());
	BOOST_TEST(j == 1.0);

	zeep::el::serializer<json>::serialize(j, "aap");
	BOOST_TEST(j.is_string());
	BOOST_TEST(j == "aap");

	std::optional<int> i;

	zeep::el::serializer<json>::serialize(j, i);
	BOOST_TEST(j.is_null());

	i = 1;
	zeep::el::serializer<json>::serialize(j, i);
	BOOST_TEST(j.is_number_int());
	BOOST_TEST(j == 1);

}

BOOST_AUTO_TEST_CASE(j_10)
{
	static_assert(std::is_constructible<json, const char*>::value, "oi");
}