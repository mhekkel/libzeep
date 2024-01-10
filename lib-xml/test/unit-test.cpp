#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <vector>

#include <zeep/xml/document.hpp>
#include <zeep/exception.hpp>
#include "../lib-xml/src/html-named-characters.hpp"

using namespace std;
namespace zx = zeep::xml;

BOOST_AUTO_TEST_CASE(xml_1)
{
	zx::element n("data", { { "attr1", "value-1" }, { "attr2", "value-2" } });

	BOOST_TEST(n.name() == "data");
	BOOST_TEST(n.attributes().empty() == false);
	BOOST_TEST(n.attributes().size() == 2);
	BOOST_TEST(n.attributes().begin() != n.attributes().end());

	size_t i = 0;
	for (auto& [name, value]: n.attributes())
	{
		switch (i++)
		{
			case 0:
				BOOST_TEST(name == "attr1");
				BOOST_TEST(value == "value-1");
				break;
			
			case 1:
				BOOST_TEST(name == "attr2");
				BOOST_TEST(value == "value-2");
				break;
		}
	}

	ostringstream s;
	s << n;

	BOOST_TEST(s.str() == R"(<data attr1="value-1" attr2="value-2"/>)");

	ostringstream s2;
	s2 << setw(2) << setiosflags(ios_base::left) << n << endl;

	const char* test = R"(<data attr1="value-1"
      attr2="value-2"/>
)";

	BOOST_TEST(s2.str() == test);

	n.validate();
}

BOOST_AUTO_TEST_CASE(xml_2)
{
	zx::element e("test");
	e.nodes().emplace_back(zx::comment("commentaar"));
	zx::element::node_iterator i = e.nodes().begin();
	BOOST_TEST(i == e.nodes().begin());
	BOOST_TEST(i != e.nodes().end());
	BOOST_TEST(i->str() == "commentaar");

	zx::element::iterator j = e.begin();
	BOOST_TEST(j == e.begin());
	BOOST_TEST(j == e.end());
}

BOOST_AUTO_TEST_CASE(xml_3)
{
	zx::element e("test");

	zx::element a("aap");

	e.nodes().emplace(e.end(), a);
	BOOST_TEST(a.name() == "aap");

	e.nodes().emplace(e.end(), std::move(a));
	BOOST_TEST(a.name() == "");

	zx::element b("noot");
	zx::node& n = b;

	e.nodes().emplace(e.end(), n);
	BOOST_TEST(b.name() == "noot");

	const zx::node& n2 = b;
	e.nodes().emplace(e.end(), n2);
	BOOST_TEST(b.name() == "noot");

	// zx::node&& n3 = std::move(b);
	// e.nodes().emplace(e.end(), n3);
	// BOOST_TEST(b.name() == "");

	e.attributes().emplace("attr1", "value1");

	ostringstream s;
	s << e;
	BOOST_TEST(s.str() == R"(<test attr1="value1"><aap/><aap/><noot/><noot/></test>)");
}

BOOST_AUTO_TEST_CASE(xml_attributes_1)
{
	using namespace zx::literals;

	auto doc = R"(<test xmlns:m="http://www.hekkelman.com">
<t1 m:a="v"/>
</test>)"_xml;

	auto& t = doc.front().front();

	for (auto& a: t.attributes())
	{
		BOOST_TEST(a.name() == "a");
		BOOST_TEST(a.get_qname() == "m:a");
		BOOST_TEST(a.get_ns() == "http://www.hekkelman.com");
	}

	for (auto a: t.attributes())
	{
		BOOST_TEST(a.name() == "a");
		BOOST_TEST(a.get_qname() == "m:a");

		// the attribute was copied and thus lost namespace information
		BOOST_TEST(a.get_ns() != "http://www.hekkelman.com");
	}
}

