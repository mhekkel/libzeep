// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/algorithm/string.hpp>

#include <zeep/crypto.hpp>
#include <zeep/http/rest-controller.hpp>

namespace ba = boost::algorithm;

namespace zeep::http
{

rest_controller::~rest_controller()
{
    for (auto mp: m_mountpoints)
        delete mp;
}

bool rest_controller::handle_request(http::request& req, http::reply& rep)
{
	std::string p = req.uri;

	if (p.front() == '/')
		p.erase(0, 1);
	
	if (not ba::starts_with(p, m_prefix_path))
		return false;

	p.erase(0, m_prefix_path.length());
	
	if (p.front() == '/')
		p.erase(0, 1);

	auto pp = p.find('?');
	if (pp != std::string::npos)
		p.erase(pp);
	
	p = decode_url(p);

    bool result = false;
	for (auto& mp: m_mountpoints)
	{
		if (req.method != mp->m_method)
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
				params.m_path_parameters.push_back({mp->m_path_params[i], m[i + 1].str()});
		}

		try
		{
			mp->call(params, rep);
		}
		catch(const std::exception& e)
		{
			rep = http::reply::stock_reply(http::internal_server_error);
			
			json::element error({ { "error", e.what() }});
			rep.set_content(error);
		}

		result = true;
		break;
	}

	return result;
}

}