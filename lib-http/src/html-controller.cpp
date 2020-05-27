// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <boost/algorithm/string.hpp>

#include <zeep/http/html-controller.hpp>
#include <zeep/http/template-processor.hpp>

#include "glob.hpp"

namespace ba = boost::algorithm;
namespace fs = std::filesystem;
namespace pt = boost::posix_time;

namespace zeep::http
{

basic_template_processor& html_controller::get_template_processor()
{
	return m_server->get_template_processor();
}

const basic_template_processor& html_controller::get_template_processor() const
{
	return m_server->get_template_processor();
}

// --------------------------------------------------------------------

void html_controller::handle_file(const request& request, const scope& scope, reply& reply)
{
	get_template_processor().handle_file(request, scope, reply);
}

// --------------------------------------------------------------------
//

bool html_controller::handle_request(request& req, reply& rep)
{
	std::string uri = req.uri;

	if (uri.front() == '/')
		uri.erase(0, 1);
	
	if (not ba::starts_with(uri, m_prefix_path))
		return false;

	uri.erase(0, m_prefix_path.length());
	
	if (uri.front() == '/')
		uri.erase(0, 1);

	// start by sanitizing the request's URI, first parse the parameters
	std::string ps = req.payload;
	if (req.method != method_type::POST)
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
	scope scope(get_server(), req);

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
				(	method == method_type::HEAD or
					method == method_type::OPTIONS or
					m.method == method or
					m.method == method_type::UNDEFINED);
		});

	bool result = false;

	if (handler != m_dispatch_table.end())
	{
		if (req.method == method_type::OPTIONS)
		{
			rep = reply::stock_reply(ok);
			rep.set_header("Allow", "GET,HEAD,POST,OPTIONS");
			rep.set_content("", "text/plain");
		}
		else
		{
			if (m_server)
			{
				json::element credentials = get_server().get_credentials(req);

				if (credentials)
				{
					if (credentials.is_string())
						req.username = credentials.as<std::string>();
					else if (credentials.is_object())
						req.username = credentials["username"].as<std::string>();

					scope.put("credentials", credentials);
				}
			}

			handler->handler(req, scope, rep);
		}

		result = true;
	}

	if (not result)
		rep = reply::stock_reply(not_found);

	return result;
}

}
