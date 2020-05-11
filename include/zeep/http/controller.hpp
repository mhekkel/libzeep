// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the base class zeep::http::controller, used by e.g. rest_controller and soap_controller

#include <zeep/http/request.hpp>
#include <zeep/http/reply.hpp>

namespace zeep::http
{

class controller
{
  public:
	controller(const std::string& prefixPath);

	virtual ~controller();

	virtual bool handle_request(request& req, reply& rep);

	std::string get_prefix() const      { return m_prefix_path; }

  protected:

	controller(const controller&) = delete;
	controller& operator=(const controller&) = delete;

	std::string m_prefix_path;
};

} // namespace zeep::http
