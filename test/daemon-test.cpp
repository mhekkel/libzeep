// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/client.hpp"
#include "zeep/http/controller.hpp"
#include "zeep/http/daemon.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/server.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <pwd.h>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>

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
		return { zh::status_type::ok };
	}
};

TEST_CASE("daemon-test-1")
{
	// start up a http server and stop it again
	// The pid is used in the log directory since it happened that the
	// pid file was still around from a previous test run causing trouble.

	std::filesystem::path log_dir = std::filesystem::temp_directory_path() / std::format("daemon-test-{}", getpid());
	std::filesystem::remove_all(log_dir);

	std::filesystem::create_directories(log_dir);

	std::filesystem::path access_file, error_file;
	access_file = log_dir / "access.log";
	error_file = log_dir / "error.log";

	auto pw = getpwuid(getuid());
	REQUIRE(pw != nullptr);

	zh::daemon d([]()
		{
		auto s = new zh::server;
		s->add_controller(new my_controller());
		return s; },
		log_dir / "daemon-test.pid",
		access_file.string(),
		error_file.string());

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::clog << "starting daemon at port " << port << '\n';

	d.start("::", port, 1, pw->pw_name);

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(100ms);

	zeep::uri uri = std::format("http://localhost:{}/", port);

	auto reply = zeep::http::get_request(uri / "test");
	CHECK(reply.get_status() == zh::status_type::ok);

	std::filesystem::rename(access_file, log_dir / "access.log.1");
	std::filesystem::rename(error_file, log_dir / "error.log.1");

	d.reload();

	std::this_thread::sleep_for(100ms);

	reply = zeep::http::get_request(uri / "test");
	CHECK(reply.get_status() == zh::status_type::ok);

	d.stop();

	CHECK(std::filesystem::file_size(access_file) > 0);
	CHECK(std::filesystem::file_size(error_file) > 0);

	// Clean up
	std::filesystem::remove_all(log_dir);

}