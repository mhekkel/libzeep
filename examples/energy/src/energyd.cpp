//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <functional>
#include <tuple>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/http/md5.hpp>

#include <boost/filesystem.hpp>
#include <zeep/el/parser.hpp>
#include <zeep/http/rest-controller.hpp>

#include <pqxx/pqxx>

using namespace std;
namespace zh = zeep::http;
namespace el = zeep::el;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
namespace po = boost::program_options;

using json = el::element;

struct Opname
{
	string				id;
	string				datum;
	map<string,float>	standen;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_nvp("id", id)
		   & zeep::make_nvp("datum", datum)
		   & zeep::make_nvp("standen", standen);
	}
};

struct Teller
{
	string id;
	string naam;
	string naam_kort;
	int schaal;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & zeep::make_nvp("id", id)
		   & zeep::make_nvp("naam", naam)
		   & zeep::make_nvp("korteNaam", naam_kort)
		   & zeep::make_nvp("schaal", schaal);
	}
};

enum class aggregatie_type
{
	dag, week, maand, jaar
};

void to_element(json& e, aggregatie_type aggregatie)
{
	switch (aggregatie)
	{
		case aggregatie_type::dag:		e = "dag"; break;
		case aggregatie_type::week:		e = "week"; break;
		case aggregatie_type::maand:	e = "maand"; break;
		case aggregatie_type::jaar:		e = "jaar"; break;
	}
}

void from_element(const json& e, aggregatie_type& aggregatie)
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

void to_element(json& e, grafiek_type type)
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

void from_element(const json& e, grafiek_type& type)
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

string selector(grafiek_type g)
{
	switch (g)
	{
		case grafiek_type::warmte:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (1) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (2, 3, 4, 5) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_hoog:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (3, 5) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_laag:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (2, 4) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_verbruik:
			return "SELECT a.tijd, SUM(b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id IN (2, 3) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_levering:
			return "SELECT a.tijd, SUM(b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id IN (4, 5) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_verbruik_hoog:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 3 ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_verbruik_laag:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 2 ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_levering_hoog:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 5 ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_levering_laag:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 4 ORDER BY a.tijd ASC";
	}
}

struct GrafiekData
{
	string 				type;
	map<string,float>	punten;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & zeep::make_nvp("type", type)
		   & zeep::make_nvp("punten", punten);
	}
};

class my_rest_controller : public zh::rest_controller
{
  public:
	my_rest_controller(const string& connectionString)
		: zh::rest_controller("ajax")
		, m_connection(connectionString)
	{
		map_post_request("opname", &my_rest_controller::post_opname, "opname");
		map_put_request("opname/{id}", &my_rest_controller::put_opname, "id", "opname");
		map_get_request("opname/{id}", &my_rest_controller::get_opname, "id");
		map_get_request("opname", &my_rest_controller::get_all_opnames);
		map_delete_request("opname/{id}", &my_rest_controller::delete_opname, "id");

		map_get_request("data/{type}/{aggr}", &my_rest_controller::get_grafiek, "type", "aggr");

		m_connection.prepare("get-opname-all",
			"SELECT a.id AS id, a.tijd AS tijd, b.teller_id AS teller_id, b.stand AS stand"
			" FROM opname a, tellerstand b"
			" WHERE a.id = b.opname_id"
			" ORDER BY a.tijd DESC");

		m_connection.prepare("get-opname",
			"SELECT a.id AS id, a.tijd AS tijd, b.teller_id AS teller_id, b.stand AS stand"
			" FROM opname a, tellerstand b"
			" WHERE a.id = b.opname_id AND a.id = $1"
			" ORDER BY a.tijd");

		m_connection.prepare("insert-opname", "INSERT INTO opname DEFAULT VALUES RETURNING id");
		m_connection.prepare("insert-stand", "INSERT INTO tellerstand (opname_id, teller_id, stand) VALUES($1, $2, $3);");

		m_connection.prepare("update-stand", "UPDATE tellerstand SET stand = $1 WHERE opname_Id = $2 AND teller_id = $3;");

		m_connection.prepare("del-opname", "DELETE FROM opname WHERE id=$1");

		m_connection.prepare("get-tellers-all",
			"SELECT id, naam, naam_kort, schaal FROM teller ORDER BY id");
	}

