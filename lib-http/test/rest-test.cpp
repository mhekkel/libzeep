#define BOOST_TEST_MODULE REST_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/http/rest-controller.hpp>
#include <zeep/exception.hpp>

using namespace std;
namespace z = zeep;
namespace zx = zeep::xml;
namespace zh = zeep::http;


struct Opname
{
	string				id;
	map<string,float>	standen;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
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
	}

	// CRUD routines
	string post_opname(Opname opname)
	{
		return {};
	}

	void put_opname(string opnameId, Opname opname)
	{
		{};
	}

	Opname get_opname(string id)
	{
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

	void delete_opname(string id)
	{
	}

	GrafiekData get_grafiek(grafiek_type type, aggregatie_type aggregatie)
	{
		return{};
	}
};



BOOST_AUTO_TEST_CASE(rest_1)	
{
	// simply see if the above compiles

	e_rest_controller rc;
}

