// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <boost/algorithm/string.hpp>

#include <zeep/rest/controller.hpp>
#include <zeep/http/server.hpp>

namespace ba = boost::algorithm;

namespace zeep
{
namespace http
{

thread_local el::element rest_controller::s_credentials;

rest_controller::~rest_controller()
{
    for (auto mp: m_mountpoints)
        delete mp;
}

bool rest_controller::validate_request(request& req, reply& rep, const std::string& realm)
{
	bool valid = false;
	s_credentials.clear();

	try
	{
		if (m_auth and m_auth->get_realm() == realm)
		{
			s_credentials = m_auth->validate_authentication(req);

			if (s_credentials)
				valid = true;
		}
	}
	catch (const unauthorized_exception& ex)
	{
		valid = false;
	}
	
	if (not valid)	
	{
		rep = reply::stock_reply(unauthorized);
		rep.set_content(el::element({
			{ "error", "Unauthorized access, unimplemented validate_request" }
		}));
		rep.set_status(unauthorized);
	}
	
	return valid;
}

bool rest_controller::handle_request(request& req, reply& rep)
{
	std::string p = req.uri;

	if (p.front() == '/')
		p.erase(0, 1);
	
	if (not ba::starts_with(p, m_prefixPath))
		return false;

	p.erase(0, m_prefixPath.length());
	
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

		if (mp->m_realm.empty() or validate_request(req, rep, mp->m_realm))
		{
			try
			{
				mp->call(params, rep);
			}
			catch(const std::exception& e)
			{
				rep = reply::stock_reply(internal_server_error);
				
				el::element error({ { "error", e.what() }});
				rep.set_content(error);
			}
		}

		result = true;
		break;
	}

	return result;
}

}
}