// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <zeep/streambuf.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/http/message-parser.hpp>

#include <cstdint>

zeep::http::reply simple_request(uint16_t port, const std::string& req);
zeep::http::reply simple_request(uint16_t port, const zeep::http::request& req);
