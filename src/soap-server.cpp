//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>

#include "soap/server.hpp"
#include "soap/envelope.hpp"
#include "soap/xml/document.hpp"

using namespace std;

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace soap {
	
namespace detail {

bool decode_uri(string uri, fs::path& path)
{
	bool result = true;
	
	string url;
	
	for (string::const_iterator c = uri.begin(); c != uri.end(); ++c)
	{
		if (*c == '%')
		{
			result = false;

			if (uri.end() - c > 3)
			{
				int value;
				istringstream is(string(c + 1, c + 2));
				if (is >> std::hex >> value)
				{
					url += static_cast<char>(value);
					c += 2;
					result = true;
				}
			}
		}
		else if (*c == '+')
			url += ' ';
		else
			url += *c;
	}
	
	if (result and ba::starts_with(url, "http://"))
	{
		// turn path into a relative path
		
		string::size_type s = url.find_first_of('/', 7);
		if (s != string::npos)
		{
			while (s < url.length() and url[s] == '/')
				++s;
			url.erase(0, s);
		}
	}

	path = url;
	
	return result;
}
	
}

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
		fs::path path;
		
		if (not detail::decode_uri(req.uri, path))
			throw http::bad_request;
		
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
			fs::path::iterator p = path.begin();
			
			while (p != path.end() and *p == "/")
				++p;
			
			if (p == path.end())
				throw http::bad_request;
			
			string root = *p++;
			
			if (root == "rest")
			{
				action = *p++;
				
				xml::node_ptr request(new xml::node(action));
				while (p != path.end())
				{
					string name = *p++;
					if (p == path.end())
						break;
					xml::node_ptr param(new xml::node(name));
					string value = *p++;
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
