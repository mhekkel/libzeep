#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/el/element.hpp>
#include <zeep/el/parser.hpp>
#include <zeep/el/serializer.hpp>
#include <zeep/exception.hpp>

using namespace std;
using json = zeep::el::element;
using my_opt = boost::optional<int>;

// -----------------------------------------------------------------------

struct MyPOD2
{
	float f = -1.5;
	std::vector<int> v = { 1, 2, 3, 4 };

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_attribute_nvp("f-f", f)
		   & zeep::make_attribute_nvp("v", v)
		   ;
	}

};

struct MyPOD
{
	std::string				s;
	int						i;
	boost::optional<int>	o{13};
	std::vector<MyPOD2>		fp{2, MyPOD2()};

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_attribute_nvp("s-s", s)
		   & zeep::make_attribute_nvp("i-i", i)
		   & zeep::make_attribute_nvp("opt", o)
		   & zeep::make_element_nvp("fp", fp)
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

// int main()
// {
// 	try
// 	{

// 		my_opt op;

// 		static_assert(zeep::is_serializable_optional_type<my_opt, zeep::serializer<json>>::value, "da's niet ok");

// 		zeep::serializer<json> sr;
// 		MyPOD p = { "test", 42 };

// 		p.serialize(sr, 0);


// 		std::cout << sr.m_elem << std::endl;

// 		MyPOD p2;
// 		zeep::deserializer<json> dsr(sr.m_elem);
// 		p2.serialize(dsr, 0);

// 		// json x = R"({
// 		// 	"a": 1.1
// 		// })"_json;

// 		// x["aap"] = 1;
// 		// x["noot"] = vector<int>({ 1, 2, 3 });

// 		// json o = {
// 		// 	{ "a": 1 },
// 		// 	{ "b": "aap" },
// 		// 	{ "c:" { "sub": 1} },
// 		// 	[ 1, 2, "3", nullptr ]
// 		// };

// 		json j_null;
// 		json j_array(json::value_type::array);
// 		json j_object(json::value_type::object);
// 		json j_string(json::value_type::string);
// 		json j_number(json::value_type::number_int);
// 		json j_bool(json::value_type::boolean);

// 		json j_string2("aap");
// 		json j_number2(1.0f);
// 		json j_number3(2L);
// 		json j_number4(3UL);

// 		json j_bool2(true);

// 		json j_array2(std::vector<int>{ 1, 2, 3});
// 		json j_array3({ 1, 2, 3});

// 		static_assert(zeep::is_serializable_array_type<std::vector<float>, zeep::serializer<json>>::value, "een");
// 		static_assert(zeep::el::detail::is_compatible_type<std::vector<float>>::value, "een");

// 		std::vector<float> vf({ 1.0, 1.1, 1.2 });
// 		json j_array5 = vf;

// 		std::map<std::string,int> m = {
// 			{ "een", 1 }, { "twee", 2 }
// 		};
// 		json j_object2(m);
// 		j_object2["drie"] = vf;

// 		json j_array4(10, j_object2);

// 		static_assert(std::experimental::is_detected_exact
// 			<void, zeep::el::detail::to_element_function, zeep::el::element_serializer<int,void>, zeep::el::element&, int>::value, "moet!");

// 		static_assert(std::experimental::is_detected_exact
// 			<void, zeep::el::detail::to_element_function, zeep::el::element_serializer<typename json::string_type,void>, zeep::el::element&, typename json::string_type>::value, "moet!");

// 		static_assert(not zeep::el::detail::is_element<int>::value, "wowo");
// 		static_assert(zeep::el::detail::is_element<decltype(j_null)>::value, "wowo");
// 		static_assert(zeep::el::detail::has_to_element<bool>::value, "Oeps");

// 		std::cout << j_null << std::endl
// 				  << j_array << std::endl
// 				  << j_array2 << std::endl
// 				  << j_array3 << std::endl
// 				  << j_array5 << std::endl
// 				  << j_object << std::endl
// 				  << j_object2 << std::endl
// 				  << j_string << std::endl
// 				  << j_string2 << std::endl
// 				  << j_number << std::endl
// 				  << j_number2 << std::endl
// 				  << j_number3 << std::endl
// 				  << j_number4 << std::endl
// 				  << j_array4 << std::endl

// 				  << j_bool << std::endl
// 				  << j_bool2 << std::endl
// 				  ;


// 		// get

// 		bool b = *j_bool2.get_ptr<bool*>();
// 		assert(b == true);

// 		std::string s = *j_string2.get_ptr<std::string*>();
// 		assert(s == "aap");

// 		json j_string3(s);
// 		assert(j_string3 == j_string2);

// 		s = j_string2.as<std::string>();

// 		// assert(j_string2.as<std::string>() == "aap");

// 		std::cout << std::endl;
		
// 		std::cout << j_number2.as<float>() << std::endl
// 				  << j_number3.as<float>() << std::endl
// 				  << j_number4.as<float>() << std::endl
// 				  << std::endl;
		

// 		auto a = j_array3.as<std::vector<float>>();

// 		for (float f: a)
// 			std::cout << f << std::endl;

// 		std::cout << std::endl
// 				  << j_object2["een"] << std::endl
// 				  << j_array4[2]["twee"] << std::endl
// 				  << std::endl;
		

// 		// test nan
// 		double d = 1 / 0.;
// 		json nan_test{ "d", d };
// 		std::cout << nan_test << std::endl;

// 		// for (auto[key, value]: j_object2.items())
// 		// {
// 		// 	std::cout << "key: " << key << " value: " << value << std::endl;
// 		// }

// 		json v;
// 		auto sj = R"(

// {
// 	"null": null,
// 	"array": [ 1, 2, 3 ],
// 	"object": { "aap": 1, "noot": 2 },
// 	"int": 42,
// 	"float": -3.14e+0,
// 	"bool": true,
// 	"str": "een tab: '\t' en dan een \\n: '\n' en een \\b:\b en een heel rare hex: '\u263C' en nog een: '\u0000'"
// }


// 		)"_json;

// 		std::cout << "parsed: " << std::endl
// 				  << sj << std::endl; 

// 		std::ofstream file("/tmp/json-test.json");
// 		if (file.is_open())
// 			file << sj;

// 	}
// 	catch(const std::exception& e)
// 	{
// 		std::cerr << e.what() << '\n';
// 		exit(1);
// 	}

// 	return 0;
// }
