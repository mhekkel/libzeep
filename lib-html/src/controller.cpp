// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

#include <cstdlib>

#include <map>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/iostreams/copy.hpp>

#include <zeep/crypto.hpp>
#include <zeep/utils.hpp>
#include <zeep/xml/character-classification.hpp>
#include <zeep/xml/xpath.hpp>
#include <zeep/html/controller.hpp>
#include <zeep/html/el-processing.hpp>
#include <zeep/html/controller.hpp>

namespace ba = boost::algorithm;
namespace io = boost::iostreams;
namespace fs = std::filesystem;
namespace pt = boost::posix_time;

namespace zeep::html
{

// --------------------------------------------------------------------
//

/// return last_write_time of \a file
std::filesystem::file_time_type file_loader::file_time(const std::string& file, std::error_code& ec) noexcept
{
	return fs::last_write_time(m_docroot / file, ec);
}

/// return last_write_time of \a file
std::istream* file_loader::load_file(const std::string& file, std::error_code& ec) noexcept
{
	std::ifstream* result = new std::ifstream(m_docroot / file, std::ios::binary);
	if (not result->is_open())
	{
		delete result;
		result = nullptr;
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
	}
	
	return result;	
}

// --------------------------------------------------------------------
//

basic_html_controller::basic_html_controller()
{
}

basic_html_controller::~basic_html_controller()
{
	for (auto a: m_authentication_validators)
		delete a;
}

void basic_html_controller::add_authenticator(http::authentication_validation_base* authenticator, bool login)
{
	m_authentication_validators.push_back(authenticator);

	if (login)
	{
		mount_get("login", &basic_html_controller::handle_get_login);
		mount_post("login", &basic_html_controller::handle_post_login);
		mount("logout", &basic_html_controller::handle_logout);
	}
}

bool basic_html_controller::handle_request(http::request& req, http::reply& rep)
{
	std::string uri = req.uri;

	std::string csrf = req.get_cookie("csrf-token");
	bool csrf_is_new = csrf.empty();

	if (csrf_is_new)
	{
		csrf = encode_base64url(random_hash());
		req.set_cookie("csrf-token", csrf);
	}

	try
	{
		// start by sanitizing the request's URI, first parse the parameters
		std::string ps = req.payload;
		if (req.method != http::method_type::POST)
		{
			std::string::size_type d = uri.find('?');
			if (d != std::string::npos)
			{
				ps = uri.substr(d + 1);
				uri.erase(d, std::string::npos);
			}
		}

		// strip off the http part including hostname and such
		if (ba::starts_with(uri, "http://"))
		{
			std::string::size_type s = uri.find_first_of('/', 7);
			if (s != std::string::npos)
				uri.erase(0, s);
		}

		// now make the path relative to the root
		while (uri.length() > 0 and uri[0] == '/')
			uri.erase(uri.begin());

		// set up the scope by putting some globals in it
		scope scope(req);

		scope.put("uri", object(uri));
		auto s = uri.find('?');
		if (s != std::string::npos)
			uri.erase(s, std::string::npos);
		scope.put("baseuri", uri);

		init_scope(scope);

		auto handler = find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
			[uri, method=req.method](const mount_point& m)
			{
				// return m.path == uri and
				return glob_match(uri, m.path) and
					(	method == http::method_type::HEAD or
						method == http::method_type::OPTIONS or
						m.method == method or
						m.method == http::method_type::UNDEFINED);
			});

		if (handler == m_dispatch_table.end())
			throw http::not_found;

		if (req.method == http::method_type::OPTIONS)
		{
			rep = http::reply::stock_reply(http::ok);
			rep.set_header("Allow", "GET,HEAD,POST,OPTIONS");
			rep.set_content("", "text/plain");
		}
		else
		{
			// check for the presence of a access_token

			json::element credentials;

			// Do authentication here, if needed
			if (not handler->realm.empty())
			{
				auto avi = std::find_if(m_authentication_validators.begin(), m_authentication_validators.end(),
					[handler](auto av) { return av->get_realm() == handler->realm; });

				if (avi == m_authentication_validators.end())	// logic error
					throw std::logic_error("No authenticator provided for realm " + handler->realm);

				credentials = (*avi)->validate_authentication(req);

				if (not credentials)
					throw http::unauthorized_exception(handler->realm);
			}
			else	// not a protected area, but see if there's a valid login anyway
			{
				for (auto av: m_authentication_validators)
				{
					credentials = av->validate_authentication(req);
					if (credentials)
						break;
				}
			}

			if (credentials)
			{
				if (credentials.is_string())
					req.username = credentials.as<std::string>();
				else if (credentials.is_object())
					req.username = credentials["username"].as<std::string>();

				scope.put("credentials", credentials);
			}

			handler->handler(req, scope, rep);

			if (req.method == http::method_type::HEAD)
			{
				rep.set_content("", rep.get_content_type());
				csrf_is_new = false;
			}
		}
	}
	catch (http::authorization_stale_exception& e)
	{
		create_unauth_reply(req, true, e.m_realm, rep);
	}
	catch (http::unauthorized_exception& e)
	{
		create_unauth_reply(req, false, e.m_realm, rep);
	}
	catch (http::status_type& s)
	{
		create_error_reply(req, s, rep);
		csrf_is_new = false;
	}
	catch (std::exception& e)
	{
		create_error_reply(req, http::internal_server_error, e.what(), rep);
		csrf_is_new = false;
	}