BOOST_AUTO_TEST_CASE(xml_emplace)
{
	zx::element e("test");

	e.emplace_back("test2", { { "a1", "v1" }, { "a2", "v2" }});

	ostringstream s;
	s << e;
	BOOST_TEST(s.str() == R"(<test><test2 a1="v1" a2="v2"/></test>)");

	e.emplace_front("test1", { { "a1", "v1" }, { "a2", "v2" }});

	ostringstream s2;
	s2 << e;
	BOOST_TEST(s2.str() == R"(<test><test1 a1="v1" a2="v2"/><test2 a1="v1" a2="v2"/></test>)");
}


BOOST_AUTO_TEST_CASE(xml_4)
{
	zx::element e("test");
	e.emplace_back(zx::element("test2", { { "attr1", "een" }, { "attr2", "twee" } }));

	ostringstream s;
	s << e;
	BOOST_TEST(s.str() == R"(<test><test2 attr1="een" attr2="twee"/></test>)");
}

BOOST_AUTO_TEST_CASE(xml_5_compare)
{
	zx::element a("test", { { "a", "v1" }, { "b", "v2" } });
	zx::element b("test", { { "b", "v2" }, { "a", "v1" } });

	BOOST_TEST(a == b);	
}

BOOST_AUTO_TEST_CASE(xml_container_and_iterators)
{
	zx::element e("test");

	zx::element n("a");
	e.insert(e.begin(), move(n));
	e.back().set_content("aap ");
	
	e.emplace_back("b").set_content("noot ");
	e.emplace_back("c").set_content("mies");

	BOOST_TEST(e.size() == 3);
	BOOST_TEST(not e.empty());

	BOOST_TEST(e.front().parent() == &e);
	BOOST_TEST(e.back().parent() == &e);

	BOOST_TEST(e.begin() != e.end());

	BOOST_TEST(e.str() == "aap noot mies");

	e.erase(next(e.begin()));
	BOOST_TEST(e.str() == "aap mies");

	ostringstream s1;
	s1 << setw(2) << left << e << endl;
	BOOST_TEST(s1.str() == R"(<test>
  <a>aap </a>
  <c>mies</c>
</test>
)");

	e.validate();

	ostringstream s2;
	s2 << e;
	BOOST_TEST(s2.str() == R"(<test><a>aap </a><c>mies</c></test>)");

	e.pop_front();
	BOOST_TEST(e.size() == 1);
	BOOST_TEST(e.front().name() == "c");

	e.push_front({"aa"});
	BOOST_TEST(e.size() == 2);
	BOOST_TEST(e.front().name() == "aa");

	e.pop_back();
	BOOST_TEST(e.size() == 1);
	BOOST_TEST(e.back().name() == "aa");
	BOOST_TEST(e.front().name() == "aa");

	e.pop_back();
	BOOST_TEST(e.empty());

	e.validate();
}

BOOST_AUTO_TEST_CASE(xml_copy)
{
	zx::element e("test", { { "a", "een" }, { "b", "twee" } });

	e.push_back(e);
	e.push_back(e);

	zx::element c("c", { { "x", "0" }});
	c.push_back(e);
	c.push_front(e);
	
	zx::element c2 = c;

	BOOST_TEST(c == c2);
}

BOOST_AUTO_TEST_CASE(xml_copy2)
{
	zx::element e("test", { { "a", "een" }, { "b", "twee" } });
	e.emplace_back("x1");
	e.nodes().emplace_back(zx::comment("bla"));
	e.emplace_back("x2");

	auto e1 = e;

	zx::element c1("test");
	c1.emplace_back(std::move(e));

	auto c2 = c1;

	zx::element c3("test");
	for (auto& n: c1)
		c3.emplace_back(std::move(n));

	BOOST_TEST(c2 == c3);

	zx::element e2("test", { { "a", "een" }, { "b", "twee" } });
	for (auto& n: c2.front().nodes())
		e2.nodes().emplace_back(std::move(n));

	BOOST_TEST(e2 == e1);

	e1.validate();
	e2.validate();
}

BOOST_AUTO_TEST_CASE(xml_iterators)
{
	zx::element e("test");
	for (int i = 0; i < 10; ++i)
		e.emplace_back("n").set_content(to_string(i));

	auto bi = e.begin();
	auto ei = e.end();

	for (int i = 0; i < 10; ++i)
	{
		BOOST_TEST((bi + i)->get_content() == to_string(i));
		BOOST_TEST((ei - i - 1)->get_content() == to_string(9 - i));
	}
}

