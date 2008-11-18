#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <cerrno>
#include <pwd.h>


#include "soap/dispatcher.hpp"
#include "soap/envelope.hpp"
#include "soap/xml/document.hpp"

#include "soap/http/server.hpp"

#include <iostream>

using namespace std;

// the data types used in our communication with the outside world
// are wrapped in a namespace.

namespace WSSearchNS
{

// the hit information

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

// and the FindResponse type.

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

class my_server
{
  public:
						my_server();

	void				ListDatabanks(
							vector<string>&				databanks);

	void				Count(
							const string&				db,
							const string&				booleanquery,
							int&						result);

	void				Find(
							const string&				db,
							const vector<string>&		queryterms,
							WSSearchNS::Algorithm		algorithm,
							bool						alltermsrequired,
							const string&				booleanfilter,
							int							resultoffset,
							int							maxresultcount,
							WSSearchNS::FindResponse&	out);

	soap::xml::node_ptr	Serve(
							soap::xml::node_ptr			request);

	soap::dispatcher	m_dispatcher;
};

my_server::my_server()
	: m_dispatcher("http://www.hekkelman.com/ws")
{
	using namespace WSSearchNS;

	const char* kListDatabanksParameterNames[] = {
		"databank"
	};
	
	m_dispatcher.register_action("ListDatabanks", this, &my_server::ListDatabanks, kListDatabanksParameterNames);

	const char* kCountParameterNames[] = {
		"db", "booleanquery", "count"
	};
	
	m_dispatcher.register_action("Count", this, &my_server::Count, kCountParameterNames);
	
	SOAP_XML_ADD_ENUM(WSSearchNS::Algorithm, Vector);
	SOAP_XML_ADD_ENUM(WSSearchNS::Algorithm, Dice);
	SOAP_XML_ADD_ENUM(WSSearchNS::Algorithm, Jaccard);

	const char* kFindParameterNames[] = {
		"db", "queryterms", "algorithm",
		"alltermsrequired", "booleanfilter", "resultoffset", "maxresultcount",
		"out"
	};
	
	m_dispatcher.register_action("Find", this, &my_server::Find, kFindParameterNames);
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
	int&						result)
{
	if (db != "sprot" and db != "trembl")
		throw soap::exception("Unknown databank: %s", db.c_str());
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
	WSSearchNS::FindResponse&	out)
{
	assert(db == "sprot");
	assert(queryterms.size() == 2);
	assert(queryterms[0] == "aap");
	assert(queryterms[1] == "noot");
	assert(algorithm == WSSearchNS::Dice);
	assert(booleanfilter.empty());
	assert(resultoffset == 0);
	assert(maxresultcount == 15);
	
	// mock up some fake answer...
	out.count = 2;

	WSSearchNS::hit h;
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

soap::xml::node_ptr my_server::Serve(
	soap::xml::node_ptr		request)
{
	// in this case the name of the request node in the envelope
	// is the same as the SOAP action to execute
	return m_dispatcher.dispatch(request->name(), request);
}

int TCPListen(
	const string&			inAddress,
	short					inPort)
{
	struct addrinfo hints = {}, *res, *ressave;
	int listenfd;
	
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	char port[32];
	snprintf(port, sizeof(port), "%d", inPort);
	
	int err = getaddrinfo(inAddress.c_str(), port, &hints, &res);
	if (err < 0)
		throw soap::exception("TCP Listen error for %s:%d (%s)", inAddress.c_str(), inPort, strerror(errno));
	
	ressave = res;
	for (; res != NULL; res = res->ai_next)
	{
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue;		// try next
		
		int on = 1;
		err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;
		
		close(listenfd);
	}
	
	if (res == NULL)
		throw soap::exception("TCP Listen error for %s:%d (%s)", inAddress.c_str(), inPort, strerror(errno));
	
	err = listen(listenfd, 10 /*kListenQueueSize*/);
	if (err < 0)
		throw soap::exception("TCP Listen error for %s:%d\n%s", inAddress.c_str(), inPort, strerror(errno));
	
	freeaddrinfo(ressave);
	
	return listenfd;
}


int main(int argc, const char* argv[])
{
    sigset_t new_mask, old_mask;
    sigfillset(&new_mask);
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

	soap::http::server server("0.0.0.0", 10333);

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

//	soap::xml::node_ptr response;
//	
//	try
//	{
//		my_server s;
//		
//		soap::xml::document doc(cin);
//
//		soap::envelope env(doc);
//		soap::xml::node_ptr request = env.request();
//		
//		response = soap::make_envelope(s.Serve(request));
//	}
//	catch (exception& e)
//	{
//		response = soap::make_fault(e);
//	}
//
//	cout << "Content-Type: text/xml\r\n"
//		 << "\r\n"
//		 << *response
//		 << "\r\n";

	return 0;
}
