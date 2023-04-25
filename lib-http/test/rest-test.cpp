#include <random>

#include <zeep/http/rest-controller.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/exception.hpp>
#include "../src/signals.hpp"

#define BOOST_TEST_MODULE REST_Test
#include <boost/test/included/unit_test.hpp>

#include "client-test-code.hpp"

using namespace std;
namespace z = zeep;
namespace zx = zeep::xml;
namespace zh = zeep::http;


struct Opname
{
	string				id;
	map<string,float>	standen;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long /*version*/)
	{
		ar & zeep::make_nvp("id", id)
		   & zeep::make_nvp("standen", standen);
	}
};

enum class aggregatie_type
{
	dag, week, maand, jaar
};

void to_element(zeep::json::element& e, aggregatie_type aggregatie)
{
	switch (aggregatie)
	{
		case aggregatie_type::dag:		e = "dag"; break;
		case aggregatie_type::week:		e = "week"; break;
		case aggregatie_type::maand:	e = "maand"; break;
		case aggregatie_type::jaar:		e = "jaar"; break;
	}
}

void from_element(const zeep::json::element& e, aggregatie_type& aggregatie)
{
	if (e == "dag")				aggregatie = aggregatie_type::dag;
	else if (e == "week")		aggregatie = aggregatie_type::week;
	else if (e == "maand")		aggregatie = aggregatie_type::maand;
	else if (e == "jaar")		aggregatie = aggregatie_type::jaar;
	else throw runtime_error("Ongeldige aggregatie");
}


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

void to_element(zeep::json::element& e, grafiek_type type)
{
	switch (type)
	{
		case grafiek_type::warmte:						e = "warmte"; break;
		case grafiek_type::electriciteit:				e = "electriciteit"; break;
		case grafiek_type::electriciteit_hoog:			e = "electriciteit-hoog"; break;
		case grafiek_type::electriciteit_laag:			e = "electriciteit-laag"; break;
		case grafiek_type::electriciteit_verbruik:		e = "electriciteit-verbruik"; break;
		case grafiek_type::electriciteit_levering:		e = "electriciteit-levering"; break;
		case grafiek_type::electriciteit_verbruik_hoog:	e = "electriciteit-verbruik-hoog"; break;
		case grafiek_type::electriciteit_verbruik_laag:	e = "electriciteit-verbruik-laag"; break;
		case grafiek_type::electriciteit_levering_hoog:	e = "electriciteit-levering-hoog"; break;
		case grafiek_type::electriciteit_levering_laag:	e = "electriciteit-levering-laag"; break;
	}
}

void from_element(const zeep::json::element& e, grafiek_type& type)
{
		 if (e == "warmte")						 type = grafiek_type::warmte;						
	else if (e == "electriciteit")				 type = grafiek_type::electriciteit;				
	else if (e == "electriciteit-hoog")			 type = grafiek_type::electriciteit_hoog;			
	else if (e == "electriciteit-laag")			 type = grafiek_type::electriciteit_laag;			
	else if (e == "electriciteit-verbruik")		 type = grafiek_type::electriciteit_verbruik;		
	else if (e == "electriciteit-levering")		 type = grafiek_type::electriciteit_levering;		
	else if (e == "electriciteit-verbruik-hoog") type = grafiek_type::electriciteit_verbruik_hoog;	
	else if (e == "electriciteit-verbruik-laag") type = grafiek_type::electriciteit_verbruik_laag;	
	else if (e == "electriciteit-levering-hoog") type = grafiek_type::electriciteit_levering_hoog;	
	else if (e == "electriciteit-levering-laag") type = grafiek_type::electriciteit_levering_laag;	
	else throw runtime_error("Ongeldige grafiek type");
}

struct GrafiekData
{
	string 				type;
	map<string,float>	punten;
	map<string,float>   vsGem;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & zeep::make_nvp("type", type)
		   & zeep::make_nvp("punten", punten)
		   & zeep::make_nvp("vsgem", vsGem);
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
		return{};
	}

	zeep::http::reply get_all_data()
	{
		return { zeep::http::ok, { 1, 0 }, {
			{ "Content-Length", "13" },
			{ "Content-Type", "text/plain" }
		}, "Hello, world!" };
	}
};

BOOST_AUTO_TEST_CASE(rest_1)	
{
	// simply see if the above compiles

	e_rest_controller rc;

	zeep::http::reply rep;

	asio_ns::io_context io_context;
	asio_ns::ip::tcp::socket s(io_context);

	zeep::http::request req{ "GET", "/ajax/all_data" };

	BOOST_CHECK(rc.dispatch_request(s, req, rep));
	
	BOOST_CHECK(rep.get_status() == zeep::http::ok);
	BOOST_CHECK(rep.get_content_type() == "text/plain");
}

BOOST_AUTO_TEST_CASE(rest_2)	
{
	// start up a http server and stop it again

	zh::daemon d([]() {
		auto s = new zh::server;
		s->add_controller(new e_rest_controller());
		return s;
	}, "zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zh::daemon::run_foreground, d, "::", port));

	std::cerr << "started daemon at port " << port << std::endl;

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	try
	{
		auto rep = simple_request(port, "GET /ajax/all_data HTTP/1.0\r\n\r\n");

		BOOST_CHECK(rep.get_status() == zeep::http::ok);
		BOOST_CHECK(rep.get_content_type() == "text/plain");

		auto reply = simple_request(port, "GET /ajax/xxxx HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::not_found);

		reply = simple_request(port, "GET /ajax/opname/xxx HTTP/1.0\r\n\r\n");
		BOOST_TEST(reply.get_status() == zh::not_found);
		BOOST_CHECK_EQUAL(reply.get_content_type(), "application/json");
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	zeep::signal_catcher::signal_hangup(t);

	t.join();
}