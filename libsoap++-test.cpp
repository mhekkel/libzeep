#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>

#include "xml/soap/dispatcher.hpp"
#include "xml/document.hpp"

#include <iostream>

#include <getopt.h>

using namespace std;

namespace WSSearchNS
{

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

enum Algorithm
{
	Vector,
	Dice,
	Jaccard
};

}

class my_server
{
  public:
						my_server();

	void				Find(
							const string&			db,
							const vector<string>&	queryterms,
							const string&			algorithm,
//							const bool&				alltermsrequired,
//							const string&			booleanfilter,
//							const int&				resultoffset,
//							const int&				maxresultcount,
							FindResponse&			out);

	xml::soap::dispatcher	m_dispatcher;
};

my_server::my_server()
	: m_dispatcher("http://www.hekkelman.com/ws")
{
	
	

	const char* kFindParameterNames[] = {
		"db", "queryterms", "algorithm",
//		"alltermsrequired", "booleanfilter", "resultoffset", "maxresultcount",
		"out"
	};

	m_dispatcher.register_action("Find", this, &my_server::Find, kFindParameterNames);
}

void my_server::Find(
	const string&			db,
	const vector<string>&	queryterms,
	Algorithm				algorithm,
//	const bool&				alltermsrequired,
//	const string&			booleanfilter,
//	const int&				resultoffset,
//	const int&				maxresultcount,
	FindResponse&			out)
{
	cout << "db: " << db << endl
		 << "queryterms: ";
	copy(queryterms.begin(), queryterms.end(), ostream_iterator<string>(cout, ", "));
	cout << endl
		 << "algorithm: " << algorithm << endl;

	out.count = 1;

	hit h;
	h.db = "sprot";
	h.id = "104k_thepa";
	h.score = 1.0f;
	h.title = "bla bla bla";
	
	out.hits.push_back(h);

	h.db = "sprot";
	h.id = "108_lyces";
	h.score = 0.8f;
	h.title = "aap <&> noot mies";
	
	out.hits.push_back(h);
}

int main(int argc, const char* argv[])
{
	if (argc != 2)
		exit(1);
	
	ifstream data(argv[1]);
	
	try
	{
		my_server s;
		
		xml::document doc(data);

		xml::soap::envelope env(doc);
		xml::node_ptr req = env.request();
		
		cout << "request:" << endl << *req << endl;
		
		if (req->name() != "Find" or req->ns() != "http://mrs.cmbi.ru.nl/mrsws/search")
			throw xml::exception("Invalid request");

		xml::node_ptr res = s.m_dispatcher.dispatch("Find", req);
		
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
