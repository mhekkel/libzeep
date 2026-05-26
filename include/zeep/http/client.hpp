/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Maarten L. Hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
