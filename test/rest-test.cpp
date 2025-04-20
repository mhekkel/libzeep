/*-
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2025 Maarten L. Hekkelman
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <zeep/exception.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/rest-controller.hpp>

#include "../src/signals.hpp"

#include "test-main.hpp"

#include "client-test-code.hpp"

#include <iostream>
#include <random>

using namespace std;
namespace z = zeep;
namespace zh = zeep::http;
namespace e = zeep::el;

struct Opname
{
	string id;
	map<string, float> standen;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long /*version*/)
	{
		// clang-format off
		ar & zeep::name_value_pair("id", id)
		   & zeep::name_value_pair("standen", standen);
		// clang-format on
	}

	auto operator<=>(const Opname &) const = default;
};

static_assert(not std::is_constructible_v<e::object, Opname>, "");

using a_map_type = map<string, float>;
static_assert(e::is_serializable_map_type_v<a_map_type>, "");

TEST_CASE("foo")
{
	Opname opn{ "1", { { "een", 0.1f },
						 { "twee", 0.2f } } };

	e::object o = e::serializer<Opname>::serialize(opn);

	std::cout << o << "\n";

	Opname opn2 = e::serializer<Opname>::deserialize(o);

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

	e::object o = e::serializer<std::vector<Opname>>::serialize(opnames);

	std::cout << o << "\n";

	auto opn2 = e::serializer<std::vector<Opname>>::deserialize(o);

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
	void serialize(Archive &ar, unsigned long)
	{
		// clang-format off
		ar & zeep::name_value_pair("type", type)
		   & zeep::name_value_pair("punten", punten)
		   & zeep::name_value_pair("vsgem", vsGem);
		// clang-format on
	}
};

using Opnames = std::vector<Opname>;

class e_rest_controller : public zeep::http::rest_controller
{
  public:
	e_rest_controller()
		: zeep::http::rest_controller("ajax")
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
	}

	// CRUD routines
	string post_opname(Opname /*opname*/)
	{
		return {};
	}

	void put_opname(string /*opnameId*/, string /*opnameId*/)
	{
		{};
	}

	Opnames get_opnames()
	{
		return { {}, {} };
	}

	void set_opnames(Opnames /*opnames*/)
	{
	}

	Opname get_opname(string id)
	{
		if (id == "xxx")
			throw zeep::http::not_found;

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

	void delete_opname(string /*id*/)
	{
	}

	GrafiekData get_grafiek(grafiek_type /*type*/, grafiek_type /*type*/)
	{
		return {};
	}

	zeep::http::reply get_all_data()
	{
		return { zeep::http::ok, { 1, 0 }, { { "Content-Length", "13" }, { "Content-Type", "text/plain" } }, "Hello, world!" };
	}
};

TEST_CASE("rest_1")
{
	mxml::value_serializer<aggregatie_type>::init({ //
		{ aggregatie_type::dag, "dag" },
		{ aggregatie_type::week, "week" },
		{ aggregatie_type::maand, "maand" },
		{ aggregatie_type::jaar, "jaar"	 }
	});

	mxml::value_serializer<grafiek_type>::init({ //
		{ grafiek_type::warmte, "warmte" },
		{ grafiek_type::electriciteit, "electriciteit" },
		{ grafiek_type::electriciteit_hoog, "electriciteit-hoog" },
		{ grafiek_type::electriciteit_laag, "electriciteit-laag" },
		{ grafiek_type::electriciteit_verbruik, "electriciteit-verbruik" },
		{ grafiek_type::electriciteit_levering, "electriciteit-levering" },
		{ grafiek_type::electriciteit_verbruik_hoog, "electriciteit-verbruik-hoog" },
		{ grafiek_type::electriciteit_verbruik_laag, "electriciteit-verbruik-laag" },
		{ grafiek_type::electriciteit_levering_hoog, "electriciteit-levering-hoog" },
		{ grafiek_type::electriciteit_levering_laag, "electriciteit-levering-laag" }});

	// simply see if the above compiles

	e_rest_controller rc;

	zeep::http::reply rep;

	asio_ns::io_context io_context;
	asio_ns::ip::tcp::socket s(io_context);

	zeep::http::request req{ "GET", "/ajax/all_data" };

	CHECK(rc.dispatch_request(s, req, rep));

	CHECK(rep.get_status() == zeep::http::ok);
	CHECK(rep.get_content_type() == "text/plain");
}

TEST_CASE("rest_2")
{
	// start up a http server and stop it again

	zh::daemon d([]()
		{
		auto s = new zh::server;
		s->add_controller(new e_rest_controller());
		return s; },
		"zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zh::daemon::run_foreground, d, "::", port));

	std::clog << "started daemon at port " << port << '\n';

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	try
	{
		auto rep = simple_request(port, "GET /ajax/all_data HTTP/1.0\r\n\r\n");

		CHECK(rep.get_status() == zeep::http::ok);
		CHECK(rep.get_content_type() == "text/plain");

		auto reply = simple_request(port, "GET /ajax/xxxx HTTP/1.0\r\n\r\n");
		CHECK(reply.get_status() == zh::not_found);

		reply = simple_request(port, "GET /ajax/opname/xxx HTTP/1.0\r\n\r\n");
		CHECK(reply.get_status() == zh::not_found);
		CHECK(reply.get_content_type() == "application/json");
	}
	catch (const std::exception &e)
	{
		std::clog << e.what() << '\n';
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}