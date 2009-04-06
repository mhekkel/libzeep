//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>

#include "zeep/server.hpp"
#include "zeep/envelope.hpp"
#include "zeep/xml/document.hpp"

using namespace std;

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep {
	
namespace detail {

string decode(const string& s)
{
	string result;
	
	for (string::const_iterator c = s.begin(); c != s.end(); ++c)
	{
		if (*c == '%')
		{
			if (s.end() - c >= 3)
			{
				int value;
				string s2(c + 1, c + 3);
				istringstream is(s2);
				if (is >> std::hex >> value)
				{
					result += static_cast<char>(value);
					c += 2;
				}
			}
		}
		else if (*c == '+')
			result += ' ';
		else
			result += *c;
	}
	return result;
}

}

server::server(const std::string& ns, const std::string& service,
	const std::string& address, short port)
	: dispatcher(ns, service)
	, http::server(address, port)
{
	if (port != 80)
		m_location = "http://" + address + ':' +
			boost::lexical_cast<string>(port) + '/' + service;
	else
		m_location = "http://" + address + '/' + service;
}

void server::handle_request(const http::request& req, http::reply& rep)
{
	string action;
	
	try
	{
		xml::node_ptr response;
		
		if (req.method == "POST")	// must be a SOAP call
		{
			xml::document doc(req.payload);
			envelope env(doc);
			xml::node_ptr request = env.request();
			
			action = request->name();
			log() << action << ' ';
			response = make_envelope(dispatch(action, env.request()));
		}
		else if (req.method == "GET")
		{
			// start by sanitizing the request's URI
			string uri = req.uri;
	
			// strip off the http part including hostname and such
			if (ba::starts_with(uri, "http://"))
			{
				string::size_type s = uri.find_first_of('/', 7);
				if (s != string::npos)
					uri.erase(0, s);
			}
			
			// now make the path relative to the root
			while (uri.length() > 0 and uri[0] == '/')
				uri.erase(uri.begin());
	
			fs::path path(uri);
			fs::path::iterator p = path.begin();
			
			if (p == path.end())
				throw http::bad_request;
			
			string root = *p++;
			
			if (root == "rest")
			{
				action = *p++;
				
				xml::node_ptr request(new xml::node(action));
				while (p != path.end())
				{
					string name = detail::decode(*p++);
					if (p == path.end())
						break;
					xml::node_ptr param(new xml::node(name));
					string value = detail::decode(*p++);
					param->content(value);
					request->add_child(param);
				}
				
				log() << action << ' ';
				response = make_envelope(dispatch(action, request));
			}
			else if (root == "wsdl")
			{
				log() << "wsdl";
				response = make_wsdl(m_location);
			}
			else
			{
				log() << req.uri;
				throw http::not_found;
			}
		}
		else
			throw http::bad_request;
		
		rep.set_content(response);
	}
	catch (std::exception& e)
	{
		rep.set_content(make_fault(e));
	}
	catch (http::status_type& s)
	{
		rep = http::reply::stock_reply(s);
	}
}
	
}