	// CRUD routines
	string post_opname(Opname opname)
	{
		pqxx::transaction tx(m_connection);
		auto r = tx.prepared("insert-opname").exec();
		if (r.empty() or r.size() != 1)
			throw runtime_error("Kon geen opname aanmaken");

		int opnameId = r.front()[0].as<int>();

		for (auto stand: opname.standen)
			tx.prepared("insert-stand")(opnameId)(stol(stand.first))(stand.second).exec();

		tx.commit();

		return to_string(opnameId);
	}

	void put_opname(string opnameId, Opname opname)
	{
		pqxx::transaction tx(m_connection);

		for (auto stand: opname.standen)
			tx.prepared("update-stand")(stand.second)(opnameId)(stol(stand.first)).exec();

		tx.commit();
	}

	Opname get_opname(string id)
	{
		pqxx::transaction tx(m_connection);
		auto rows = tx.prepared("get-opname")(id).exec();

		if (rows.empty())
			throw runtime_error("opname niet gevonden");

		Opname result{ rows.front()[0].as<string>(), rows.front()[1].as<string>() };

		for (auto row: rows)
			result.standen[row[2].as<string>()] = row[3].as<float>();
		
		return result;
	}

	vector<Opname> get_all_opnames()
	{
		vector<Opname> result;

		pqxx::transaction tx(m_connection);

		auto rows = tx.prepared("get-opname-all").exec();
		for (auto row: rows)
		{
			auto id = row[0].as<string>();

			if (result.empty() or result.back().id != id)
				result.push_back({id, row[1].as<string>()});

			result.back().standen[row[2].as<string>()] = row[3].as<float>();
		}

		return result;
	}

	void delete_opname(string id)
	{
		pqxx::transaction tx(m_connection);
		tx.prepared("del-opname")(id).exec();
		tx.commit();
	}

	vector<Teller> get_tellers()
	{
		vector<Teller> result;

		pqxx::transaction tx(m_connection);

		auto rows = tx.prepared("get-tellers-all").exec();
		for (auto row: rows)
		{
			auto c1 = row.column_number("id");
			auto c2 = row.column_number("naam");
			auto c3 = row.column_number("naam_kort");
			auto c4 = row.column_number("schaal");

			result.push_back({row[c1].as<string>(), row[c2].as<string>(), row[c3].as<string>(), row[c4].as<int>()});
		}

		return result;
	}

	// GrafiekData get_grafiek(const string& type, aggregatie_type aggregatie);
	GrafiekData get_grafiek(grafiek_type type, aggregatie_type aggregatie);

  private:
	pqxx::connection m_connection;
};

GrafiekData my_rest_controller::get_grafiek(grafiek_type type, aggregatie_type aggr)
{
	using namespace boost::posix_time;
	using namespace boost::gregorian;

	pqxx::transaction tx(m_connection);

	map<date,float> data;
	float standVan = 0, standTot;
	ptime van, tot;

	unique_ptr<date_iterator> iter;
	auto start_w = first_day_of_the_week_before(Sunday);

	for (auto r: tx.exec(selector(type)))
	{
		if (standVan == 0)
		{
			van = time_from_string(r[0].as<string>());
			standVan = r[1].as<float>();
			continue;
		}

		tot = time_from_string(r[0].as<string>());
		standTot = r[1].as<float>();

		auto verbruik = standTot - standVan;

		time_duration dt = tot - van;
		float verbruikPerSeconde = verbruik / dt.total_seconds();

		switch (aggr)
		{
			case aggregatie_type::dag:
				iter.reset(new day_iterator(van.date()));
				break;

			case aggregatie_type::week:
				iter.reset(new week_iterator(start_w.get_date(van.date())));
				break;

			case aggregatie_type::maand:
				iter.reset(new month_iterator(date(van.date().year(), van.date().month(), 1)));
				break;

			case aggregatie_type::jaar:
				iter.reset(new year_iterator(date(van.date().year(), Jan, 1)));
				break;
		}

		auto eind = ((tot + days(1)).date());

		while (**iter < eind)
		{
			auto dag = **iter;

			ptime tijdVan(dag);
			if (tijdVan < van)
				tijdVan = van;

			++*iter;

			ptime tijdTot(**iter);
			if (tijdTot > tot)
				tijdTot = tot;

			auto dagDuur = (tijdTot - tijdVan).total_seconds();
			if (dagDuur <= 0)
				continue;

			float dagVerbruik = verbruikPerSeconde * dagDuur;
			data[dag] += dagVerbruik;
		}

		van = tot;
		standVan = standTot;
	}

	GrafiekData result;

	for (auto p: data)
	{
		date dag;
		float verbruik;
		tie(dag, verbruik) = p;

		result.punten.emplace(to_iso_extended_string(dag), verbruik);
	}

	return result;
}


