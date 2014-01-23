//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/http/md5.hpp>

#include <boost/filesystem.hpp>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

using namespace std;
namespace zh = zeep::http;
namespace el = zeep::http::el;
namespace fs = boost::filesystem;

class my_webapp : public zh::webapp
{
  public:
	my_webapp();
	
	virtual string get_hashed_password(const string& username, const string& realm);
	void welcome(const zh::request& request, const el::scope& scope, zh::reply& reply);
	void status(const zh::request& request, const el::scope& scope, zh::reply& reply);
	void error(const zh::request& request, const el::scope& scope, zh::reply& reply);
	void handle_file(const zh::request& request, const el::scope& scope, zh::reply& reply);
};

my_webapp::my_webapp()
	: webapp("http://www.hekkelman.com/libzeep/ml", fs::current_path() / "docroot")
{
	string realm = "test-realm";
	
	mount("", boost::bind(&my_webapp::welcome, this, _1, _2, _3));
	mount("status", realm, boost::bind(&my_webapp::status, this, _1, _2, _3));
	mount("error", boost::bind(&my_webapp::error, this, _1, _2, _3));
	mount("style.css", boost::bind(&my_webapp::handle_file, this, _1, _2, _3));
}
	
string my_webapp::get_hashed_password(const string& username, const string& realm)
{
	string ha1;
	if (username == "scott")
		ha1 = zh::md5(username + ':' + realm + ':' + "tiger").finalise();

	return ha1;
}

void my_webapp::welcome(const zh::request& request, const el::scope& scope, zh::reply& reply)
{
	create_reply_from_template("index.html", scope, reply);
}

void my_webapp::status(const zh::request& request, const el::scope& scope, zh::reply& reply)
{
	// put the http headers in the scope
	
	el::scope sub(scope);
	vector<el::object> headers;
	foreach (const zh::header& h, request.headers)
	{
		el::object header;
		header["name"] = h.name;
		header["value"] = h.value;
		headers.push_back(header);	
	}
	sub.put("headers", headers);
	
	create_reply_from_template("status.html", sub, reply);
}

void my_webapp::error(const zh::request& request, const el::scope& scope, zh::reply& reply)
{
	zh::parameter_map params;
	get_parameters(scope, params);

	el::scope sub(scope);
	
	el::object error;
	error["nr"] = params.get("err", 0).as<int>();
	error["head"] = "Test of error page";
	error["message"] = "A test of the error page is being looked at";

	sub.put("error", error);
	
	create_reply_from_template("error.html", sub, reply);
}

void my_webapp::handle_file(const zh::request& request, const el::scope& scope, zh::reply& reply)
{
	fs::path file = get_docroot() / scope["baseuri"].as<string>();
	
	webapp::handle_file(request, scope, reply);
	
	if (file.extension() == ".html" or file.extension() == ".xhtml")
		reply.set_content_type("application/xhtml+xml");
}

int main(int argc, char* const argv[])
{
	my_webapp app;

	app.bind("0.0.0.0", 10333);
    boost::thread t(boost::bind(&my_webapp::run, &app, 2));
	t.join();
	
	return 0;
}