BOOST_AUTO_TEST_CASE(xml_iterators_2)
{
	zx::element e("test");
	for (int i = 0; i < 10; ++i)
		e.emplace_back("n").set_content(to_string(i));

	auto bi = e.begin();
	auto ei = e.end();

	for (int i = 0; i < 10; ++i)
	{
		BOOST_TEST((bi + i)->get_content() == to_string(i));
		BOOST_TEST((ei - i - 1)->get_content() == to_string(9 - i));
	}

	std::vector<zx::node*> nodes;
	for (auto& n: e.nodes())
		nodes.push_back(&n);

	BOOST_TEST(nodes.size() == 10);

	for (int i = 0; i < 10; ++i)
	{
		zx::element* el = dynamic_cast<zx::element*>(nodes[i]);
		BOOST_TEST(el != nullptr);
		BOOST_TEST(el->get_content() == to_string(i));
	}
}

BOOST_AUTO_TEST_CASE(xml_attributes)
{
	zx::element e("test", { { "a", "1" }, { "b", "2" } });

	auto& attr = e.attributes();

	BOOST_TEST(attr.contains("a"));
	BOOST_TEST(attr.contains("b"));
	BOOST_TEST(not attr.contains("c"));

	BOOST_TEST(attr.find("a")->value() == "1");
	BOOST_TEST(attr.find("b")->value() == "2");
	BOOST_TEST(attr.find("c") == attr.end());
	
	auto i = attr.emplace("c", "3");

	BOOST_TEST(attr.contains("c"));
	BOOST_TEST(attr.find("c") == i.first);
	BOOST_TEST(attr.find("c")->value() == "3");
	BOOST_TEST(i.second == true);

	i = attr.emplace("c", "3a");

	BOOST_TEST(attr.contains("c"));
	BOOST_TEST(attr.find("c") == i.first);
	BOOST_TEST(attr.find("c")->value() == "3a");
	BOOST_TEST(i.second == false);
}

BOOST_AUTO_TEST_CASE(xml_doc)
{
	zx::document doc;

	zx::element e("test", { { "a", "1" }, { "b", "2" } });
	doc.push_back(move(e));

	zx::document doc2(R"(<test a="1" b="2"/>)");

	BOOST_TEST(doc == doc2);

	using namespace zx::literals;

	auto doc3 = R"(<test a="1" b="2"/>)"_xml;
	BOOST_TEST(doc == doc3);

	auto doc4 = R"(<l1><l2><l3><l4/></l3></l2></l1>)"_xml;

	BOOST_TEST(doc4.size() == 1);

	auto l1 = doc4.front();
	BOOST_TEST(l1.get_qname() == "l1");
	BOOST_TEST(l1.size() == 1);

	auto l2 = l1.front();
	BOOST_TEST(l2.get_qname() == "l2");
	BOOST_TEST(l2.size() == 1);

	auto l3 = l2.front();
	BOOST_TEST(l3.get_qname() == "l3");
	BOOST_TEST(l3.size() == 1);

	auto l4 = l3.front();
	BOOST_TEST(l4.get_qname() == "l4");
	BOOST_TEST(l4.empty());

	auto i = l3.find_first("./l4");
	BOOST_TEST(i != l3.end());
	l3.erase(i);

	BOOST_TEST(l3.empty());

	i = l1.find_first(".//l3");
	BOOST_TEST(i != l1.end());

	BOOST_CHECK_THROW(l1.erase(i), zeep::exception);

	l1.erase(l1.begin());

	BOOST_TEST(l1.empty());
}

BOOST_AUTO_TEST_CASE(xml_doc2)
{
	zx::document doc;
	doc.emplace_back("first", { { "a1", "v1" }});
	BOOST_CHECK_THROW(doc.emplace_back("second"), zeep::exception);
}

BOOST_AUTO_TEST_CASE(xml_xpath)
{
	using namespace zx::literals;
	auto doc = R"(<test><a/><a/><a/></test>)"_xml;

	auto r = doc.find("//a");
	BOOST_TEST(r.size() == 3);
	BOOST_TEST(r.front()->get_qname() == "a");
}