class my_server : public zh::webapp
{
  public:
	my_server(const string& dbConnectString)
		: zh::webapp("http://www.hekkelman.com/libzeep/ml", fs::current_path() / "docroot")
		, m_rest_controller(new my_rest_controller(dbConnectString))
	{
		add_controller(m_rest_controller);
	
		mount("", &my_server::opname);
		mount("opnames", &my_server::opname);
		mount("grafiek", &my_server::grafiek);
		mount("css", &my_server::handle_file);
		mount("scripts", &my_server::handle_file);
		mount("fonts", &my_server::handle_file);
	}

	void opname(const zh::request& request, const el::scope& scope, zh::reply& reply);
	void grafiek(const zh::request& request, const el::scope& scope, zh::reply& reply);
	void handle_file(const zh::request& request, const el::scope& scope, zh::reply& reply);

  private:
	my_rest_controller*	m_rest_controller;
};

void my_server::opname(const zh::request& request, const el::scope& scope, zh::reply& reply)
{
	el::scope sub(scope);

	sub.put("page", "opname");

	auto v = m_rest_controller->get_all_opnames();
	json opnames;
	zeep::to_element(opnames, v);
	sub.put("opnames", opnames);

	auto u = m_rest_controller->get_tellers();
	json tellers;
	zeep::to_element(tellers, u);
	sub.put("tellers", tellers);

	create_reply_from_template("opnames.html", sub, reply);
}

void my_server::grafiek(const zh::request& request, const el::scope& scope, zh::reply& reply)
{
	el::scope sub(scope);

	sub.put("page", "opname");

	auto v = m_rest_controller->get_all_opnames();
	json opnames;
	zeep::to_element(opnames, v);
	sub.put("opnames", opnames);

	auto u = m_rest_controller->get_tellers();
	json tellers;
	zeep::to_element(tellers, u);
	sub.put("tellers", tellers);

	create_reply_from_template("grafiek.html", sub, reply);
}

void my_server::handle_file(const zh::request& request, const el::scope& scope, zh::reply& reply)
{
	fs::path file = get_docroot() / scope["baseuri"].as<string>();
	
	webapp::handle_file(request, scope, reply);
	
	if (file.extension() == ".html" or file.extension() == ".xhtml")
		reply.set_content_type("application/xhtml+xml");
}

int main(int argc, const char* argv[])
{
	po::options_description visible_options(argv[0] + " options"s);
	visible_options.add_options()
		("help,h",										"Display help message")
		("verbose,v",									"Verbose output")
		
		("address",				po::value<string>(),	"External address, default is 0.0.0.0")
		("port",				po::value<uint16_t>(),	"Port to listen to, default is 10336")
		// ("no-daemon,F",									"Do not fork into background")
		// ("user,u",				po::value<string>(),	"User to run the daemon")
		// ("logfile",				po::value<string>(),	"Logfile to write to, default /var/log/rama-angles.log")

		("db-host",				po::value<string>(),	"Database host")
		("db-port",				po::value<string>(),	"Database port")
		("db-dbname",			po::value<string>(),	"Database name")
		("db-user",				po::value<string>(),	"Database user name")
		("db-password",			po::value<string>(),	"Database password")
		;
	
	po::options_description cmdline_options;
	cmdline_options.add(visible_options);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
	po::notify(vm);

	// --------------------------------------------------------------------

	if (vm.count("help"))
	{
		cerr << visible_options << endl;
		exit(0);
	}
	
	try
	{
		vector<string> vConn;
		for (string opt: { "db-host", "db-port", "db-dbname", "db-user", "db-password" })
		{
			if (vm.count(opt) == 0)
				continue;
			
			vConn.push_back(opt.substr(3) + "=" + vm[opt].as<string>());
		}

		my_server app(ba::join(vConn, " "));

		string address = "0.0.0.0";
		uint16_t port = 10333;
		if (vm.count("address"))
			address = vm["address"].as<string>();
		if (vm.count("port"))
			port = vm["port"].as<uint16_t>();

		app.bind(address, port);
		thread t(bind(&my_server::run, &app, 2));
		t.join();

	}
	catch (const exception& ex)
	{
		cerr << ex.what() << endl;
		exit(1);
	}

	return 0;
}