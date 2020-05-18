#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/streambuf.hpp>
#include <zeep/http/webapp.hpp>
#include <zeep/http/el-processing.hpp>

using namespace std;

using json = zeep::json::element;
using namespace zeep::xml::literals;

zeep::http::basic_html_controller& dummy_webapp = *(new zeep::http::file_based_webapp());

void process_and_compare(zeep::xml::document& a, zeep::xml::document& b, const zeep::http::scope& scope = {})
{
	zeep::http::tag_processor_v2 tp;
	tp.process_xml(a.child(), scope, "", dummy_webapp);
 
	BOOST_TEST(a == b);
	if (a != b)
	{
		cerr << string(80, '-') << endl
			 << a << endl
			 << string(80, '-') << endl
			 << b << endl
			 << string(80, '-') << endl;
	}
}

BOOST_AUTO_TEST_CASE(test_1)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<div>
<test1 m:if="${true}"/><test2 m:unless="${true}"/>
</div>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<div>
<test1 />
</div>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_2)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
	<test m:if="${'d' not in b}"/>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
	<test />
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("b", zeep::json::element{ "a", "b", "c"});
 
	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_3)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
	<test m:text="${x}"/>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
	<test>&lt;hallo, wereld!&gt;</test>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("x", "<hallo, wereld!>");
 
	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_3a)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
	<test m:utext="${x}"/>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
	<test><b>hallo, wereld!</b></test>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("x", "<b>hallo, wereld!</b>");
 
	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_4)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
	[[${x}]]
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
	hallo, wereld!
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("x", "hallo, wereld!");
 
	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_5)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
	[(${x})]
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
	<b>hallo, wereld!</b>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("x", "<b>hallo, wereld!</b>");
 
	process_and_compare(doc, doc_test, scope);
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
<data>
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

	zeep::http::scope scope;
	scope.put("x", "\"<b>'hallo, wereld!'</b>\"");
	scope.put("y", "Een \"moeilijke\" string");
	scope.put("a", zeep::json::element{ "a", "b", "c"});
 
	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_8)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:each="b: ${a}" m:text="${b}" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test>a</test><test>b</test><test>c</test>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("a", zeep::json::element{ "a", "b", "c"});

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_8a)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:text="${b}" m:each="b: ${a}" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test>a</test><test>b</test><test>c</test>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("a", zeep::json::element{ "a", "b", "c"});

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_9)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:each="b, i: ${a}" m:text="${i}" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test>{&quot;count&quot;:1,&quot;current&quot;:&quot;a&quot;,&quot;even&quot;:false,&quot;first&quot;:true,&quot;index&quot;:0,&quot;last&quot;:false,&quot;odd&quot;:true,&quot;size&quot;:3}</test><test>{&quot;count&quot;:2,&quot;current&quot;:&quot;b&quot;,&quot;even&quot;:true,&quot;first&quot;:false,&quot;index&quot;:1,&quot;last&quot;:false,&quot;odd&quot;:false,&quot;size&quot;:3}</test><test>{&quot;count&quot;:3,&quot;current&quot;:&quot;c&quot;,&quot;even&quot;:false,&quot;first&quot;:false,&quot;index&quot;:2,&quot;last&quot;:true,&quot;odd&quot;:true,&quot;size&quot;:3}</test>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("a", zeep::json::element{ "a", "b", "c"});

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_10)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:attr="data-id=${id}" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test data-id="my-id-101" />
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("id", "my-id-101");

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_11)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:attr="data-id1=${id}, data-id2=${id}" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test data-id1="my-id-101" data-id2="my-id-101" />
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("id", "my-id-101");

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_12)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:class="${ok}? 'ok'" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test class="ok" />
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("ok", true);

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_13)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:class="${not ok} ?: 'ok'" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test class="ok" />
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("ok", true);

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_14)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<test m:text="${#dates.format('2019-08-07 12:14', '%e %B %Y, %H:%M')}" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<test> 7 augustus 2019, 12:14</test>
</data>
	)"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "nl, en-US;q=0.7, en;q=0.3" });

	zeep::http::scope scope(req);
	scope.put("ok", true);

	tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	BOOST_TEST_WARN(doc == doc_test);
	if (doc != doc_test)
	{
		cerr << doc << endl
			 << doc_test << endl;
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
<data>
<test> 7 august 2019, 12:14</test>
</data>
	)"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "da, en-US;q=0.7, en;q=0.3" });

	zeep::http::scope scope(req);
	scope.put("ok", true);

	tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	BOOST_TEST_WARN(doc == doc_test);
	if (doc != doc_test)
	{
		cerr << doc << endl
			 << doc_test << endl;
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
<data>
<test>12,345.68</test>
<test>12.06 K</test>
</data>
	)"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "en-GB, en-US;q=0.7, en;q=0.3" });

	zeep::http::scope scope(req);
	scope.put("ok", true);

	tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	BOOST_TEST(doc == doc_test);
	if (doc != doc_test)
	{
		cerr << doc << endl
			 << doc_test << endl;
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
<data>
<test>12.345,68</test>
</data>
	)"_xml;

	zeep::http::tag_processor_v2 tp;

	zeep::http::request req;
	req.headers.push_back({ "Accept-Language", "fr_FR, en-US;q=0.7, en;q=0.3" });

	zeep::http::scope scope(req);
	scope.put("ok", true);

	tp.process_xml(doc.child(), scope, "", dummy_webapp);
 
	BOOST_TEST_WARN(doc == doc_test);
	if (doc != doc_test)
	{
		cerr << doc << endl
			 << doc_test << endl;
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
<data>
<test><test2>x</test2></test>
</data>
	)"_xml;

	zeep::http::scope scope;

	json p;
	p["n"] = "x";

	scope.put("p", p);

	process_and_compare(doc, doc_test, scope);
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
<data>
<input type="checkbox" checked="checked"/>
<input type="checkbox"/>
</data>
	)"_xml;

	zeep::http::scope scope;

	scope.put("b", true);
	scope.put("c", false);

	process_and_compare(doc, doc_test, scope);
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
<data>
<div><div>2</div></div>

