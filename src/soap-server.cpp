//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>

#include <zeep/server.hpp>
#include <zeep/envelope.hpp>
#include <zeep/xml/document.hpp>

using namespace std;

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep {
	
server::server(const std::string& ns, const std::string& service,
	const std::string& address, short port, int nr_of_threads)
	: dispatcher(ns, service)
	, http::server(address, port, nr_of_threads)
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
		xml::element* response;
		
		if (req.method == "POST")	// must be a SOAP call
		{
			xml::document doc;
			doc.read(req.payload);
			envelope env(doc);
			xml::element* request = env.request();
			
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
				
				xml::element* request(new xml::element(action));
				while (p != path.end())
				{
					string name = decode_url(*p++);
					if (p == path.end())
						break;
					xml::element* param(new xml::element(name));
					string value = decode_url(*p++);
					param->content(value);
					request->append(param);
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
