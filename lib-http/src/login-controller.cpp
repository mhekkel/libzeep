//        Copyright Maarten L. Hekkelman, 2014-2022
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <zeep/http/error-handler.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/uri.hpp>

namespace fs = std::filesystem;

namespace zeep::http
{

class login_error_handler : public error_handler
{
  public:
	login_error_handler(login_controller *c)
		: m_login_controller(c)
	{
	}

	bool create_unauth_reply(const request &req, reply &reply) override
	{
		m_login_controller->create_unauth_reply(req, reply);
		return true;
	}

	bool create_error_reply(const request &req, std::exception_ptr eptr, reply &reply) override
	{
		bool result = false;

		try
		{
			if (eptr)
				std::rethrow_exception(eptr);
		}
		catch (const unauthorized_exception &)
		{
			result = create_unauth_reply(req, reply);
		}
		catch (...)
		{
			result = false;
		}

		return result;
	}

  private:
	login_controller *m_login_controller;
};

login_controller::login_controller(const std::string &prefix_path)
	: html_controller(prefix_path)
{
	map_get("login", &login_controller::handle_get_login);
	map_post("login", &login_controller::handle_post_login, "username", "password");

	map_get("logout", &login_controller::handle_logout);
	map_post("logout", &login_controller::handle_logout);
}

void login_controller::set_server(basic_server *server)
{
	controller::set_server(server);

	assert(server->has_security_context());
	if (not server->has_security_context())
		throw std::runtime_error("The HTTP server has no security context");

	auto &sc = server->get_security_context();
	sc.add_rule("/login", {});

	server->add_error_handler(new login_error_handler(this));
}

xml::document login_controller::load_login_form(const request &req) const
{
	if (m_server->has_template_processor())
	{
		try
		{
			auto &tp = m_server->get_template_processor();

			xml::document doc;
			doc.set_preserve_cdata(true);

			tp.load_template("login", doc);

			tp.process_tags(doc.child(), { get_server(), req });

			return doc;
		}
		catch (const std::exception &ex)
		{
			std::cerr << ex.what() << std::endl;
		}
	}

	using namespace xml::literals;

	return R"(<!DOCTYPE html SYSTEM "about:legacy-compat">
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

void login_controller::create_unauth_reply(const request &req, reply &reply)
{
	auto doc = load_login_form(req);

	auto csrf_cookie = req.get_cookie("csrf-token");
	if (csrf_cookie.empty())
	{
		csrf_cookie = encode_base64url(random_hash());
		reply.set_cookie("csrf-token", csrf_cookie, { { "HttpOnly", "" }, { "SameSite", "Lax" }, { "Path", "/" } });
	}

	for (auto csrf : doc.find("//input[@name='_csrf']"))
		csrf->set_attribute("value", csrf_cookie);

	for (auto uri : doc.find("//input[@name='uri']"))
		uri->set_attribute("value", req.get_uri().string());

	reply.set_content(doc);
	reply.set_status(status_type::unauthorized);
}

reply login_controller::handle_get_login(const scope &scope)
{
	auto &req = scope.get_request();
	auto doc = load_login_form(req);

	reply rep = reply::stock_reply(ok);

	auto csrf_cookie = req.get_cookie("csrf-token");
	if (csrf_cookie.empty())
	{
		csrf_cookie = encode_base64url(random_hash());
		rep.set_cookie("csrf-token", csrf_cookie, { { "HttpOnly", "" }, { "SameSite", "Lax" }, { "Path", "/" } });
	}

	for (auto csrf : doc.find("//input[@name='_csrf']"))
		csrf->set_attribute("value", csrf_cookie);

	rep.set_content(doc);
	return rep;
}

reply login_controller::handle_post_login(const scope &scope, const std::string &username, const std::string &password)
{
	auto &req = scope.get_request();
	auto csrf = req.get_parameter("_csrf");
	if (csrf != req.get_cookie("csrf-token"))
		throw status_type::forbidden;

	uri uri(req.get_parameter("uri"));
	auto rep = create_redirect_for_request(req);

	try
	{
		get_server()->get_security_context().verify_username_password(username, password, rep);
	}
	catch (const invalid_password_exception &e)
	{
		auto doc = load_login_form(req);
		for (auto csrf_attr : doc.find("//input[@name='_csrf']"))
			csrf_attr->set_attribute("value", req.get_cookie("csrf-token"));

		auto user = doc.find_first("//input[@name='username']");
		user->set_attribute("value", username);

		auto pw = doc.find_first("//input[@name='password']");
		pw->set_attribute("class", pw->get_attribute("class") + " is-invalid");

		for (auto i_uri : doc.find("//input[@name='uri']"))
			i_uri->set_attribute("value", uri.string());

		rep.set_content(doc);

		std::cerr << e.what() << '\n';
	}

	return rep;
}

reply login_controller::handle_logout(const scope &scope)
{
	auto &req = scope.get_request();

	auto rep = create_redirect_for_request(req);
	rep.set_delete_cookie("access_token");

	return rep;
}

reply login_controller::create_redirect_for_request(const request &req)
{
	uri url;

	if (req.has_parameter("uri"))
	{
		auto s = req.get_parameter("uri");
		if (zeep::http::is_fully_qualified_uri(s))
			url = s;
		else if (s == "/")
			url = get_context_name();
		else
			url = uri(s, get_context_name());
	}
	else
	{
		url = get_context_name();
		if (url.get_path().empty())
			url.set_path("/");
	}
	return reply::redirect(url, see_other);
}

} // namespace zeep::http


