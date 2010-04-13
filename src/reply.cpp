//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "zeep/http/reply.hpp"
#include "zeep/xml/document.hpp"
#include "zeep/xml/writer.hpp"

using namespace std;

namespace zeep { namespace http {

namespace detail {

struct status_string
{
	status_type	code;
	const char*	text;
} kStatusStrings[] = {
	{ cont,					"Continue" },
	{ ok,					"OK" },
	{ not_found,			"Not Found" },
	{ created,				"Created" },
	{ accepted,				"Accepted" },
	{ no_content,			"No Content" },
	{ multiple_choices,		"Multiple Choices" },
	{ moved_permanently,	"Moved Permanently" },
	{ moved_temporarily,	"Moved Temporarily" },
	{ not_modified,			"Not Modified" },
	{ bad_request,			"Bad Request" },
	{ unauthorized,			"Unauthorized" },
	{ forbidden,			"Forbidden" },
	{ not_found,			"Not Found" },
	{ internal_server_error,"Internal Server Error" },
	{ not_implemented,		"Not Implemented" },
	{ bad_gateway,			"Bad Gateway" },
	{ service_unavailable,	"Service Unavailable" }
};

const int
	kStatusStringCount = sizeof(kStatusStrings) / sizeof(status_string);
	
string get_status_text(status_type status)
{
	string result = "Internal Service Error";
	
	for (int i = 0; i < kStatusStringCount; ++i)
	{
		if (kStatusStrings[i].code == status)
		{
			result = kStatusStrings[i].text;
			break;
		}
	}
	
	return result;
}
	
}

// ----------------------------------------------------------------------------

namespace
{
const char
		kNameValueSeparator[] = { ':', ' ' },
		kCRLF[] = { '\r', '\n' };
}

vector<boost::asio::const_buffer> reply::to_buffers()
{
	vector<boost::asio::const_buffer>	result;

	status_line = string("HTTP/1.0 ") + boost::lexical_cast<string>(status) + ' ' +
		detail::get_status_text(status) + " \r\n";
	result.push_back(boost::asio::buffer(status_line));
	
	for (vector<header>::iterator h = headers.begin(); h != headers.end(); ++h)
	{
		result.push_back(boost::asio::buffer(h->name));
		result.push_back(boost::asio::buffer(kNameValueSeparator));
		result.push_back(boost::asio::buffer(h->value));
		result.push_back(boost::asio::buffer(kCRLF));
	}

	result.push_back(boost::asio::buffer(kCRLF));
	result.push_back(boost::asio::buffer(content));

	return result;
}

void reply::set_content(xml::element* data)
{
	stringstream s;

	xml::document doc;
	doc.child(data);
	
	xml::writer w(s);
	w.set_wrap(false);
	w.set_indent(0);
//	w.trim(true);
	doc.write(w);
	
	content = s.str();
	status = ok;
	
	headers.resize(2);
	headers[0].name = "Content-Length";
	headers[0].value = boost::lexical_cast<string>(content.length());
	headers[1].name = "Content-Type";
	headers[1].value = "text/xml; charset=utf-8";
}

void reply::set_content(const string& data, const string& mimetype)
{
	content = data;
	status = ok;
	
	headers.resize(2);
	headers[0].name = "Content-Length";
	headers[0].value = boost::lexical_cast<string>(content.length());
	headers[1].name = "Content-Type";
	headers[1].value = mimetype;
}

string reply::get_as_text()
{
	// for best performance, we pre calculate memory requirements and reserve that first
	status_line = string("HTTP/1.0 ") + boost::lexical_cast<string>(status) + ' ' +
		detail::get_status_text(status) + " \r\n";
	
	int size = status_line.length();
	for (vector<header>::iterator h = headers.begin(); h != headers.end(); ++h)
		size += h->name.length() + 2 + h->value.length() + 2;
	size += 2 + content.length();
	
	string result;
	result.reserve(size);	
	
	result = status_line;
	for (vector<header>::iterator h = headers.begin(); h != headers.end(); ++h)
	{
		result += h->name;
		result += ": ";
		result += h->value;
		result += "\r\n";
	}

	result += "\r\n";
	result += content;
	
	return result;
}

reply reply::stock_reply(status_type status)
{
	reply result;

	result.status = status;
	
	string text = detail::get_status_text(status);
	result.content =
		string("<html><head><title>") + text + "</title></head><body><h1>" +
 		boost::lexical_cast<string>(status) + ' ' + text + "</h1></body></html>";

	result.headers.resize(2);
	result.headers[0].name = "Content-Length";
	result.headers[0].value = boost::lexical_cast<string>(result.content.length());
	result.headers[1].name = "Content-Type";
	result.headers[1].value = "text/html; charset=utf-8";
	
	return result;
}

}
}