	if (csrf_is_new)
		rep.set_cookie("csrf-token", csrf, {
			{ "HttpOnly", "" },
			{ "SameSite", "Lax" },
			{ "Path", "/" }
		});

	#warning("hier moet nog iets komen")
	return true;
}

void basic_html_controller::handle_get_login(const http::request& request, const scope& scope, http::reply& reply)
{
	create_unauth_reply(request, false, "", reply);
	reply.set_status(http::ok);
}

void basic_html_controller::handle_post_login(const http::request& request, const scope& scope, http::reply& reply)
{
	reply = http::reply::stock_reply(http::unauthorized);

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
					reply = http::reply::redirect("/");
				else
					reply = http::reply::redirect(uri);

				credentials["iat"] = std::to_string(std::time(nullptr));
				credentials["sub"] = av->get_realm();

				av->add_authorization_headers(reply, credentials);
				break;
			}
		}
		break;
	}

	if (reply.get_status() == http::unauthorized)
		create_unauth_reply(request, false, "", reply);
}

void basic_html_controller::handle_logout(const http::request& request, const scope& scope, http::reply& reply)
{
	reply = http::reply::redirect("/");
	reply.set_cookie("access_token", "", {
		{ "Max-Age", "0" }
	});
}

void basic_html_controller::create_unauth_reply(const http::request& req, bool stale, const std::string& realm, http::reply& rep)
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
	rep.set_status(http::unauthorized);

	for (auto av: m_authentication_validators)
	{
		if (av->get_realm() != realm)
			continue;
		
		av->add_challenge_headers(rep, stale);
	}
}

void basic_html_controller::create_error_reply(const http::request& req, http::status_type status, http::reply& rep)
{
	create_error_reply(req, status, "", rep);
}

void basic_html_controller::create_error_reply(const http::request& req, http::status_type status, const std::string& message, http::reply& rep)
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

void basic_html_controller::handle_file(const http::request& request, const scope& scope, http::reply& reply)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;

	std::error_code ec;
	auto ft = file_time(scope["baseuri"].as<std::string>(), ec);

	if (ec)
	{
		reply = http::reply::stock_reply(http::not_found);
		return;
	}

	auto lastWriteTime = decltype(ft)::clock::to_time_t(ft);

	std::string ifModifiedSince;
	for (const http::header& h : request.headers)
	{
		if (iequals(h.name, "If-Modified-Since"))
		{
			local_date_time modifiedSince(local_sec_clock::local_time(time_zone_ptr()));

			local_time_input_facet *lif1(new local_time_input_facet("%a, %d %b %Y %H:%M:%S GMT"));

			std::stringstream ss;
			ss.imbue(std::locale(std::locale::classic(), lif1));
			ss.str(h.value);
			ss >> modifiedSince;

			local_date_time fileDate(from_time_t(lastWriteTime), time_zone_ptr());

			if (fileDate <= modifiedSince)
			{
				reply = http::reply::stock_reply(http::not_modified);
				return;
			}

			break;
		}
	}

	fs::path file = scope["baseuri"].as<std::string>();

	std::unique_ptr<std::istream> in(load_file(file, ec));
	if (ec)
	{
		reply = http::reply::stock_reply(http::not_found);
		return;
	}

	std::stringstream out;

	io::copy(*in, out);

	std::string mimetype = "text/plain";

	if (file.extension() == ".css")
		mimetype = "text/css";
	else if (file.extension() == ".js")
		mimetype = "text/javascript";
	else if (file.extension() == ".png")
		mimetype = "image/png";
	else if (file.extension() == ".svg")
		mimetype = "image/svg+xml";
	else if (file.extension() == ".html" or file.extension() == ".htm")
		mimetype = "text/html";
	else if (file.extension() == ".xml" or file.extension() == ".xsl" or file.extension() == ".xslt")
		mimetype = "text/xml";
	else if (file.extension() == ".xhtml")
		mimetype = "application/xhtml+xml";

	reply.set_content(out.str(), mimetype);

	local_date_time t(local_sec_clock::local_time(time_zone_ptr()));
	local_time_facet *lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));

	std::stringstream s;
	s.imbue(std::locale(std::cout.getloc(), lf));

	ptime pt = from_time_t(lastWriteTime);
	local_date_time t2(pt, time_zone_ptr());
	s << t2;

	reply.set_header("Last-Modified", s.str());
}

void basic_html_controller::set_docroot(const fs::path& path)
{
	m_docroot = path;
}

