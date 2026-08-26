// SPDX-FileCopyrightText: Maarten L. Hekkelman 2025
// SPDX-License-Identifier: BSL-1.0

#include <boost/asio/ip/address_v6.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../src/detail/signals.hpp"

#include "zeep/el/object.hpp"
#include "zeep/el/serializer.hpp"
#include "zeep/http/asio.hpp"
#include "zeep/http/client.hpp"
#include "zeep/http/controller.hpp"
#include "zeep/http/daemon.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/scope.hpp"
#include "zeep/http/server.hpp"
#include "zeep/http/status.hpp"
#include "zeep/uri.hpp"

#include <zeem/zeem.hpp>


#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

using namespace std;

struct Opname
{
	string id;
	map<string, float> standen;

	template <typename Archive>
	void serialize(Archive &ar, uint64_t /*version*/)
	{
		// clang-format off
		ar & zeem::name_value_pair("id", id)
		   & zeem::name_value_pair("standen", standen);
		// clang-format on
	}

	auto operator<=>(const Opname &) const = default;
};

static_assert(not std::is_constructible_v<zeep::el::object, Opname>);

using a_map_type = map<string, float>;
static_assert(zeep::el::is_serializable_map_type_v<a_map_type>);

TEST_CASE("foo")
{
	Opname opn{ "1", { { "een", 0.1f },
						 { "twee", 0.2f } } };

	zeep::el::object o = zeep::el::serializer<Opname>::serialize(opn);

	std::cout << o << "\n";

	Opname opn2 = zeep::el::serializer<Opname>::deserialize(o);

	CHECK(opn == opn2);
}

TEST_CASE("bar")
{
	std::vector<Opname> opnames{
		{ "1", { { "een", 0.1f },
				   { "twee", 0.2f } } },
		{ "2", { { "drie", 0.3f },
				   { "vier", 0.4f } } },
	};

	zeep::el::object o = zeep::el::serializer<std::vector<Opname>>::serialize(opnames);

	std::cout << o << "\n";

	auto opn2 = zeep::el::serializer<std::vector<Opname>>::deserialize(o);

	CHECK(opnames == opn2);
}

enum class aggregatie_type
{
	dag,
	week,
	maand,
	jaar
};

enum class grafiek_type
{
	warmte,
	electriciteit,
	electriciteit_hoog,
	electriciteit_laag,
	electriciteit_verbruik,
	electriciteit_levering,
	electriciteit_verbruik_hoog,
	electriciteit_verbruik_laag,
	electriciteit_levering_hoog,
	electriciteit_levering_laag
};

struct GrafiekData
{
	string type;
	map<string, float> punten;
	map<string, float> vsGem;

	template <typename Archive>
	void serialize(Archive &ar, uint64_t /* version */)
	{
		// clang-format off
		ar & zeem::name_value_pair("type", type)
		   & zeem::name_value_pair("punten", punten)
		   & zeem::name_value_pair("vsgem", vsGem);
		// clang-format on
	}
};

using Opnames = std::vector<Opname>;

class e_rest_controller : public zeep::http::controller
{
  public:
	e_rest_controller()
		: zeep::http::controller("ajax")
	{
		map_post_request("opname", &e_rest_controller::post_opname, "opname");
		map_put_request("opname/{id}", &e_rest_controller::put_opname, "id", "opname");
		map_get_request("opname/{id}", &e_rest_controller::get_opname, "id");
		map_get_request("opname", &e_rest_controller::get_all_opnames);
		map_delete_request("opname/{id}", &e_rest_controller::delete_opname, "id");

		map_get_request("data/{type}/{aggr}", &e_rest_controller::get_grafiek, "type", "aggr");

		map_get_request("opname", &e_rest_controller::get_opnames);

		map_put_request("opnames", &e_rest_controller::set_opnames, "opnames");

		map_get_request("all_data", &e_rest_controller::get_all_data);

		map_get_request("scope_test", &e_rest_controller::scope_test, "id");
	}

	// CRUD routines
	string post_opname(const Opname& /*opname*/)
	{
		return {};
	}

	void put_opname(const string& /*opnameId*/, const string& /*opnameId*/)
	{
		{};
	}

	Opnames get_opnames()
	{
		return { {}, {} };
	}

