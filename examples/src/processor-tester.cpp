//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <functional>
#include <utility>
#include <filesystem>

#include <zeep/http/webapp.hpp>
#include <zeep/http/md5.hpp>

using namespace std;
namespace zh = zeep::http;
namespace el = zeep::el;
namespace fs = std::filesystem;

class my_webapp : public zh::file_based_webapp
{
  public:
	my_webapp();
	
	void index(const zh::request& request, const zh::scope& scope, zh::reply& reply);
};

my_webapp::my_webapp()
	: zh::file_based_webapp(fs::current_path() / "docroot")
{
	register_tag_processor<zh::tag_processor_v1>("http://www.hekkelman.com/libzeep/ml");
	register_tag_processor<zh::tag_processor_v2>("http://www.hekkelman.com/libzeep/m2");

	mount("", &my_webapp::index);
	mount("index", &my_webapp::index);
	mount("css", &my_webapp::handle_file);
}
	
void my_webapp::index(const zh::request& request, const zh::scope& scope, zh::reply& reply)
{
	zh::scope sub(scope);

	sub.put("username", "maarten");

	create_reply_from_template("index.html", sub, reply);
}

int main(int argc, char* const argv[])
{
	my_webapp app;

	app.bind("0.0.0.0", 10333);
    thread t(bind(&my_webapp::run, &app, 2));
	t.join();
	
	return 0;
}
