#define BOOST_TEST_MODULE Serializer_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/config.hpp>

#include <array>
#include <iostream>
#include <deque>

#include <zeep/xml/document.hpp>

using namespace std;
using namespace zeep::xml;
namespace tt = boost::test_tools;

struct st_1
{
	int		i;
	string	s;

	template<class Archive>
	void serialize(Archive& ar, unsigned long /*v*/)
	{
		ar & ZEEP_ELEMENT_NAME_VALUE(i) & ZEEP_ELEMENT_NAME_VALUE(s);
	}
	
	bool operator==(const st_1& rhs) const { return i == rhs.i and s == rhs.s; }
};

typedef vector<st_1>	v_st_1;

BOOST_AUTO_TEST_CASE(serializer_1)
{
	using namespace zeep::xml::literals;

	auto doc = R"(<test>42</test>)"_xml;

	int32_t i = -1;

	deserializer ds(doc);
	ds.deserialize_element("test", i);

	BOOST_TEST(i == 42);

	document doc2;
	serializer sr(doc2);
	sr.serialize_element("test", i);

	BOOST_TEST(doc == doc2);
}

struct S
{
	int8_t a;
	float b;
	string c;

	bool operator==(const S& s) const { return a == s.a and b == s.b and c == s.c; }

	template<typename Archive>
	void serialize(Archive& ar, unsigned long /*version*/)
	{
		ar & element_nvp("a", a)
			& element_nvp("b", b)
			& element_nvp("c", c);
	}
};

BOOST_AUTO_TEST_CASE(serializer_2)
{
	using namespace zeep::xml::literals;

	auto doc = R"(<test><a>1</a><b>0.2</b><c>aap</c></test>)"_xml;

	S s;

	deserializer ds(doc);
	ds.deserialize_element("test", s);

	BOOST_TEST(s.a == 1);
	BOOST_TEST(s.b == 0.2, tt::tolerance(0.01));
	BOOST_TEST(s.c == "aap");

	document doc2;
	serializer sr(doc2);
	sr.serialize_element("test", s);

	BOOST_TEST(doc == doc2);
}

BOOST_AUTO_TEST_CASE(test_s_1)
{
	st_1 s1 = { 1, "aap" };
	
	document doc;
	doc.serialize("s1", s1);

	static_assert(zeep::has_serialize_v<S, serializer>, "Oeps");

	stringstream s;
	s << doc;
		
	BOOST_CHECK_EQUAL(s.str(), "<s1><i>1</i><s>aap</s></s1>");

	doc.clear();
	serializer sr(doc);
	sr.serialize_element("s1", s1);

	stringstream ss2;
	ss2 << doc;

	BOOST_CHECK_EQUAL(ss2.str(), "<s1><i>1</i><s>aap</s></s1>");

	st_1 s2;
	doc.deserialize("s1", s2);
	
	BOOST_CHECK(s1 == s2);
}

struct S_arr
{
	vector<int> vi;
	deque<S>	ds;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & element_nvp("vi", vi) & element_nvp("ds", ds);
	}
};

BOOST_AUTO_TEST_CASE(test_serialize_arrays)
{

	static_assert(std::experimental::is_detected_v<zeep::std_string_npos_t,std::string>, "oeps");
	static_assert(not std::experimental::is_detected_v<zeep::std_string_npos_t,std::vector<int>>, "oeps");
	static_assert(not std::experimental::is_detected_v<zeep::std_string_npos_t,int>, "oeps");

	vector<int> ii{ 1, 2, 3, 4 };

	element e("test");
	serializer sr(e);
	sr.serialize_element("i", ii);

	document doc;
	doc.insert(doc.begin(), e);// copy

	vector<int> ii2;
	deserializer dsr(e);
	dsr.deserialize_element("i", ii2);

	BOOST_TEST(ii == ii2);
}

