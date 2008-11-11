#include <fstream>
#include <sstream>
#include <vector>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>

#include "xml/node.hpp"
#include "xml/document.hpp"
#include "xml/exception.hpp"
#include "xml/serialize.hpp"
#include "xml/soap/envelope.hpp"
#include "xml/soap/dispatcher.hpp"

#include <iostream>

#include <getopt.h>

using namespace std;

class my_server : public xml::soap::dispatcher<my_server>
{
  public:
						my_server();

	struct hit
	{
		string			db;
		string			id;
		string			title;
		float			score;
		
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & BOOST_SERIALIZATION_NVP(db)
			   & BOOST_SERIALIZATION_NVP(id)
			   & BOOST_SERIALIZATION_NVP(title)
			   & BOOST_SERIALIZATION_NVP(score);
		}
	};

	struct FindResponse
	{
		int				count;
		vector<hit>		hits;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & BOOST_SERIALIZATION_NVP(count)
			   & BOOST_SERIALIZATION_NVP(hits);
		}
	};

	void				Find(
							const string&			db,
							const vector<string>&	queryterms,
							const string&			algorithm,
							const bool&				alltermsrequired,
							const string&			booleanfilter,
							const int&				resultoffset,
							const int&				maxresultcount,
							FindResponse&			out);
};

my_server::my_server()
{
	register_soap_call("Find", &my_server::Find,
		"db", "queryterms", "algorithm", "alltermsrequired",
		"booleanfilter", "resultoffset", "maxresultcount");
}

void my_server::Find(
	const string&			db,
	const vector<string>&	queryterms,
	const string&			algorithm,
	const bool&				alltermsrequired,
	const string&			booleanfilter,
	const int&				resultoffset,
	const int&				maxresultcount,
	FindResponse&			out)
{
	out.count = 1;

	hit h;
	h.db = "sprot";
	h.id = "104k_thepa";
	h.score = 1.0f;
	h.title = "bla bla bla";
	
	out.hits.push_back(h);
}

int main(int argc, const char* argv[])
{
	if (argc != 2)
		exit(1);
	
	ifstream data(argv[1]);
	
	try
	{
		xml::document doc(data);

		xml::soap::envelope env(doc);
		xml::node_ptr req = env.request();
		
		cout << "request:" << endl << *req << endl;
		
		if (req->name() != "Find" or req->ns() != "http://mrs.cmbi.ru.nl/mrsws/search")
			throw xml::exception("Invalid request");

		xml::node_ptr res(new xml::node("FindResponse"));

		my_server s;
		s.dispatch("Find", req, res);
		
		cout << "response: " << endl << *res << endl;
	}
	catch (xml::exception& e)
	{
		cout << e.what() << endl;
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
	}

	return 0;
}
