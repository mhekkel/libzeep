// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/security.hpp>
#include <zeep/html/error-handler.hpp>
#include <zeep/html/el-processing.hpp>

namespace zeep::html
{

bool error_handler::create_error_reply(const http::request& req, http::status_type status, const std::string& message, http::reply& rep)
{
	scope scope(req);

	object error
	{
		{ "nr", static_cast<int>(status) },
		{ "head", get_status_text(status) },
		{ "description", get_status_description(status) },
		{ "message", message },
		{ "request",
			{
				// { "line", ba::starts_with(req.uri, "http://") ? (boost::format("%1% %2% HTTP%3%/%4%") % to_string(req.method) % req.uri % req.http_version_major % req.http_version_minor).str() : (boost::format("%1% http://%2%%3% HTTP%4%/%5%") % to_string(req.method) % req.get_header("Host") % req.uri % req.http_version_major % req.http_version_minor).str() },
				{ "username", req.username },
				{ "method", to_string(req.method) },
				{ "uri", req.uri }
			}
		}
	};

	scope.put("error", error);

	try
	{
		create_reply_from_template("error.html", scope, rep);
	}
	catch(const std::exception& e)
	{
		using namespace xml::literals;

		auto doc = R"(<!DOCTYPE html SYSTEM "about:legacy-compat">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:z="http://www.hekkelman.com/libzeep/m2" xml:lang="en" lang="en">
<head>
    <title>Error</title>
	<meta name="viewport" content="width=device-width, initial-scale=1" />
	<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<style>

body, html {
	margin: 0;
}

.head {
	display: block;
	height: 5em;
	line-height: 5em;
	background-color: #6f1919;
	color: white;
	font-size: normal;
	font-weight: bold;
}

.head span {
	display: inline-block;
	vertical-align: middle;
	line-height: normal;
}

.head .error-nr {
	font-size: xx-large;
	padding: 0 0.5em 0 0.5em;
}

.error-head-text {
	font-size: large;
}

	</style>
</head>
<body>
	<div class="head">
		<span z:if="${error.nr}" class="error-nr" z:text="${error.nr}"></span>
		<span class="error-head-text" z:text="${error.head}"></span>
	</div>
	<div id="main">
		<p class="error-main-text" z:text="${error.description}"></p>
		<p z:if="${error.message}">Additional information: <span z:text="${error.message}"/></p>
	</div>
	<div id="footer">
		<span z:if="${error.request.method}">Method: <em z:text="${error.request.method}"/></span>
		<span z:if="${error.request.uri}">URI: <e z:text="${error.request.uri}"/></span>
		<span z:if="${error.request.username}">Username: <em z:text="${error.request.username}"/></span>
	</div>
</body>
</html>
)"_xml;
		process_tags(doc.child(), scope);
		rep.set_content(doc);
	}

	rep.set_status(status);
}

}