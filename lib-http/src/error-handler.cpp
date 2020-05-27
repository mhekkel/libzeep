// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/security.hpp>
#include <zeep/http/error-handler.hpp>

namespace zeep::http
{

error_handler::error_handler(const std::string& error_template)
	: m_error_template(error_template)
{
}

error_handler::~error_handler()
{
}

bool error_handler::create_error_reply(const request& req, std::exception_ptr eptr, reply& reply)
{
	bool result = false;

	try
	{
		if (eptr)
			std::rethrow_exception(eptr);
	}
	catch (const status_type& s)
	{
		result = create_error_reply(req, s, get_status_description(s), reply);
	}
	catch (const unauthorized_exception& ex)
	{
		result = create_unauth_reply(req, ex.m_realm, reply);
	}
	catch (const std::exception& ex)
	{
		result = create_error_reply(req, internal_server_error, ex.what(), reply);
	}
	catch (...)
	{
		result = create_error_reply(req, internal_server_error, "unhandled exception", reply);
	}

	return result;
}

bool error_handler::create_unauth_reply(const request& req, const std::string& realm, reply& rep)
{
	return create_error_reply(req, unauthorized, "You don't have access to this page", rep);
}

bool error_handler::create_error_reply(const request& req, status_type status, reply& rep)
{
	return create_error_reply(req, status, get_status_description(status), rep);
}

bool error_handler::create_error_reply(const request& req, status_type status, const std::string& message, reply& rep)
{
	bool handled = false;

	if (not m_error_template.empty() and m_server->has_template_processor())
	{
		auto& template_processor = m_server->get_template_processor();

		scope scope(*get_server(), req);

		object error
		{
			{ "nr", static_cast<int>(status) },
			{ "head", get_status_text(status) },
			{ "description", get_status_description(status) },
			{ "message", message },
			{ "request",
				{
					{ "username", req.username },
					{ "method", to_string(req.method) },
					{ "uri", req.uri }
				}
			}
		};

		scope.put("error", error);

		try
		{
			template_processor.create_reply_from_template(m_error_template, scope, rep);
			handled = true;
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
			template_processor.process_tags(doc.child(), scope);
			rep.set_content(doc);

			handled = true;
		}
	}

	if (not handled)
	{
		rep = reply::stock_reply(status, message);
		handled = true;		
	}
	else
		rep.set_status(status);

	return true;
}

}