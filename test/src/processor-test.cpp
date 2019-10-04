#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/http/webapp.hpp>

using namespace std;

using json = zeep::el::element;

zeep::xml::document operator""_xml(const char* text, size_t length)
{
    zeep::xml::document doc;
	doc.set_preserve_cdata(true);
    doc.read(string(text, length));
    return doc;
}

zeep::http::basic_webapp& dummy_webapp = *(new zeep::http::webapp());

BOOST_AUTO_TEST_CASE(test_1)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test1 m:if="${true}"/><test2 m:unless="${true}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test1 />
</data>
    )"_xml;

    zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    tp.process_xml(doc.child(), scope, "", dummy_webapp);

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
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test m:if="${'d' not in b}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test />
</data>
    )"_xml;

    zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("b", zeep::el::element{ "a", "b", "c"});
 
    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
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
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test m:text="${x}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test>&lt;hallo, wereld!&gt;</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "<hallo, wereld!>");
 
    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
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
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test m:utext="${x}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test><b>hallo, wereld!</b></test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "<b>hallo, wereld!</b>");
 
    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
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
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    [[${x}]]
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    hallo, wereld!
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "hallo, wereld!");
 
    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
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
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    [(${x})]
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <b>hallo, wereld!</b>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "<b>hallo, wereld!</b>");
 
    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
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
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<script m:inline="javascript">
<![CDATA[
	const x = /*[[${x}]]*/ null;
	var y = [[${y}]];
]]>
</script>
<script m:inline="none">
	const x = /*[[${x}]]*/ null;
	var y = [[${y}]];
</script>
<script>
	const x = /*[[${x}]]*/ null;
	var y = [[${y}]];
</script>

<script m:inline="javascript">
	const a = /*[[${a}]]*/ null
	const b = 1;
</script>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<script>
<![CDATA[
	const x = "\"<b>'hallo, wereld!'<\/b>\"";
	var y = "Een \"moeilijke\" string";
]]>
</script>
<script>
	const x = /*[[${x}]]*/ null;
	var y = [[${y}]];
</script>
<script>
	const x = /*&quot;&lt;b&gt;&#39;hallo, wereld!&#39;&lt;/b&gt;&quot;*/ null;
	var y = Een &quot;moeilijke&quot; string;
</script>

<script>
	const a = ["a","b","c"]
	const b = 1;
</script>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    scope.put("x", "\"<b>'hallo, wereld!'</b>\"");
    scope.put("y", "Een \"moeilijke\" string");
    scope.put("a", zeep::el::element{ "a", "b", "c"});
 
    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_8)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:each="b: ${a}" m:text="${b}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test>a</test><test>b</test><test>c</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("a", zeep::el::element{ "a", "b", "c"});

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_8a)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:text="${b}" m:each="b: ${a}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test>a</test><test>b</test><test>c</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("a", zeep::el::element{ "a", "b", "c"});

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_9)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:each="b, i: ${a}" m:text="${i}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test>{&quot;count&quot;:1,&quot;current&quot;:&quot;a&quot;,&quot;even&quot;:false,&quot;first&quot;:true,&quot;index&quot;:0,&quot;last&quot;:false,&quot;odd&quot;:true,&quot;size&quot;:3}</test><test>{&quot;count&quot;:2,&quot;current&quot;:&quot;b&quot;,&quot;even&quot;:true,&quot;first&quot;:false,&quot;index&quot;:1,&quot;last&quot;:false,&quot;odd&quot;:false,&quot;size&quot;:3}</test><test>{&quot;count&quot;:3,&quot;current&quot;:&quot;c&quot;,&quot;even&quot;:false,&quot;first&quot;:false,&quot;index&quot;:2,&quot;last&quot;:true,&quot;odd&quot;:true,&quot;size&quot;:3}</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("a", zeep::el::element{ "a", "b", "c"});

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_10)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:attr="data-id=${id}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test data-id="my-id-101" />
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("id", "my-id-101");

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_11)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:attr="data-id1=${id}, data-id2=${id}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test data-id1="my-id-101" data-id2="my-id-101" />
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("id", "my-id-101");

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_12)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:class="${ok}? 'ok'" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test class="ok" />
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("ok", true);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_13)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:class="${not ok} ?: 'ok'" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test class="ok" />
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
     scope.put("ok", true);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_14)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:text="${#dates.format('2019-08-07 12:14', '%e %B %Y, %H:%M')}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test> 7 augustus 2019, 12:14</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "nl, en-US;q=0.7, en;q=0.3" });

    zeep::el::scope scope(req);
     scope.put("ok", true);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_15)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:text="${#dates.format('2019-08-07 12:14', '%e %B %Y, %H:%M')}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test> 7 August 2019, 12:14</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "da, en-US;q=0.7, en;q=0.3" });

    zeep::el::scope scope(req);
     scope.put("ok", true);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_16)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:text="${#numbers.formatDecimal(12345.6789, 1, 2)}" />
