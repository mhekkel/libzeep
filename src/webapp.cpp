// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem/fstream.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/http/webapp/el.hpp>

using namespace std;
namespace ba = boost::algorithm;
namespace io = boost::iostreams;
namespace fs = boost::filesystem;

namespace zeep { namespace http {

// --------------------------------------------------------------------
//

webapp::webapp(
	const std::string&	ns,
	const fs::path&		docroot)
	: template_processor(ns, docroot)
{
}

webapp::~webapp()
{
}

void webapp::handle_request(
	const request&		req,
	reply&				rep)
{
	string uri = req.uri;

	// shortcut, only handle GET, POST and PUT
	if (req.method != "GET" and req.method != "POST" and req.method != "PUT" and
		req.method != "OPTIONS" and req.method != "HEAD")
	{
		rep = reply::stock_reply(bad_request);
		return;
	}

	try
	{
		// start by sanitizing the request's URI, first parse the parameters
		string ps = req.payload;
		if (req.method != "POST")
		{
			string::size_type d = uri.find('?');
			if (d != string::npos)
			{
				ps = uri.substr(d + 1);
				uri.erase(d, string::npos);
			}
		}
		
		// strip off the http part including hostname and such
		if (ba::starts_with(uri, "http://"))
		{
			string::size_type s = uri.find_first_of('/', 7);
			if (s != string::npos)
				uri.erase(0, s);
		}
		
		// now make the path relative to the root
		while (uri.length() > 0 and uri[0] == '/')
			uri.erase(uri.begin());
		
		// decode the path elements
		string action = uri;
		string::size_type s = action.find('/');
		if (s != string::npos)
			action.erase(s, string::npos);
		
		// set up the scope by putting some globals in it
		el::scope scope(req);
		scope.put("action", el::object(action));
		scope.put("uri", el::object(uri));
		s = uri.find('?');
		if (s != string::npos)
			uri.erase(s, string::npos);
		scope.put("baseuri", uri);
		scope.put("mobile", req.is_mobile());

		handler_map::iterator handler = m_dispatch_table.find(uri);
		if (handler == m_dispatch_table.end())
			handler = m_dispatch_table.find(action);

		if (handler != m_dispatch_table.end())
		{
			if (req.method == "OPTIONS")
			{
				rep = reply::stock_reply(ok);
				rep.set_header("Allow", "GET,HEAD,POST,OPTIONS");
				rep.set_content("", "text/plain");
			}
			else
			{
				init_scope(scope);
				
				handler->second(req, scope, rep);
				
				if (req.method == "HEAD")
					rep.set_content("", rep.get_content_type());
			}
		}
		else
			throw not_found;
	}
	catch (unauthorized_exception& e)
	{
		create_unauth_reply(e.m_stale, e.m_realm, rep);
	}
	catch (status_type& s)
	{
		rep = reply::stock_reply(s);
	}
	catch (std::exception& e)
	{
		el::scope scope(req);
		scope.put("errormsg", el::object(e.what()));

		create_reply_from_template("error.html", scope, rep);
	}
}

void webapp::create_unauth_reply(bool stale, const string& realm, reply& rep)
{
	rep = reply::stock_reply(unauthorized);
}

void webapp::mount(const std::string& path, handler_type handler)
{
	m_dispatch_table[path] = handler;
}

void webapp::handle_file(
	const zeep::http::request&	request,
	const el::scope&			scope,
	zeep::http::reply&			reply)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;
	
	fs::path file = get_docroot() / scope["baseuri"].as<string>();

	string ifModifiedSince;
	foreach (const zeep::http::header& h, request.headers)
	{
		if (h.name == "If-Modified-Since")
		{
			local_date_time modifiedSince(local_sec_clock::local_time(time_zone_ptr()));

			local_time_input_facet* lif1(new local_time_input_facet("%a, %d %b %Y %H:%M:%S GMT"));
			
			stringstream ss;
			ss.imbue(std::locale(std::locale::classic(), lif1));
			ss.str(h.value);
			ss >> modifiedSince;

			local_date_time fileDate(from_time_t(fs::last_write_time(file)), time_zone_ptr());

			if (fileDate <= modifiedSince)
			{
				reply = zeep::http::reply::stock_reply(zeep::http::not_modified);
				return;
			}
			
			break;
		}
	}
	
	fs::ifstream in(file, ios::binary);
	stringstream out;

	io::copy(in, out);

	string mimetype = "text/plain";

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
	local_time_facet* lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));
	
	stringstream s;
	s.imbue(std::locale(std::cout.getloc(), lf));
	
	ptime pt = from_time_t(boost::filesystem::last_write_time(file));
	local_date_time t2(pt, time_zone_ptr());
	s << t2;

	reply.set_header("Last-Modified", s.str());
}

void webapp::get_cookies(
	const el::scope&	scope,
	parameter_map&		cookies)
{
	const request& req = scope.get_request();
	foreach (const header& h, req.headers)
	{
		if (h.name != "Cookie")
			continue;
		
		vector<string> rawCookies;
		ba::split(rawCookies, h.value, ba::is_any_of(";"));
		
		foreach (string& cookie, rawCookies)
		{
			ba::trim(cookie);
			cookies.add(cookie);
		}
	}
}

}
}
