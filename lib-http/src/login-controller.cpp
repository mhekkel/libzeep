// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

#include <zeep/http/login-controller.hpp>
#include <zeep/http/security.hpp>

namespace zeep::http
{

namespace
{

using namespace xml::literals;

auto kLoginDoc = R"(<!DOCTYPE html SYSTEM "about:legacy-compat">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
	<head>
		<meta charset="utf-8" />
		<meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no" />
		<meta name="description" content="" />
		<meta name="author" content="" />
		<title>Please sign in</title>
		<link href="https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-beta/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-/Y6pD6FV/Vv2HJnA6t+vslU6fwYXjCFtcEpHbNJ0lyAFsXTsjBbfaDjzALeQsN6M" crossorigin="anonymous" />
		<link href="https://getbootstrap.com/docs/4.0/examples/signin/signin.css" rel="stylesheet" crossorigin="anonymous" />
	</head>
	<body>
		<div class="container">
			<form class="form-signin" method="post" action="/login">
				<input type="hidden" name="uri" />
				<input type="hidden" name="_csrf" />
				<h2 class="form-signin-heading">Please sign in</h2>
				<div class="mt-2 mb-2">
					<label for="username" class="sr-only">Username</label>
					<input type="text" id="username" name="username" class="form-control" placeholder="Username" required="required" autofocus="autofocus" />
				</div>
				<div class="mt-2 mb-2">
					<label for="password" class="sr-only">Password</label>
					<input type="password" id="password" name="password" class="form-control" placeholder="Password" required="required" value="" />
					<div class="invalid-feedback">
            Invalid username/password
		  </div>
				</div>
				<button class="btn btn-lg btn-primary btn-block" type="submit">Sign in</button>
			</form>
		</div>
	</body>
</html>)"_xml;


}

login_controller::login_controller(const std::string& prefix_path)
	: http::controller(prefix_path)
{
}

void login_controller::create_unauth_reply(const request& req, const std::string& realm, reply& rep)
{
	rep.set_content(kLoginDoc);
	rep.set_status(unauthorized);
}

bool login_controller::handle_request(request& req, reply& rep)
{
	bool result = false;

	if (req.get_path() == "/login")
	{
		result = true;

		if (req.method == method_type::GET)
		{
			auto doc = kLoginDoc;
			auto csrf = doc.find_first("//input[@name='_csrf']");
			if (not csrf)
				throw internal_server_error;
			csrf->set_attribute("value", req.get_cookie("csrf-token"));
			rep.set_content(doc);
		}
		else if (req.method == method_type::POST)
		{
			auto csrf = req.get_parameter("_csrf");
			if (csrf != req.get_cookie("csrf-token"))
				throw status_type::forbidden;

			auto uri = req.get_parameter("uri");
			rep = reply::redirect(uri);

			auto username = req.get_parameter("username");
			auto password = req.get_parameter("password");
			get_server().get_security_context().verify_username_password(username, password, rep);
		}
		else
			result = false;
	}
	else if (req.get_path() == "/logout")
	{
		auto uri = req.get_parameter("uri");
		rep = reply::redirect(uri);

		rep.set_cookie("access_token", "", {
			{ "Max-Age", "0" }
		});

		result = true;
	}

	return result;
}

}