	void set_opnames(const Opnames& /*opnames*/)
	{
	}

	Opname get_opname(const string& id)
	{
		if (id == "xxx")
			throw zeep::http::http_status_exception(zeep::http::status_type::not_found);

		return {};
	}

	Opname get_last_opname()
	{
		return {};
	}

	vector<Opname> get_all_opnames()
	{
		return {};
	}

	void delete_opname(const string& /*id*/)
	{
	}

	GrafiekData get_grafiek(grafiek_type /*type*/, grafiek_type /*type*/)
	{
		return {};
	}

	zeep::http::reply get_all_data()
	{
		return { zeep::http::status_type::ok, { 1, 0 }, { { "Content-Length", "13" }, { "Content-Type", "text/plain" } }, "Hello, world!" };
	}

	zeep::http::reply scope_test(const zeep::http::scope &scope, int  /*id*/)
	{
		zeep::http::reply result{ zeep::http::status_type::ok, { 1, 0 }, { { "Content-Length", "13" }, { "Content-Type", "text/plain" } }, "Hello, world!" };

		if (scope.get_request().get_accept("application/json") == 1.0f)
			result.set_content(zeep::el::object{
				{ "message", "Hello, world!" } });
		return result;
	}
};

TEST_CASE("rest_1")
{
	zeep::value_serializer<aggregatie_type>::init({ //
		{ aggregatie_type::dag, "dag" },
		{ aggregatie_type::week, "week" },
		{ aggregatie_type::maand, "maand" },
		{ aggregatie_type::jaar, "jaar" } });

	zeep::value_serializer<grafiek_type>::init({ //
		{ grafiek_type::warmte, "warmte" },
		{ grafiek_type::electriciteit, "electriciteit" },
		{ grafiek_type::electriciteit_hoog, "electriciteit-hoog" },
		{ grafiek_type::electriciteit_laag, "electriciteit-laag" },
		{ grafiek_type::electriciteit_verbruik, "electriciteit-verbruik" },
		{ grafiek_type::electriciteit_levering, "electriciteit-levering" },
		{ grafiek_type::electriciteit_verbruik_hoog, "electriciteit-verbruik-hoog" },
		{ grafiek_type::electriciteit_verbruik_laag, "electriciteit-verbruik-laag" },
		{ grafiek_type::electriciteit_levering_hoog, "electriciteit-levering-hoog" },
		{ grafiek_type::electriciteit_levering_laag, "electriciteit-levering-laag" } });

	// simply see if the above compiles

	e_rest_controller rc;

	zeep::http::reply rep;

	asio_ns::io_context io_context;
	asio_ns::ip::tcp::socket s(io_context);

	zeep::http::request req{ "GET", "/ajax/all_data" };

	CHECK(rc.dispatch_request(s, req, rep));

	CHECK(rep.get_status() == zeep::http::status_type::ok);
	CHECK(rep.get_content_type() == "text/plain");
}

