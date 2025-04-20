// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2025
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/html-controller.hpp>
#include <zeep/http/template-processor.hpp>
#include <zeep/http/uri.hpp>

#include "glob.hpp"

namespace fs = std::filesystem;

namespace zeep::http
{

basic_template_processor &html_controller::get_template_processor()
{
	return m_server->get_template_processor();
}

const basic_template_processor &html_controller::get_template_processor() const
{
	return m_server->get_template_processor();
}

// // --------------------------------------------------------------------

// void html_controller::mount_point_v2_simple::call(const scope &scope, const parameter_pack &, reply &rep)
// {
// 	rep = m_controller.get_template_processor().create_reply_from_template(m_template, scope);
// }

// --------------------------------------------------------------------

void html_controller::handle_file(const request &request, const scope &scope, reply &reply)
{
	get_template_processor().handle_file(request, scope, reply);
}

void html_controller::init_scope(request &req, scope &scope)
{
	// set up the scope by putting some globals in it

	auto uri = get_prefixless_path(req);
	auto path = uri.string();
	scope.put("baseuri", path);
}

// --------------------------------------------------------------------
//

bool html_controller::handle_request(request& req, reply& rep)
{
	bool result = controller::handle_request(req, rep);

	if (not result)
	{
		auto uri = get_prefixless_path(req);
		auto path = uri.string();
	
		// set up the scope by putting some globals in it
		scope scope(get_server(), req);
	
		scope.put("baseuri", path);
	
		init_scope(req, scope);
	
		auto handler = find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
			[&uri, method=req.get_method()](const mount_point_v1& m)
			{
				// return m.path == path and
				return glob_match(uri, m.path) and
					(	method == "HEAD" or
						method == "OPTIONS" or
						m.method == method or
						m.method == "UNDEFINED");
			});

		if (handler != m_dispatch_table.end())
		{
			if (req.get_method() == "OPTIONS")
				get_options(req, rep);
			else
				handler->handler(req, scope, rep);

			result = true;
		}
	}

	if (not result)
		rep = reply::stock_reply(not_found);

	return result;
}

} // namespace zeep::http
