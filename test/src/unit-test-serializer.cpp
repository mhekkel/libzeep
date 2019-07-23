#include <zeep/config.hpp>

#include <iostream>

#include <boost/test/unit_test.hpp>

#include <zeep/xml/document.hpp>
#include <zeep/xml/writer.hpp>

using namespace std;
using namespace zeep;


struct st_1
{
	int		i;
	string	s;

	template<class Archive>
	void serialize(Archive& ar, unsigned long v)
	{
		ar & ZEEP_ELEMENT_NAME_VALUE(i) & ZEEP_ELEMENT_NAME_VALUE(s);
	}
	
	bool operator==(const st_1& rhs) const { return i == rhs.i and s == rhs.s; }
};

typedef vector<st_1>	v_st_1;

BOOST_AUTO_TEST_CASE(test_s_1)
{
	st_1 s1 = { 1, "aap" };
	
	xml::document doc;
	doc.serialize("s1", s1);

	stringstream s;
	xml::writer w(s);
	w.set_wrap(false);
	w.set_indent(0);

	doc.write(w);
	
	cout << s.str() << endl;
		
	BOOST_CHECK_EQUAL(s.str(), "<s1><i>1</i><s>aap</s></s1>");
	
	st_1 s2;
	doc.deserialize("s1", s2);
	
	BOOST_CHECK(s1 == s2);
}

BOOST_AUTO_TEST_CASE(test_s_2)
{
	st_1 s1 = { 1, "aap" };

	v_st_1 v1;
	v1.push_back(s1);
	v1.push_back(s1);
	
	xml::document doc;
	BOOST_CHECK_THROW(doc.serialize("v1", v1), zeep::exception);
}

BOOST_AUTO_TEST_CASE(test_s_3)
{
	st_1 st[] = { { 1, "aap" }, { 2, "noot" } };

	v_st_1 v1;
	v1.push_back(st[0]);
	v1.push_back(st[1]);
	
	xml::document doc("<v1/>");
	xml::serializer sr(doc.child());
	sr.serialize_element("s1", v1);

	stringstream s;
	xml::writer w(s);
	w.set_wrap(false);
	w.set_indent(0);

	doc.write(w);
	
	cout << s.str() << endl;
		
	BOOST_CHECK_EQUAL(s.str(), "<v1><s1><i>1</i><s>aap</s></s1><s1><i>2</i><s>noot</s></s1></v1>");
	
	v_st_1 v2;
	BOOST_CHECK_THROW(doc.deserialize("v1", v2), zeep::exception);

	xml::deserializer dr(doc.child());
	dr.deserialize_element("s1", v2);

	BOOST_CHECK(v1 == v2);
}

struct st_2
{
	vector<string>	s;
	
	template<class Archive>
	void serialize(Archive& ar, unsigned long v)
	{
		ar & ZEEP_ELEMENT_NAME_VALUE(s);
	}
};

BOOST_AUTO_TEST_CASE(test_s_4)
{
	st_2 s1;
	s1.s.push_back("aap");
	s1.s.push_back("noot");
	
	xml::document doc;
	doc.serialize("st2", s1);

	stringstream s;
	xml::writer w(s);
	w.set_wrap(false);
	w.set_indent(0);

	doc.write(w);
	
	BOOST_CHECK_EQUAL(s.str(), "<st2><s>aap</s><s>noot</s></st2>");
	
	st_2 s2;
	doc.deserialize("st2", s2);

	BOOST_CHECK(s1.s == s2.s);
}
