// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2022-2026
// SPDX-License-Identifier: BSL-1.0

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
/// \param req  The request to send (with a fully qualified URI)
/// \return     The reply received from the server
reply send_request(request req);

/// \brief Do a simple HTTP GET request using \a uri as URL and \a headers as (extra) headers
/// \param uri      The URL to request
/// \param headers  Optional extra HTTP headers
/// \return         The reply received from the server
reply get_request(const zeep::uri &uri, std::vector<zeep::http::header> headers = {});

/// \brief Do a simple HTTP HEAD request using \a uri as URL and \a headers as (extra) headers
///
/// Since this is a HEAD request, no data will be returned, just the status code and reply headers
/// \param uri      The URL to request
/// \param headers  Optional extra HTTP headers
/// \return         The reply received from the server (status and headers only)
reply head_request(const zeep::uri &uri, std::vector<zeep::http::header> headers = {});

/// \brief Do a simple HTTP POST request using \a uri as URL and \a headers as (extra) headers and \a payload as content
///
/// The Content-Type header will be set to `application/plain` unless set in \a headers
/// \param uri      The URL to request
/// \param headers  The HTTP headers to include
/// \param payload  The request body content
/// \return         The reply received from the server
reply post_request(const zeep::uri &uri, std::vector<zeep::http::header> headers, const std::string &payload);

} // namespace zeep::http
