#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>


#include <zeep/http/tag-processor-v2.hpp>

using namespace std;

zeep::xml::document operator""_xml(const char* text, size_t length)
{
    zeep::xml::document doc;
	doc.set_preserve_cdata(true);
    doc.read(string(text, length));
    return doc;
}

class dummy_template_loader : public zeep::http::template_loader
{
	void load_template(const std::string& file, zeep::xml::document& doc)
		{}
} dummy_loader;

BOOST_AUTO_TEST_CASE(test_1)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test m:if="${true}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test />
</data>
    )"_xml;

    zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    tp.process_xml(doc.child(), scope, "");

	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_2)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test m:if="${'d' not in b}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test />
</data>
    )"_xml;

    zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("b", zeep::el::element{ "a", "b", "c"});
 
    tp.process_xml(doc.child(), scope, "");
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_3)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test m:text="${x}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test>&lt;hallo, wereld!&gt;</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "<hallo, wereld!>");
 
    tp.process_xml(doc.child(), scope, "");
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_3a)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test m:utext="${x}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <test><b>hallo, wereld!</b></test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "<b>hallo, wereld!</b>");
 
    tp.process_xml(doc.child(), scope, "");
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_4)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    [[${x}]]
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    hallo, wereld!
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "hallo, wereld!");
 
    tp.process_xml(doc.child(), scope, "");
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_5)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    [(${x})]
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <b>hallo, wereld!</b>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "<b>hallo, wereld!</b>");
 
    tp.process_xml(doc.child(), scope, "");
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_6)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <script m:inline="javascript">
	<![CDATA[
		const x = /* [[${x}]] */ null;
	]]>
	</script>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <script>
	<![CDATA[
		const x = "<b>hallo, wereld!</b>";
	]]>
	</script>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "\"<b>hallo, wereld!</b>\"");
 
    tp.process_xml(doc.child(), scope, "");
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_7)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <script m:inline="javascript">
//<![CDATA[
		const a = /* [[${a}]] */ null;
//]]>
	</script>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/ml-2">
    <script>
//<![CDATA[
		const x = ["a","b","c"];
//]]>
	</script>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp(dummy_loader, "http://www.hekkelman.com/libzeep/ml-2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("a", zeep::el::element{ "a", "b", "c"});

    tp.process_xml(doc.child(), scope, "");
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}
