// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2026
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/http/html-controller.hpp"

#include "zeep/http/controller.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/scope.hpp"
#include "zeep/http/server.hpp"
#include "zeep/http/template-processor.hpp"
#include "zeep/uri.hpp"

#include "glob.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <string>

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

// --------------------------------------------------------------------

reply html_controller::html_mount_point_simple::call(const scope &scope)
{
	return m_controller.get_template_processor().create_reply_from_template(m_template, scope);
}

// --------------------------------------------------------------------

reply html_controller::handle_file(const scope &scope)
{
	assert(scope.get_request().get_method() == "GET" or scope.get_request().get_method() == "HEAD");
	return get_template_processor().create_reply_for_get_file(scope);
}

void html_controller::init_scope(scope &/* scope */)
{
}

// --------------------------------------------------------------------
//

bool html_controller_v1::handle_request(request &req, reply &rep)
{
	bool result = controller::handle_request(req, rep);

	if (not result)
	{
		auto uri = get_prefixless_path(req);
		auto path = uri.string();

		// set up the scope by putting some globals in it
		scope scope(get_server(), req);

		scope.put("baseuri", path);

		init_scope(scope);

		auto handler = std::ranges::find_if(m_dispatch_table,
			[&uri, method = req.get_method()](const mount_point_v1 &m)
			{
				// return m.path == path and
				return glob_match(uri, m.path) and
			           (method == "HEAD" or
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
		rep = reply::stock_reply(status_type::not_found);

	return result;
}

} // namespace zeep::http
