// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
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
	std::string uri = get_prefixless_path(req);

	// set up the scope by putting some globals in it
	scope scope(get_server(), req);

	scope.put("baseuri", uri);

	init_scope(scope);

    bool result = false;
	for (auto& mp: m_mountpoints)
	{
		if (req.get_method() != mp->m_method and req.get_method() != "OPTIONS")
			continue;
		
		parameter_pack params(req);

		if (mp->m_path_params.empty())
		{
			if (mp->m_path != uri)
				continue;
		}
		else
		{
			std::smatch m;
			if (not std::regex_match(uri, m, mp->m_rx))
				continue;

			for (size_t i = 0; i < mp->m_path_params.size(); ++i)
			{
				std::string v = m[i + 1].str();
				decode_url(v);
				params.m_path_parameters.push_back({ mp->m_path_params[i], v });
			}
		}

		if (req.get_method() == "OPTIONS")
			get_options(req, rep);
		else
			mp->call(scope, params, rep);

		result = true;
		break;
	}

	if (not result)
	{
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

}
