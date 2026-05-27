//          Copyright Maarten L. Hekkelman 2026
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/exception.hpp"
#include "zeep/http/client.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/security.hpp"
#include "zeep/uri.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <set>
#include <string>
#include <thread>
#include <tuple>

namespace zh = zeep::http;

TEST_CASE("openid-test-1")
{
	auto r = zh::get_request("");

}