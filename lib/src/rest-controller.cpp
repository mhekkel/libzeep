// Copyright Maarten L. Hekkelman, Radboud University 2008-2019.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <boost/algorithm/string.hpp>

#include <zeep/http/rest-controller.hpp>
#include <zeep/http/server.hpp>

namespace ba = boost::algorithm;

namespace zeep
{
namespace http
{

rest_controller::~rest_controller()
{
    for (auto mp: m_mountpoints)
        delete mp;
}

bool rest_controller::handle_request(const request& req, reply& rep)
{
	std::string p = req.uri;

	if (p.front() == '/')
		p.erase(0, 1);

	if (ba::starts_with(p, m_prefixPath))
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
				const_cast<request&>(req).path_params.push_back({mp->m_path_params[i], m[i + 1].str()});
		}

		mp->call(req, rep);

		result = true;
		break;
	}

	return result;
}

}
}