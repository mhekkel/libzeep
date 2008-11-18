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
		string::size_type s = url.find_first_of('/', 7);
		if (s != string::npos)
			path = url.substr(s);
	}
	else	// assume a get without full url?
		path = url;
	
	return result;
}
	
}

void server::handle_request(const http::request& req, http::reply& rep)
{
	try
	{
		fs::path path;
		
		if (not detail::decode_uri(req.uri, path))
			rep = http::reply::stock_reply(http::bad_request);
		else if (req.method == "POST")	// must be a SOAP call
		{
			xml::document doc(req.payload);
			envelope env(doc);
			xml::node_ptr request = env.request();
			
			string action = request->name();

			xml::node_ptr response = dispatch(action, env.request());
			
			rep.set_content(make_envelope(response));
		}
		else if (req.method == "GET")
		{
			fs::path::iterator p = path.begin();
			
			if (p != path.end() and *p == "/")
				++p;
			
			if (*p == "rest")
			{
				++p;
			}
			else if (*p == "wsdl")
			{
				
			}
			else
				rep = http::reply::stock_reply(http::not_found);
		}
		else
			rep = http::reply::stock_reply(http::bad_request);
	}
	catch (exception& e)
	{
		rep.set_content(make_fault(e));
	}	
}
	
}
