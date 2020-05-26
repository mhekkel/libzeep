// rsrc_webapp-test

#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <random>

#include <zeep/crypto.hpp>
#include <zeep/streambuf.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/template-processor.hpp>

using namespace std;

using json = zeep::json::element;
using namespace zeep::xml::literals;


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

	zeep::http::tag_processor_v2 tp;
	zeep::http::rsrc_based_html_template_processor p;
	zeep::http::request req;
    zeep::http::scope scope(req);

    scope.put("b", "b");

    tp.process_xml(doc.child(), scope, "", p);
 
	BOOST_TEST(doc == doc_test);

	if (doc != doc_test)
	{
		cerr << doc << endl
			 << doc_test << endl;
	}
}