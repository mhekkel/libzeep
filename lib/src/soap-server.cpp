// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>

#include <zeep/server.hpp>
#include <zeep/envelope.hpp>
#include <zeep/xml/document.hpp>

namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

namespace zeep {
	
inline std::string to_string(const boost::filesystem::path& p)
{
	return p.string();
}

server::server(const std::string& ns, const std::string& service)
	: dispatcher(ns, service)
{
}

void server::bind(const std::string& address, unsigned short port)
{
	http::server::bind(address, port);
	
	if (m_location.empty())
	{
		if (port != 80)
			m_location = "http://" + address + ':' +
				boost::lexical_cast<std::string>(port) + '/' + m_service;
		else
			m_location = "http://" + address + '/' + m_service;
	}
}

void server::handle_request(const http::request& req, http::reply& rep)
{
	std::string action;
	
	try
	{
		xml::element* response;
		
		if (req.method == http::method_type::POST)	// must be a SOAP call
		{
			xml::document doc;
			doc.read(req.payload);
			envelope env(doc);
			xml::element* request = env.request();
			
			action = request->name();
			log() << action << ' ';
			response = make_envelope(dispatch(action, env.request()));
		}
		else if (req.method == http::method_type::GET)
		{
			// start by sanitizing the request's URI
			std::string uri = req.uri;
	
			// strip off the http part including hostname and such
			if (ba::starts_with(uri, "http://"))
			{
				std::string::size_type s = uri.find_first_of('/', 7);
				if (s != std::string::npos)
					uri.erase(0, s);
			}
			
			// now make the path relative to the root
			while (uri.length() > 0 and uri[0] == '/')
				uri.erase(uri.begin());
	
			fs::path path(uri);
			fs::path::iterator p = path.begin();
			
			if (p == path.end())
				throw http::bad_request;
			
			std::string root = to_string(*p++);
			
			if (root == "rest")
			{
				action = to_string(*p++);
				
				xml::element* request(new xml::element(action));
				while (p != path.end())
				{
					std::string name = http::decode_url(to_string(*p++));
					if (p == path.end())
						break;
					xml::element* param(new xml::element(name));
					std::string value = http::decode_url(to_string(*p++));
					param->content(value);
					request->append(param);
				}
				
				log() << action << ' ';
				response = make_envelope(dispatch(action, request));
			}
			else if (root == "wsdl" or root == "?wsdl")
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