<div><div>2</div></div>

<div><div>2</div></div>
</data>
	)"_xml;

	zeep::http::scope scope;

	scope.put("a", 2);

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_21)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<a m:with="a=${b}" m:text="${a}"/>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<a>b</a>
</data>
	)"_xml;

	zeep::http::scope scope;

	scope.put("b", "b");

	process_and_compare(doc, doc_test, scope);
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

	auto doc_test = R"(<data>
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

	zeep::http::scope scope;
	scope.put("b", "b");

	process_and_compare(doc, doc_test, scope);
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
<data>
<span>link</span>
<span>link?b=b</span>
<span>link/b</span>
<span>link?b=b&amp;test=test%26</span>
<span>link/bb</span>
<span>link?c=bla%20met%20%3c%20en%20%3d</span>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("b", "b");
	scope.put("c", "bla met < en =");

	process_and_compare(doc, doc_test, scope);
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
<data>
<span></span>
<span>een twee drie</span>
<span>een b en bla met &lt; en =</span>
<span>een twee b</span>
</data>
	)"_xml;

	zeep::http::scope scope;
	scope.put("b", "b");
	scope.put("c", "bla met < en =");

	process_and_compare(doc, doc_test, scope);
}

BOOST_AUTO_TEST_CASE(test_25)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span m:each="x: ${ { 'aap', 'noot', 'mies' } }" m:text="${x}"/>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<span>aap</span>
<span>noot</span>
<span>mies</span>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_26)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span a="none" m:remove="none"><x/><y/></span>
<span a="all" m:remove="all"><x/><y/></span>
<span a="body" m:remove="body"><x/><y/></span>
<span a="all-but-first" m:remove="all-but-first"><x/><y/></span>
<span a="tag" m:remove="tag"><x><y/></x><z/></span>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<span a="none"><x/><y/></span>

<span a="body"></span>
<span a="all-but-first"><x/></span>
<x><y/></x><z/>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_27)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span m:assert="1==0" />
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
Error processing element 'span': Assertion failed for '1==0'<span/>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_28)
{
	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<span m:ref="1"/>
<m:block>in een blok<em>met een em</em></m:block>
</data>
	)"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<span/>
in een blok<em>met een em</em>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_29)
{
	auto doc = R"xml(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<m:block m:remove="all">
<span m:fragment="fr1">fragment</span>
<span m:ref="fr1">ref</span>
</m:block>
<div m:replace="~{::fr1}"/>
<div m:replace="~{::fr1/text()}"/>
</data>
	)xml"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>

<span>fragment</span><span>ref</span>
fragmentref
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_30)
{
	auto doc = R"xml(<?xml version="1.0"?>
<data xmlns:th="http://www.hekkelman.com/libzeep/m2">
<span th:remove="all" th:ref="R_1">ref-1</span>
<th:block th:remove="all">
	<div th:fragment="F_1(arg)"><span th:replace="${arg}"/></div>
	<div th:ref="thediv">The div</div>
</th:block>
<div th:replace="~{::F_1(~{::thediv})}"/>
</data>
	)xml"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>

<div><div>The div</div></div>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_31)
{
	auto doc = R"xml(<?xml version="1.0"?>
<data xmlns:th="http://www.hekkelman.com/libzeep/m2">
<div th:replace="~{fragment-file::frag3(~{::title})}">
	<title>De titel is vervangen</title>
</div>
<div th:replace="~{fragment-file::frag3(~{})}"/>
</data>
	)xml"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<nav>
		<title>De titel is vervangen</title>
	</nav>
<nav>
		<title>Niet vervangen</title>
	</nav>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_32)
{
	auto doc = R"xml(<?xml version="1.0"?>
<data xmlns:th="http://www.hekkelman.com/libzeep/m2">
<span th:remove="all" th:ref="R_1">ref-1</span>
<th:block th:remove="all"><div th:fragment="F_1(arg)"><span th:text="${arg}"/></div></th:block>
<div th:replace="~{::R_1}"/>
<div th:replace="~{::F_1(~{::R_1/text()})}"/>
</data>
	)xml"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>




<div><span>ref-1</span></div>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}

BOOST_AUTO_TEST_CASE(test_33)
{
	auto doc = R"xml(<?xml version="1.0"?>
<data xmlns:z="http://www.hekkelman.com/libzeep/m2">
<div class="een" z:classappend="${true} ? 'twee'"/>
<div style="width: 30" z:styleappend="height: 30"/>
</data>
	)xml"_xml;

	auto doc_test = R"(<?xml version="1.0"?>
<data>
<div class="een twee"/>
<div style="width: 30; height: 30;"/>
</data>
	)"_xml;

	process_and_compare(doc, doc_test);
}