BOOST_AUTO_TEST_CASE(xml_xpath_2)
{
	using namespace zx::literals;
	auto doc = R"(
<test>
	<b/>
	<b>
		<c>
			<a>x</a>
		</c>
	</b>
	<b>
		<c>
			<a>
				<![CDATA[x]]>
			</a>
		</c>
	</b>
	<b>
		<c z='z'>
			<a>y</a>
		</c>
	</b>
</test>
)"_xml;

	auto r = doc.find("//b[c/a[contains(text(),'x')]]");
	BOOST_TEST(r.size() == 2);
	BOOST_TEST(r.front()->get_qname() == "b");

	auto r2 = doc.find("//b/c[@z='z']/a[text()='y']");
	BOOST_TEST(r2.size() == 1);
	BOOST_TEST(r2.front()->get_qname() == "a");

}

BOOST_AUTO_TEST_CASE(xml_namespaces)
{
	using namespace zx::literals;

    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<div>
<m:test0/>
<test1 m:if="${true}"/><test2 m:unless="${true}"/>
</div>
</data>
    )"_xml;

	auto& data = doc.front();
	BOOST_TEST(data.parent() == &doc);
	BOOST_TEST(data.name() == "data");
	BOOST_TEST(data.get_ns().empty());

	BOOST_TEST(data.empty() == false);
	BOOST_TEST(data.begin() != data.end());

	auto& div = data.front();
	BOOST_TEST(div.name() == "div");
	BOOST_TEST(div.get_ns().empty());
	BOOST_TEST(div.parent() == &data);

	auto& test0 = div.front();
	BOOST_TEST(test0.parent() == &div);
	BOOST_TEST(test0.name() == "test0");
	BOOST_TEST(test0.get_qname() == "m:test0");
	BOOST_TEST(test0.get_ns() == "http://www.hekkelman.com/libzeep/m2");

	auto& test1 = *(div.begin() + 1);
	BOOST_TEST(test1.parent() == &div);
	BOOST_TEST(test1.name() == "test1");
	BOOST_TEST(test1.get_ns().empty());

	BOOST_TEST(test1.attributes().size() == 1);
	auto& test1_if = *test1.attributes().begin();
	BOOST_TEST(test1_if.name() == "if");
	BOOST_TEST(test1_if.get_qname() == "m:if");
	BOOST_TEST(test1_if.get_ns() == "http://www.hekkelman.com/libzeep/m2");
	
	auto& test2 = *(div.begin() + 2);
	BOOST_TEST(test2.parent() == &div);
	BOOST_TEST(test2.name() == "test2");
	BOOST_TEST(test2.get_ns().empty());

	BOOST_TEST(test2.attributes().size() == 1);
	auto& test2_unless = *test2.attributes().begin();
	BOOST_TEST(test2_unless.name() == "unless");
	BOOST_TEST(test2_unless.get_qname() == "m:unless");
	BOOST_TEST(test2_unless.get_ns() == "http://www.hekkelman.com/libzeep/m2");
}

