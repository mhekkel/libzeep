// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

#include <zeep/html/login-controller.hpp>

namespace zeep::html
{

login_controller::login_controller(const std::string& prefix_path = "/")
	: html::controller(prefix_path)
{
	mount_get("login", &login_controller::handle_get_login);
	mount_post("login", &login_controller::handle_post_login);
	mount("logout", &login_controller::handle_logout);
}

void login_controller::create_unauth_reply(const request& req, const std::string& realm, reply& rep)
{
	xml::document doc;
	doc.set_preserve_cdata(true);

	try
	{
		load_template("login", doc);
	}
	catch(const std::exception& e)
	{
		using namespace xml::literals;

		doc = R"(<!DOCTYPE html SYSTEM "about:legacy-compat">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:z2="http://www.hekkelman.com/libzeep/m2" xml:lang="en" lang="en">
  <head>
    <meta charset="utf-8"/>
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no"/>
    <meta name="description" content=""/>
    <meta name="author" content=""/>
    <title>Please sign in</title>
    <link href="https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-beta/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-/Y6pD6FV/Vv2HJnA6t+vslU6fwYXjCFtcEpHbNJ0lyAFsXTsjBbfaDjzALeQsN6M" crossorigin="anonymous"/>
    <link href="https://getbootstrap.com/docs/4.0/examples/signin/signin.css" rel="stylesheet" crossorigin="anonymous"/>
  </head>
  <body>
     <div class="container">
      <form class="form-signin" method="post" action="/login">
		<input type="hidden" name="uri" z2:value="${uri ?: '/'}"/>
        <h2 class="form-signin-heading">Please sign in</h2>
        <div class="mt-2 mb-2">
          <label for="username" class="sr-only">Username</label>
          <input type="text" id="username" name="username" class="form-control" placeholder="Username" required="required" autofocus="autofocus"
		  	z2:value="${username}"/>
        </div>
        <div class="mt-2 mb-2">
          <label for="password" class="sr-only">Password</label>
          <input type="password" id="password" name="password" class="form-control" placeholder="Password" required="required"
		  	value="" z2:classappend="${invalid}?'is-invalid'"/>
		  <div class="invalid-feedback" >
            Invalid username/password
          </div>
        </div>
        <button class="btn btn-lg btn-primary btn-block" type="submit">Sign in</button>
      </form>
</div>
</body></html>)"_xml;
	}

	scope scope(req);

	auto uri = req.get_parameter("uri");
	if (uri.empty())
		uri = req.uri;
	scope.put("uri", uri);

	auto username = req.get_parameter("username");
	if (username.empty())
		username = req.username;
	scope.put("username", username);

	if (req.get_parameter("password").empty() == false)
		scope.put("invalid", true);

	process_tags(doc.child(), scope);
	rep.set_content(doc);
	rep.set_status(unauthorized);

	for (auto av: m_authentication_validators)
	{
		if (av->get_realm() != realm)
			continue;
		
		av->add_challenge_headers(rep);
	}
}

void login_controller::handle_get_login(const request& request, const scope& scope, reply& reply)
{
	create_unauth_reply(request, false, "", reply);
	reply.set_status(ok);
}

void login_controller::handle_post_login(const request& request, const scope& scope, reply& reply)
{
	reply = reply::stock_reply(unauthorized);

	for (;;)
	{
		auto csrfToken = request.get_cookie("csrf-token");
		if (csrfToken.empty())
			break;
		
		if (request.get_parameter("_csrf") != csrfToken)
			break;
		
		auto username = request.get_parameter("username");
		auto password = request.get_parameter("password");

		for (auto av: m_authentication_validators)
		{
			auto credentials = av->validate_username_password(username, password);
			if (credentials)
			{
				auto uri = request.get_parameter("uri");
				if (uri.empty())
					reply = reply::redirect("/");
				else
					reply = reply::redirect(uri);

				credentials["iat"] = std::to_string(std::time(nullptr));
				credentials["sub"] = av->get_realm();

				av->add_authorization_headers(reply, credentials);
				break;
			}
		}
		break;
	}

	if (reply.get_status() == unauthorized)
		create_unauth_reply(request, false, "", reply);
}

void login_controller::handle_logout(const request& request, const scope& scope, reply& reply)
{
	reply = reply::redirect("/");
	reply.set_cookie("access_token", "", {
		{ "Max-Age", "0" }
	});
}

} // namespace zeep