BOOST_AUTO_TEST_CASE(test_serialize_arrays2)
{
	S_arr sa{
		{ 1, 2, 3, 4 },
		{
			{ 1, 0.5f, "aap" },
			{ 2, 1.5f, "noot" }
		}
	};

	static_assert(zeep::has_serialize_v<decltype(sa.ds)::value_type,serializer>, "oeps");
	static_assert(not zeep::has_serialize_v<decltype(sa.vi)::value_type,serializer>, "oeps");

	static_assert(zeep::is_serializable_type_v<decltype(sa.ds)::value_type,serializer>, "oeps");
	static_assert(zeep::is_serializable_type_v<decltype(sa.vi)::value_type,serializer>, "oeps");


	document doc;
	doc.serialize("test", sa);

	S_arr sa2;
	doc.deserialize("test", sa2);

	BOOST_TEST(sa.vi == sa2.vi);
	BOOST_TEST(sa.ds == sa2.ds);
}

BOOST_AUTO_TEST_CASE(serialize_arrays_2)
{
	using namespace zeep::xml::literals;

	element e("test");

	int i[] = { 1, 2, 3 };

	serializer sr(e);
	sr.serialize_element("i", i);

	ostringstream os;
	os << e;

	BOOST_TEST(os.str() == R"(<test><i>1</i><i>2</i><i>3</i></test>)");
}


BOOST_AUTO_TEST_CASE(serialize_container_1)
{
	using namespace zeep::xml::literals;

	element e("test");

	array<int,3> i = { 1, 2, 3 };

	serializer sr(e);
	sr.serialize_element("i", i);

	array<int,3> j;
	deserializer dsr(e);
	dsr.deserialize_element("i", j);

	BOOST_TEST(i == j);

	ostringstream os;
	os << e;

	BOOST_TEST(os.str() == R"(<test><i>1</i><i>2</i><i>3</i></test>)");
}



enum class E { aap, noot, mies };

struct Se
{
	E m_e;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & make_element_nvp("e", m_e);
	}
};

BOOST_AUTO_TEST_CASE(test_s_2)
{
	zeep::value_serializer<E>::instance("my-enum")
		(E::aap, "aap")
		(E::noot, "noot")
		(E::mies, "mies");

	vector<E> e = { E::aap, E::noot, E:: mies };

	document doc;
	// cannot create more than one root element in a doc:
	BOOST_CHECK_THROW(doc.serialize("test", e), zeep::exception);

	element test("test");
	serializer sr(test);
	sr.serialize_element("e", e);

	vector<E> e2;

	deserializer dsr(test);
	dsr.deserialize_element("e", e2);

	BOOST_TEST(e == e2);

	ostringstream s;
	s << test;
	BOOST_TEST(s.str() == "<test><e>aap</e><e>noot</e><e>mies</e></test>");

	Se se{E::aap};

	document doc2;
	doc2.serialize("s", se);

	ostringstream os;
	os << doc2;
	BOOST_TEST(os.str() == "<s><e>aap</e></s>");
}

BOOST_AUTO_TEST_CASE(test_optional)
{
	using namespace zeep::xml::literals;

	std::optional<std::string> s;

	document doc;
	// doc.serialize("test", s);

	// BOOST_TEST(doc == "<test/>"_xml);

	s.emplace("aap");
	doc.clear();
	doc.serialize("test", s);

	BOOST_TEST(doc == "<test>aap</test>"_xml);

	s.reset();

	doc.deserialize("test", s);

	BOOST_TEST((bool)s);
	BOOST_TEST(*s == "aap");
}

BOOST_AUTO_TEST_CASE(test_schema)
{
	using namespace zeep::xml::literals;

	element schema;
	type_map types;

	// schema_creator

}

struct date_t1
{
	date::sys_days sd;

	template<typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		ar & zeep::make_nvp("d", sd);
	}
};

BOOST_AUTO_TEST_CASE(test_date_1)
{
	using namespace zeep::xml::literals;
	using namespace date;

	auto doc = "<d>2022-12-06</d>"_xml;

	zeep::xml::deserializer ds(*doc.root());

	date_t1 t1;
	ds.deserialize_element(t1);

	BOOST_CHECK(t1.sd == 2022_y/12/6);
}