BOOST_AUTO_TEST_CASE(xml_namespaces_2)
{
	using namespace zx::literals;

    auto doc = R"(<?xml version="1.0"?>
<data xmlns="http://www.hekkelman.com/libzeep">
<x a="1">
<y a="2"/>
</x>
</data>
    )"_xml;

	auto& data = doc.front();
	BOOST_TEST(data.parent() == &doc);
	BOOST_TEST(data.name() == "data");
	BOOST_TEST(data.get_ns() == "http://www.hekkelman.com/libzeep");

	BOOST_TEST(data.empty() == false);
	BOOST_TEST(data.begin() != data.end());

	auto& x = data.front();
	BOOST_TEST(x.name() == "x");
	BOOST_TEST(x.get_qname() == "x");
	BOOST_TEST(x.get_ns() == "http://www.hekkelman.com/libzeep");
	BOOST_TEST(x.parent() == &data);

	auto ax = x.attributes().find("a");
	BOOST_TEST(ax != x.attributes().end());
	BOOST_TEST(ax->value() == "1");
	BOOST_TEST(ax->get_ns() == "http://www.hekkelman.com/libzeep");

	auto& y = x.front();
	BOOST_TEST(y.parent() == &x);
	BOOST_TEST(y.name() == "y");
	BOOST_TEST(y.get_qname() == "y");
	BOOST_TEST(y.get_ns() == "http://www.hekkelman.com/libzeep");

	auto ay = y.attributes().find("a");
	BOOST_TEST(ay != y.attributes().end());
	BOOST_TEST(ay->value() == "2");
	BOOST_TEST(ay->get_ns() == "http://www.hekkelman.com/libzeep");

	zx::element data2("data", { { "xmlns", "http://www.hekkelman.com/libzeep" }});
	auto& x2 = data2.emplace_back("x", { { "a", "1" } });
	x2.emplace_back("y", { { "a", "2" } });

	BOOST_TEST(data == data2);
}

BOOST_AUTO_TEST_CASE(xml_namespaces_3)
{
	using namespace zx::literals;

    auto doc = R"(<?xml version="1.0"?>
<data xmlns="http://www.hekkelman.com/libzeep" xmlns:a="http://a.com/">
<x a="1">
<y a:a="2"/>
</x>
</data>
    )"_xml;

	auto& data = doc.front();
	BOOST_TEST(data.parent() == &doc);
	BOOST_TEST(data.name() == "data");
	BOOST_TEST(data.get_ns() == "http://www.hekkelman.com/libzeep");

	BOOST_TEST(data.empty() == false);
	BOOST_TEST(data.begin() != data.end());

	auto& x = data.front();
	BOOST_TEST(x.name() == "x");
	BOOST_TEST(x.get_qname() == "x");
	BOOST_TEST(x.get_ns() == "http://www.hekkelman.com/libzeep");
	BOOST_TEST(x.parent() == &data);

	auto ax = x.attributes().find("a");
	BOOST_TEST(ax != x.attributes().end());
	BOOST_TEST(ax->value() == "1");
	BOOST_TEST(ax->get_ns() == "http://www.hekkelman.com/libzeep");

	auto& y = x.front();
	BOOST_TEST(y.parent() == &x);
	BOOST_TEST(y.name() == "y");
	BOOST_TEST(y.get_qname() == "y");
	BOOST_TEST(y.get_ns() == "http://www.hekkelman.com/libzeep");

	auto ay = y.attributes().find("a:a");
	BOOST_TEST(ay != y.attributes().end());
	BOOST_TEST(ay->value() == "2");
	BOOST_TEST(ay->get_ns() == "http://a.com/");
}

BOOST_AUTO_TEST_CASE(security_test_1)
{
	using namespace zx::literals;

    zx::element n("test");
	n.set_attribute("a", "a\xf6\"b");
	std::stringstream ss;
	BOOST_CHECK_THROW(ss << n, zeep::exception);
}

BOOST_AUTO_TEST_CASE(named_char_1)
{
	const zx::doctype::general_entity *c;

	c = zx::get_named_character("AElig");
	BOOST_TEST(c != nullptr);
	BOOST_TEST(c->get_replacement() == "Æ");

	c = zx::get_named_character("zwnj");
	BOOST_TEST(c != nullptr);
	BOOST_TEST(c->get_replacement() == "‌");

	c = zx::get_named_character("supseteq");
	BOOST_TEST(c != nullptr);
	BOOST_TEST(c->get_replacement() == "⊇");
}

BOOST_AUTO_TEST_CASE(named_char_2)
{
	using namespace zx::literals;

	auto a = R"(<!DOCTYPE html SYSTEM "about:legacy-compat" ><test xmlns:m="http://www.hekkelman.com">&supseteq;</test>)"_xml;

	auto b = R"(<test xmlns:m="http://www.hekkelman.com">⊇</test>)"_xml;

	BOOST_TEST((a == b) == true);
	if (not (a == b))
		std::cout << std::setw(2) << a << '\n' << b << '\n';
}