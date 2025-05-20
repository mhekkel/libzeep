#include "zeep/http/asio.hpp"

#include "test-main.hpp"

#include <iostream>
#include <random>
#include <sstream>

#include <zeep/crypto.hpp>
#include <zeep/exception.hpp>
#include <zeep/streambuf.hpp>

#include <zeep/http/controller.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/server.hpp>

#include "client-test-code.hpp"
#include "../src/signals.hpp"

#include <sys/types.h>
#include <pwd.h>

namespace z = zeep;
namespace zh = zeep::http;


// a very simple controller, serving only /test/one and /test/three
class my_controller : public zh::controller
{
	public:
	my_controller()	
		: zh::controller("/")
	{
		map_get_request("test", &my_controller::test);
	}

	zh::reply test()
	{
		return zeep::http::reply(zh::ok);
	}
};


TEST_CASE("daemon-test-1")
{
	// start up a http server and stop it again

	std::filesystem::path log_dir = std::filesystem::temp_directory_path() / "daemon-test";
	std::filesystem::remove_all(log_dir);

	std::filesystem::create_directories(log_dir);

	std::filesystem::path access_file, error_file;
	access_file = log_dir / "access.log";
	error_file = log_dir / "error.log";

	auto pw = getpwuid(getuid());
	REQUIRE(pw != nullptr);

	zh::daemon d([]() {
		auto s = new zh::server;
		s->add_controller(new my_controller());
		return s;
	}, log_dir / "daemon-test.pid", access_file.string(), error_file.string());

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::clog << "starting daemon at port " << port << '\n';

	SECTION("single process")
	{
		d.start("::", port, 1, pw->pw_name);
	}

	SECTION("pre-forked process")
	{
		d.start("::", port, 1, 1, pw->pw_name);
	}
		
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	auto reply = simple_request(port, "GET /test HTTP/1.0\r\n\r\n");
	CHECK(reply.get_status() == zh::ok);

	std::filesystem::rename(access_file, log_dir / "access.log.1");
	std::filesystem::rename(error_file, log_dir / "error.log.1");

	d.reload();

	std::this_thread::sleep_for(1s);

	reply = simple_request(port, "GET /test HTTP/1.0\r\n\r\n");
	CHECK(reply.get_status() == zh::ok);	

	d.stop();

	CHECK(std::filesystem::file_size(access_file) > 0);
	CHECK(std::filesystem::file_size(error_file) > 0);
}