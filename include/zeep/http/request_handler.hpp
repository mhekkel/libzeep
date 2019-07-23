// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
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

/// Simply an interface

class request_handler
{
public:
	virtual void handle_request(boost::asio::ip::tcp::socket& socket,
								const request& req, reply& reply) = 0;
};

} // namespace http
} // namespace zeep