BOOST_AUTO_TEST_CASE(test_date_2)
{
	using namespace zeep::xml::literals;
	using namespace date;

	date_t1 t1{ 1966_y/6/27 };

	zeep::xml::document doc;
	zeep::xml::serializer s(doc);

	s.serialize_element(t1);

	BOOST_CHECK(doc == "<d>1966-06-27</d>"_xml);
	if (not (doc == "<d>1966-06-27</d>"_xml))
		std::cout << std::setw(2) << doc << std::endl;
}

struct time_t1
{
	std::chrono::system_clock::time_point st;

	template<typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		ar & zeep::make_nvp("t", st);
	}
};

BOOST_AUTO_TEST_CASE(test_time_1)
{
	using namespace zeep::xml::literals;
	using namespace date;

	auto doc = "<t>2022-12-06T00:01:02.34Z</t>"_xml;

	zeep::xml::deserializer ds(*doc.root());

	time_t1 t1;
	ds.deserialize_element(t1);

	BOOST_CHECK(t1.st == sys_days{2022_y/12/6} + 0h + 1min + 2.34s);
}

BOOST_AUTO_TEST_CASE(test_time_2)
{
	using namespace zeep::xml::literals;
	using namespace date;

	time_t1 t1{ sys_days{2022_y/12/6} + 1h + 2min + 3s };

	zeep::xml::document doc;
	zeep::xml::serializer s(doc);

	s.serialize_element(t1);

	auto ti = doc.find_first("//t");
	BOOST_ASSERT(ti != doc.end());

	auto ti_c = ti->get_content();

	std::regex rx(R"(^2022-12-06T01:02:03(\.0+)?Z$)");

	BOOST_CHECK(std::regex_match(ti_c, rx));
}

// BOOST_AUTO_TEST_CASE(test_s_2)
// {
// 	st_1 s1 = { 1, "aap" };

// 	v_st_1 v1;
// 	v1.push_back(s1);
// 	v1.push_back(s1);
	
// 	xml::document doc;
// 	BOOST_CHECK_THROW(doc.serialize("v1", v1), zeep::exception);
// }

// BOOST_AUTO_TEST_CASE(test_s_3)
// {
// 	st_1 st[] = { { 1, "aap" }, { 2, "noot" } };

// 	v_st_1 v1;
// 	v1.push_back(st[0]);
// 	v1.push_back(st[1]);
	
// 	xml::document doc("<v1/>");
// 	xml::serializer sr(doc.front());
// 	sr.serialize_element("s1", v1);

// 	stringstream s;
// 	s << doc;
	
// 	cout << s.str() << endl;
		
// 	BOOST_CHECK_EQUAL(s.str(), "<v1><s1><i>1</i><s>aap</s></s1><s1><i>2</i><s>noot</s></s1></v1>");
	
// 	v_st_1 v2;
// 	BOOST_CHECK_THROW(doc.deserialize("v1", v2), zeep::exception);

// 	xml::deserializer dr(doc.front());
// 	dr.deserialize_element("s1", v2);

// 	BOOST_CHECK(v1 == v2);
// }

// struct st_2
// {
// 	vector<string>	s;
	
// 	template<class Archive>
// 	void serialize(Archive& ar, unsigned long v)
// 	{
// 		ar & ZEEP_ELEMENT_NAME_VALUE(s);
// 	}
// };

// BOOST_AUTO_TEST_CASE(test_s_4)
// {
// 	st_2 s1;
// 	s1.s.push_back("aap");
// 	s1.s.push_back("noot");
	
// 	xml::document doc;
// 	doc.serialize("st2", s1);

// 	stringstream s;
// 	s << doc;
	
// 	BOOST_CHECK_EQUAL(s.str(), "<st2><s>aap</s><s>noot</s></st2>");
	
// 	st_2 s2;
// 	doc.deserialize("st2", s2);

// 	BOOST_CHECK(s1.s == s2.s);
// }