void basic_html_controller::load_template(const std::string& file, xml::document& doc)
{
	std::string templateSelector, templateFile = file;

	auto spec = evaluate_el({}, templateFile);

	if (spec.is_object())	// reset the content, saves having to add another method
	{
		templateFile = spec["template"].as<std::string>();
		templateSelector = spec["selector"]["xpath"].as<std::string>();
	}

	std::unique_ptr<std::istream> data;

	for (const char* ext: { "", ".xhtml", ".html", ".xml" })
	{
		std::error_code ec;

		fs::path template_file = m_docroot / (templateFile + ext);

		(void)file_time(template_file, ec);
		if (ec)
			continue;

		data.reset(load_file(template_file.string(), ec));
		
		break;
	}

	if (not data)
	{
#if defined(_MSC_VER)
		char msg[1024] = "";

		DWORD dw = ::GetLastError();
		if (dw != NO_ERROR)
		{
			char *lpMsgBuf = nullptr;
			int m = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
										NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);

			if (lpMsgBuf != nullptr)
			{
				// strip off the trailing whitespace characters
				while (m > 0 and isspace(lpMsgBuf[m - 1]))
					--m;
				lpMsgBuf[m] = 0;

				strncpy(msg, lpMsgBuf, sizeof(msg));

				::LocalFree(lpMsgBuf);
			}
		}

		throw exception((boost::format("error opening: %1% (%2%)") % (m_docroot / file) % msg).str());
#else
		throw exception("error opening: " + m_docroot.string() + " (" + strerror(errno) + ")");
#endif
	}

	doc.set_preserve_cdata(true);
	*data >> doc;

	if (not templateSelector.empty())
	{
		// tricky? Find first matching fragment and make it the root node of the document

		xml::context ctx;

		// this is problematic, take the first processor namespace for now.
		// TODO fix this
		std::string ns;

		for (auto& tp: m_tag_processor_creators)
		{
			std::unique_ptr<tag_processor> ptp(tp.second(tp.first));
			if (dynamic_cast<tag_processor_v2*>(ptp.get()) == nullptr)
				continue;

			ns = tp.first;
			ctx.set("ns", ns);
			break;
		}
		xml::xpath xp(templateSelector);

		std::vector<std::unique_ptr<xml::node>> result;

		for (auto n: xp.evaluate<xml::node>(doc, ctx))
		{
			auto e = dynamic_cast<xml::element*>(n);
			if (e == nullptr)
				continue;

			xml::document dest;

			auto& attr = e->attributes();

			if (spec["selector"]["by-id"])
				attr.erase("id");

			attr.erase(e->prefix_tag("ref", ns));
			attr.erase(e->prefix_tag("fragment", ns));

			auto parent = e->parent();
			dest.push_back(std::move(*e));

			xml::fix_namespaces(dest.front(), *parent, dest.front());

			doc.swap(dest);
			break;
		}
	}
}

void basic_html_controller::create_reply_from_template(const std::string& file, const scope& scope, http::reply& reply)
{
	xml::document doc;
	doc.set_preserve_cdata(true);

	load_template(file, doc);

	process_tags(doc.child(), scope);

	reply.set_content(doc);
}

void basic_html_controller::init_scope(scope& scope)
{
}

void basic_html_controller::process_tags(xml::node* node, const scope& scope)
{
	// only process elements
	if (dynamic_cast<xml::element*>(node) == nullptr)
		return;

	std::set<std::string> registeredNamespaces;
	for (auto& tpc: m_tag_processor_creators)
		registeredNamespaces.insert(tpc.first);

	if (not registeredNamespaces.empty())
		process_tags(static_cast<xml::element*>(node), scope, registeredNamespaces);

	// decorate all forms with a hidden input with name _csrf

	auto csrf = get_csrf_token(scope);
	if (not csrf.empty())
	{
		auto forms = xml::xpath(R"(//form[not(input[@name='_csrf'])])");
		xml::context ctx;
		for (auto& form: forms.evaluate<xml::element>(*node, ctx))
			form->emplace_back(xml::element("input", {
				{ "name", "_csrf" },
				{ "value", csrf },
				{ "type", "hidden" }
			}));
	}
}

void basic_html_controller::process_tags(xml::element* node, const scope& scope, std::set<std::string> registeredNamespaces)
{
	std::set<std::string> nss;

	for (auto& ns: node->attributes())
	{
		if (not ns.is_namespace())
			continue;

		if (registeredNamespaces.count(ns.value()))
			nss.insert(ns.value());
	}

	for (auto& ns: nss)
	{
		std::unique_ptr<tag_processor> processor(create_tag_processor(ns));
		processor->process_xml(node, scope, "", *this);

		registeredNamespaces.erase(ns);
	}

	if (not registeredNamespaces.empty())
	{
		for (auto& e: *node)
			process_tags(&e, scope, registeredNamespaces);
	}
}

} // namespace http::zeep
