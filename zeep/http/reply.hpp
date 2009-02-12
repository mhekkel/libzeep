//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_REPLY_HPP
#define SOAP_HTTP_REPLY_HPP

#include <vector>
#include "soap/http/header.hpp"
#include <boost/asio/buffer.hpp>
#include "soap/xml/node.hpp"

namespace soap { namespace http {

enum status_type
{
	cont =					100,
    ok =					200,
    created =				201,
    accepted =				202,
    no_content =			204,
    multiple_choices =		300,
    moved_permanently =		301,
    moved_temporarily =		302,
    not_modified =			304,
    bad_request =			400,
    unauthorized =			401,
    forbidden =				403,
    not_found =				404,
    internal_server_error =	500,
    not_implemented =		501,
    bad_gateway =			502,
    service_unavailable =	503
};

struct reply
{
	status_type			status;
	std::string			status_line;
	std::vector<header>	headers;
	std::string			content;

	void				set_content(xml::node_ptr data);

	std::vector<boost::asio::const_buffer>
						to_buffers();

	std::string			get_as_text();
	
	static reply		stock_reply(status_type inStatus);
};

}
}

#endif
