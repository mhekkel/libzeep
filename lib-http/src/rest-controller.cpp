// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/crypto.hpp>
#include <zeep/http/rest-controller.hpp>

namespace zeep::http
{

rest_controller::~rest_controller()
{
    for (auto mp: m_mountpoints)
        delete mp;
}

bool rest_controller::handle_request(http::request& req, http::reply& rep)
{
	auto p = get_prefixless_path(req).string();

    bool result = false;
	for (auto& mp: m_mountpoints)
	{
		if (req.get_method() != mp->m_method)
			continue;
		
		parameter_pack params(req);

		if (mp->m_path_params.empty())
		{
			if (mp->m_path != p)
				continue;
		}
		else
		{
			std::smatch m;
			if (not std::regex_match(p, m, mp->m_rx))
				continue;

			for (size_t i = 0; i < mp->m_path_params.size(); ++i)
			{
				std::string v = m[i + 1].str();
				decode_url(v);
				params.m_path_parameters.push_back({ mp->m_path_params[i], v });
			}
		}

		try
		{
			if (req.get_method() == "OPTIONS")
				get_options(req, rep);
			else
				mp->call(params, rep);
		}
		catch (status_type s)
		{
			rep = http::reply::stock_reply(s);
			
			json::element error({ { "error", get_status_description(s) }});
			rep.set_content(error);
			rep.set_status(s);
		}
		catch (const std::exception& e)
		{
			rep = http::reply::stock_reply(http::internal_server_error);
			
			json::element error({ { "error", e.what() }});
			rep.set_content(error);
			rep.set_status(http::internal_server_error);
		}

		result = true;
		break;
	}

	return result;
}

}