<test m:text="${#numbers.formatDiskSize(12345, 2)}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test>12,345.68</test>
<test>12,06 K</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "en-GB, en-US;q=0.7, en;q=0.3" });

    zeep::el::scope scope(req);
     scope.put("ok", true);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_17)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:text="${#numbers.formatDecimal(12345.6789, 1, 2)}" />
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test>12 345,68</test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "fr_FR, en-US;q=0.7, en;q=0.3" });

    zeep::el::scope scope(req);
     scope.put("ok", true);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_18)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:object="${p}"><test2 m:text="*{n}" /></test>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test><test2>x</test2></test>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;
	zeep::http::request req;
    zeep::el::scope scope(req);

    json p;
    p["n"] = "x";

    scope.put("p", p);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_19)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<input type="checkbox" m:checked="${b}"/>
<input type="checkbox" m:checked="${c}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<input type="checkbox" checked="checked"/>
<input type="checkbox"/>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;
	zeep::http::request req;
    zeep::el::scope scope(req);

    scope.put("b", true);
    scope.put("c", false);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_20)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<div m:switch="${a}">
<div m:case="1">1</div>
<div m:case="2">2</div>
<div m:case="3">3</div>
<div m:case="*">*</div>
</div>

<div m:switch="${a}">
<a><div m:case="1">1</div></a>
<div m:case="2">2</div>
<div m:case="3">3</div>
<div m:case="*">*</div>
</div>

<div m:switch="${a}">
<div m:case="1">1<div m:case="2">2</div></div>
<div m:case="2">2</div>
<div m:case="3">3</div>
<div m:case="*">*</div>
</div>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<div>

<div>2</div>


</div>

<div>
<a/>
<div>2</div>


</div>

<div>

<div>2</div>


</div>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;
	zeep::http::request req;
    zeep::el::scope scope(req);

    scope.put("a", 2);

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_21)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<a m:with="a=${b}" m:text="${a}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<a>b</a>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;
	zeep::http::request req;
    zeep::el::scope scope(req);

    scope.put("b", "b");

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_22)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<div id="frag1">hello world</div>
<span m:insert=" :: #frag1"></span>
<span m:replace=" :: #frag1"></span>
<span m:include=" :: #frag1"></span>
<span m:insert="this :: #frag1"></span>
<span m:replace="this :: #frag1"></span>
<span m:include="this :: #frag1"></span>
<span m:insert="fragment-file :: frag1"></span>
<span m:replace="fragment-file :: frag1"></span>
<span m:include="fragment-file :: frag1"></span>
<span m:insert="fragment-file :: #frag2"></span>
<span m:replace="fragment-file :: #frag2"></span>
<span m:include="fragment-file :: #frag2"></span>
</data>
    )"_xml;

    auto doc_test = R"(<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<div id="frag1">hello world</div>
<span><div>hello world</div></span>
<div>hello world</div>
<span>hello world</span>
<span><div>hello world</div></span>
<div>hello world</div>
<span>hello world</span>
<span><div>fragment-1</div></span>
<div>fragment-1</div>
<span>fragment-1</span>
<span><div>fragment-2</div></span>
<div>fragment-2</div>
<span>fragment-2</span>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;
	zeep::http::request req;
    zeep::el::scope scope(req);

    scope.put("b", "b");

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_23)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span m:text="@{link}"/>
<span m:text="@{link(b=${b})}"/>
<span m:text="@{link/{b}(b=${b})}"/>
<span m:text="@{link(b=${b},test='test&amp;')}"/>
<span m:text="@{link/{b}{b}(b=${b})}"/>
<span m:text="@{link(c=${c})}"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span>link</span>
<span>link?b=b</span>
<span>link/b</span>
<span>link?b=b&amp;test=test%26</span>
<span>link/bb</span>
<span>link?c=bla%20met%20%3c%20en%20%3d</span>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;
	zeep::http::request req;
    zeep::el::scope scope(req);

    scope.put("b", "b");
	scope.put("c", "bla met < en =");

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}

BOOST_AUTO_TEST_CASE(test_24)
{
    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span m:text="||"/>
<span m:text="|een twee drie|"/>
<span m:text="|een ${b} en ${c}|"/>
<span m:text="'een ' + |twee ${b}|"/>
</data>
    )"_xml;

    auto doc_test = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span></span>
<span>een twee drie</span>
<span>een b en bla met &lt; en =</span>
<span>een twee b</span>
</data>
    )"_xml;

	zeep::http::tag_processor_v2 tp;
	zeep::http::request req;
    zeep::el::scope scope(req);

    scope.put("b", "b");
	scope.put("c", "bla met < en =");

    tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	if (doc != doc_test)
	{
		ostringstream s1;
		s1 << doc;
		ostringstream s2;
		s2 << doc_test;

		BOOST_TEST(s1.str() == s2.str());
	}
}