TEST_CASE("rest_2")
{
	// start up a http server and stop it again

	zeep::http::daemon d([]()
		{
		auto s = new zeep::http::server;
		s->add_controller(new e_rest_controller());
		return s; },
		"zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t([&d, port]
		{ return d.run_foreground("::", port); });

	std::clog << "started daemon at port " << port << '\n';

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(100ms);

	zeep::uri uri = std::format("http://localhost:{}/", port);
	
	try
	{
		auto rep = zeep::http::get_request(uri / "ajax/all_data");

		CHECK(rep.get_status() == zeep::http::status_type::ok);
		CHECK(rep.get_content_type() == "text/plain");

		auto reply = zeep::http::get_request(uri / "GET /ajax/xxxx");
		CHECK(reply.get_status() == zeep::http::status_type::not_found);

		// reply = zeep::http::simple_request(uri / "GET /ajax/opname/xxx HTTP/1.0\r\n\r\n");
		reply = zeep::http::get_request(uri / "ajax/opname/xxx", { { "Accept", "application/json" } });
		CHECK(reply.get_status() == zeep::http::status_type::not_found);
		CHECK(reply.get_content_type() == "application/json");
	}
	catch (const std::exception &e)
	{
		std::clog << e.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}

TEST_CASE("rest_3")
{
	// start up a http server and stop it again

	zeep::http::daemon d([]()
		{
		auto s = new zeep::http::server;
		s->add_controller(new e_rest_controller());
		return s; },
		"zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t([&d, port] { return d.run_foreground("::", port); });

	std::clog << "started daemon at port " << port << '\n';

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(100ms);

	zeep::uri uri = std::format("http://localhost:{}/", port);

	try
	{
		auto rep_1 = zeep::http::get_request(uri / "ajax/scope_test", { { "accept", "text/plain" } });
		CHECK(rep_1.get_status() == zeep::http::status_type::ok);
		CHECK(rep_1.get_content_type() == "text/plain");

		auto rep_2 = zeep::http::get_request(uri / "ajax/scope_test", { { "accept", "application/json" } } );
		CHECK(rep_2.get_status() == zeep::http::status_type::ok);
		CHECK(rep_2.get_content_type() == "application/json");
	}
	catch (const std::exception &e)
	{
		std::clog << e.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}

class options_route_controller : public zeep::http::controller
{
  public:
	options_route_controller()
		: zeep::http::controller("r")
	{
		map_get_request("data/{kind,type}/{id}", &options_route_controller::handle_data, "id");
		map_get_request("page/{one,two}", &options_route_controller::handle_page);
	}

	zeep::http::reply handle_data(const zeep::http::scope& /*scope*/, const std::string& id)
	{
		zeep::http::reply rep(zeep::http::status_type::ok);
		rep.set_content(id, "text/plain");
		return rep;
	}

	zeep::http::reply handle_page()
	{
		return zeep::http::reply::stock_reply(zeep::http::status_type::ok);
	}
};

TEST_CASE("mixed options and value route params align")
{
	options_route_controller rc;

	asio_ns::io_context io_context;
	asio_ns::ip::tcp::socket s(io_context);

	// the fixed-options group must not shift the capture index of {id}
	zeep::http::request req{ "GET", "/r/data/type/42" };
	zeep::http::reply rep;

	REQUIRE(rc.dispatch_request(s, req, rep));
	CHECK(rep.get_status() == zeep::http::status_type::ok);
	CHECK(rep.get_content() == "42");
}

TEST_CASE("options-only route matches")
{
	options_route_controller rc;

	asio_ns::io_context io_context;
	asio_ns::ip::tcp::socket s(io_context);

	zeep::http::request req{ "GET", "/r/page/two" };
	zeep::http::reply rep;

	CHECK(rc.dispatch_request(s, req, rep));
}

TEST_CASE("read timeout closes idle connection (slowloris)")
{
	using namespace std::chrono_literals;

	zeep::http::server srv;
	srv.set_read_timeout(400ms);

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	srv.bind("127.0.0.1", port);
	std::thread t([&srv] { srv.run(1); });
	std::this_thread::sleep_for(200ms);

	asio_system_ns::error_code ec;
	asio_ns::io_context io;
	asio_ns::ip::tcp::socket sock(io);
	auto addr = asio_ns::ip::make_address("127.0.0.1", ec);
	REQUIRE(not ec);
	sock.connect(asio_ns::ip::tcp::endpoint(addr, port), ec);
	REQUIRE(not ec);

	// Send nothing; the server must close the connection once the read timeout fires.
	// read_some blocks until the server closes, which should happen after ~400ms.
	char buf[16];
	auto started = std::chrono::steady_clock::now();
	sock.read_some(asio_ns::buffer(buf), ec);
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);

	CHECK(ec); // connection was closed by the server (EOF/reset)

	// Should have happened around the configured timeout, not before.
	CHECK(elapsed >= 300ms);

	sock.close();
	srv.stop();
	t.join();
}

TEST_CASE("plus in path parameter is preserved")
{
	options_route_controller rc;

	asio_ns::io_context io_context;
	asio_ns::ip::tcp::socket s(io_context);

	// '+' in a path parameter must stay literal (not be decoded to a space)
	zeep::http::request req{ "GET", "/r/data/type/a+b" };
	zeep::http::reply rep;

	REQUIRE(rc.dispatch_request(s, req, rep));
	CHECK(rep.get_status() == zeep::http::status_type::ok);
	CHECK(rep.get_content() == "a+b");
}
