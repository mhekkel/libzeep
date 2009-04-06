//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/server.hpp"

#include <boost/lexical_cast.hpp>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

// the data types used in our communication with the outside world
// are wrapped in a namespace.

namespace WSSearchNS
{

// the hit information

struct Hit
{
	string			db;
	string			id;
	string			title;
	float			score;

					Hit() : score(0) {}
	
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(db)
		   & BOOST_SERIALIZATION_NVP(id)
		   & BOOST_SERIALIZATION_NVP(title)
		   & BOOST_SERIALIZATION_NVP(score);
	}
};

// and the FindResult type.

struct FindResult
{
	int				count;
	vector<Hit>		hits;

					FindResult() : count(0) {}

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(count)
		   & BOOST_SERIALIZATION_NVP(hits);
	}
};

// these are the supported algorithms for ranked searching

enum Algorithm
{
	Vector,
	Dice,
	Jaccard
};

}

// now construct a server that can do several things:
// ListDatabanks:	simply return the list of searchable databanks
// Count:			a simple call taking two parameters and returning one 
// Find:			complex search routine taking several parameters and returning a complex type

class my_server : public zeep::server
{
  public:
						my_server(
							const string&				address,
							short						port);

	void				ListDatabanks(
							vector<string>&				databanks);

	void				Count(
							const string&				db,
							const string&				booleanquery,
							unsigned int&				result);

	void				Find(
							const string&				db,
							const vector<string>&		queryterms,
							WSSearchNS::Algorithm		algorithm,
							bool						alltermsrequired,
							const string&				booleanfilter,
							int							resultoffset,
							int							maxresultcount,
							WSSearchNS::FindResult&		out);

};

my_server::my_server(const string& address, short port)
	: zeep::server("http://mrs.cmbi.ru.nl/mrsws/search", "zeep", address, port)
{
	using namespace WSSearchNS;

	SOAP_XML_SET_STRUCT_NAME(Hit);
	SOAP_XML_SET_STRUCT_NAME(FindResult);

	const char* kListDatabanksParameterNames[] = {
		"databank"
	};
	
	register_action("ListDatabanks", this, &my_server::ListDatabanks, kListDatabanksParameterNames);

	const char* kCountParameterNames[] = {
		"db", "booleanquery", "response"
	};
	
	register_action("Count", this, &my_server::Count, kCountParameterNames);
	
	SOAP_XML_ADD_ENUM(Algorithm, Vector);
	SOAP_XML_ADD_ENUM(Algorithm, Dice);
	SOAP_XML_ADD_ENUM(Algorithm, Jaccard);

	const char* kFindParameterNames[] = {
		"db", "queryterms", "algorithm",
		"alltermsrequired", "booleanfilter", "resultoffset", "maxresultcount",
		"out"
	};
	
	register_action("Find", this, &my_server::Find, kFindParameterNames);
}

void my_server::ListDatabanks(
	vector<string>&				response)
{
	response.push_back("sprot");
	response.push_back("trembl");
}

void my_server::Count(
	const string&				db,
	const string&				booleanquery,
	unsigned int&				result)
{
	if (db != "sprot" and db != "trembl" and db != "uniprot")
		throw zeep::exception("Unknown databank: %s", db.c_str());

	log() << db;

	result = 10;
}

void my_server::Find(
	const string&				db,
	const vector<string>&		queryterms,
	WSSearchNS::Algorithm		algorithm,
	bool						alltermsrequired,
	const string&				booleanfilter,
	int							resultoffset,
	int							maxresultcount,
	WSSearchNS::FindResult&		out)
{
	log() << db;

	// mock up some fake answer...
	out.count = 2;

	WSSearchNS::Hit h;
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

#define FORKED_MODE 1

int main(int argc, const char* argv[])
{
#if defined(FORKED_MODE)
 	for (;;)
 	{
 		cout << "restarting server" << endl;
 		
	    sigset_t new_mask, old_mask;
	    sigfillset(&new_mask);
	    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
	
		auto_ptr<zeep::http::server_starter> starter(
			zeep::http::server_starter::create<my_server>("0.0.0.0", 10333, true, 2));
		boost::thread t(
			boost::bind(&zeep::http::server_starter::run, starter.get()));
	
	    pthread_sigmask(SIG_SETMASK, &old_mask, 0);
	
		// start listening
		starter->start_listening();
	 
		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		sigaddset(&wait_mask, SIGCHLD);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
		int sig = 0;
		sigwait(&wait_mask, &sig);
	
		starter->stop();
		t.join();
		
		if (sig == SIGCHLD)
		{
			int status, pid;
			pid = waitpid(-1, &status, WUNTRACED);
	
			if (pid != -1 and WIFSIGNALED(status))
				cout << "child " << pid << " terminated by signal " << WTERMSIG(status) << endl;
			continue;
		}
		
		break;
 	}
#else
    sigset_t new_mask, old_mask;
    sigfillset(&new_mask);
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

	my_server server("0.0.0.0", 10333);
    boost::thread t(boost::bind(&my_server::run, &server));

    pthread_sigmask(SIG_SETMASK, &old_mask, 0);

	// Wait for signal indicating time to shut down.
	sigset_t wait_mask;
	sigemptyset(&wait_mask);
	sigaddset(&wait_mask, SIGINT);
	sigaddset(&wait_mask, SIGQUIT);
	sigaddset(&wait_mask, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
	int sig = 0;
	sigwait(&wait_mask, &sig);
	
	server.stop();
	t.join();
#endif
	return 0;
}
