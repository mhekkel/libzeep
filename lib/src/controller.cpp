// Copyright Maarten L. Hekkelman, Radboud University 2008-2019.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/controller.hpp>

namespace zeep
{
namespace http
{

controller::controller(const std::string& prefixPath)
	: m_prefixPath(prefixPath)
{
}

controller::~controller()
{
}

bool controller::handle_request(const request& req, reply& rep)
{
	return false;
}

}
}