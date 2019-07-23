// Copyright Maarten L. Hekkelman, Radboud University 2008-2019.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>

namespace zeep
{
namespace http
{

class controller
{
  public:
	controller(const std::string& prefixPath);
	controller(const controller&) = delete;
	controller& operator=(const controller&) = delete;

	virtual ~controller();

	virtual bool handle_request(const request& req, reply& rep);

	std::string prefix() const      { return m_prefixPath; }

  protected:
	std::string m_prefixPath;
};

} // namespace http
} // namespace zeep
