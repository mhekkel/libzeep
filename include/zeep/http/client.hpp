//          Copyright Maarten L. Hekkelman 2026
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of routines to do HTTP requests

#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/uri.hpp"

namespace zeep::http
{

/// \brief Send request \a req and return the reply
///
/// Note that the uri in the request in this case is a fully qualified URI.
/// This will be rewritten in the actual request to just the path and the
/// hostname and port will be put in the headers.
reply send_request(request req);

/// \brief Do a simple HTTP GET request using \a uri as URL and \a headers as (extra) headers
reply get_request(const zeep::uri &uri, std::vector<zeep::http::header> headers = {});

/// \brief Do a simple HTTP HEAD request using \a uri as URL and \a headers as (extra) headers
///
/// Since this is a HEAD request, no data will be returned, just the status code and reply headers
reply head_request(const zeep::uri &uri, std::vector<zeep::http::header> headers = {});

/// \brief Do a simple HTTP POST request using \a uri as URL and \a headers as (extra) headers and \a payload as content
///
/// The Content-Type header will be set to `application/plain` unless set in \a headers
reply post_request(const zeep::uri &uri, std::vector<zeep::http::header> headers, const std::string &payload);

} // namespace zeep::http
