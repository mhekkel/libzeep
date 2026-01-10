//          Copyright Maarten L. Hekkelman 2026
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/http/scope.hpp"
#include "zeep/http/tag-processor.hpp"
#include "zeep/http/template-processor.hpp"

#include <catch2/catch_test_macros.hpp>

#include <zeem/document.hpp>
#include <zeem/node.hpp>

#include <filesystem>
#include <iostream>
#include <string>

using namespace zeem::literals;

TEST_CASE("test_22")
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

	zeep::http::tag_processor tp;
	zeep::http::rsrc_based_html_template_processor p;
	zeep::http::scope scope;

	scope.put("b", "b");

	tp.process_xml(doc.child(), scope, "", p);

	CHECK(doc == doc_test);

	if (doc != doc_test)
	{
		std::cerr << doc << '\n'
				  << doc_test << '\n';
	}
}