// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/html-controller-2.hpp>
#include <zeep/http/template-processor.hpp>

#include "glob.hpp"

namespace fs = std::filesystem;

namespace zeep::http
{

basic_template_processor& html_controller_2::get_template_processor()
{
	return m_server->get_template_processor();
}

const basic_template_processor& html_controller_2::get_template_processor() const
{
	return m_server->get_template_processor();
}

// --------------------------------------------------------------------

void html_controller_2::handle_file(const request& request, const scope& scope, reply& reply)
{
	get_template_processor().handle_file(request, scope, reply);
}

// --------------------------------------------------------------------
//

bool html_controller_2::handle_request(request& req, reply& rep)
{
	std::string uri = get_prefixless_path(req);

	// set up the scope by putting some globals in it
	scope scope(get_server(), req);

	scope.put("baseuri", uri);

	init_scope(scope);

	auto handler = find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
		[uri, method=req.get_method()](const mount_point& m)
		{
			// return m.path == uri and
			return glob_match(uri, m.path) and
				(	method == "HEAD" or
					method == "OPTIONS" or
					m.method == method or
					m.method == "UNDEFINED");
		});

	bool result = false;

	if (handler != m_dispatch_table.end())
	{
		if (req.get_method() == "OPTIONS")
			get_options(req, rep);
		else
			handler->handler(req, scope, rep);

		result = true;
	}

	if (not result)
		rep = reply::stock_reply(not_found);

	return result;
}

}
