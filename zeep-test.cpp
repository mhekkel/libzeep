// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#ifdef NO_PREFORK
#undef SOAP_SERVER_HAS_PREFORK
#endif

#if SOAP_SERVER_HAS_PREFORK
#include <zeep/http/preforked-server.hpp>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <zeep/server.hpp>
#include <zeep/http/webapp/el.hpp>

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>
#include <boost/optional.hpp>

using namespace std;

int TRACE, VERBOSE;

// the data types used in our communication with the outside world
// are wrapped in a namespace.

namespace WSSearchNS
{

// the hit information

enum HitType
{
	HitTypeOne, HitTypeTwo
};

struct Hit
{
	HitType			type;
	string			db;
	string			id;
	string			title;
	float			score;
	int				v_int;
	unsigned int	v_uint;
	long			v_long;
	unsigned long	v_ulong;
	long int		v_long2;
	long unsigned int	v_ulong2;
	long long		v_longlong;
	unsigned long long
					v_ulonglong;
	int64			v_longlong2;
	uint64			v_ulonglong2;
	long int			v_longlong3;
	unsigned long int	v_ulonglong3;
	boost::posix_time::ptime v_ptime;
	boost::optional<string>  opt_text;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & ZEEP_ATTRIBUTE_NAME_VALUE(type)
		   & ZEEP_ATTRIBUTE_NAME_VALUE(db)
		   & ZEEP_ATTRIBUTE_NAME_VALUE(id)
		   & ZEEP_ELEMENT_NAME_VALUE(title)
		   & ZEEP_ELEMENT_NAME_VALUE(v_int)
		   & ZEEP_ELEMENT_NAME_VALUE(v_uint)
		   & ZEEP_ELEMENT_NAME_VALUE(v_long)
		   & ZEEP_ELEMENT_NAME_VALUE(v_ulong)
		   & ZEEP_ELEMENT_NAME_VALUE(v_long2)
		   & ZEEP_ELEMENT_NAME_VALUE(v_ulong2)
		   & ZEEP_ELEMENT_NAME_VALUE(v_longlong)
		   & ZEEP_ELEMENT_NAME_VALUE(v_ulonglong)
		   & ZEEP_ELEMENT_NAME_VALUE(v_longlong2)
		   & ZEEP_ELEMENT_NAME_VALUE(v_ulonglong2)
		   & ZEEP_ELEMENT_NAME_VALUE(v_longlong3)
		   & ZEEP_ELEMENT_NAME_VALUE(v_ulonglong3)
		   & ZEEP_ELEMENT_NAME_VALUE(v_ptime)
		   & ZEEP_ELEMENT_NAME_VALUE(score)
		   & ZEEP_ELEMENT_NAME_VALUE(opt_text)
		;
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
		ar & zeep::xml::make_attribute_nvp("count", count)
		   & zeep::xml::make_element_nvp("hit", hits);
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

namespace zeep {
namespace xml {

template<class Archive>
struct struct_serializer<Archive,WSSearchNS::Hit>
{
	static void serialize(Archive& ar, WSSearchNS::Hit& hit)
	{
//		ar & BOOST_SERIALIZATION_NVP(hit.db)
//			& BOOST_SERIALIZATION_NVP(hit.id)
//			& BOOST_SERIALIZATION_NVP(hit.title)
//			& BOOST_SERIALIZATION_NVP(hit.score);
		hit.serialize(ar, 0);
	}
};

template <typename Archive, typename T, typename U>
struct struct_serializer<Archive,std::pair<T, U> >
{
    static void serialize(Archive& ar, std::pair<T, U>& pair)
    {
        ar & BOOST_SERIALIZATION_NVP(pair.first);
        ar & BOOST_SERIALIZATION_NVP(pair.second);
    }
};

}
}

// now construct a server that can do several things:
// ListDatabanks:	simply return the list of searchable databanks
// Count:			a simple call taking two parameters and returning one 
// Find:			complex search routine taking several parameters and returning a complex type

class my_server : public zeep::server
{
  public:
						my_server(
							const string&				my_param);

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

	void				PairTest(const string& in, pair<int,int>& out);

	void				DateTimeTest(
							const boost::posix_time::ptime&
														in,
							boost::posix_time::ptime&	out);

	void				ForceStop(string&				out);

  protected:
	/* Uncomment to have your custom http logger
	virtual void		log_request(const boost::asio::ip::address& addr,
							const zeep::http::request& req, const zeep::http::reply& rep,
							const boost::posix_time::ptime& start,
							const std::string& referer, const std::string& userAgent,
							const std::string& entry)
	{
		cout << "Not logging :) ..." << endl;
	}
	*/

  private:
	string				m_param;
};

my_server::my_server(const string& param)
	: zeep::server("http://mrs.cmbi.ru.nl/mrsws/search", "zeep")
	, m_param(param)
{
//	set_location("http://131.174.88.174:10333");

	using namespace WSSearchNS;

	zeep::xml::enum_map<HitType>::instance("HitType").add_enum()
		( "HitTypeOne", HitTypeOne )
		( "HitTypeTwo", HitTypeTwo )
		;

	zeep::xml::struct_serializer_impl<Hit>::set_struct_name("Hit");
	
	// The next call is needed since FindResult is defined in another namespace
	SOAP_XML_SET_STRUCT_NAME(FindResult);

	const char* kListDatabanksParameterNames[] = {
		"databank"
	};
	
	register_action("ListDatabanks", this, &my_server::ListDatabanks, kListDatabanksParameterNames);

	const char* kCountParameterNames[] = {
		"db", "booleanquery", "response"
	};
	
	register_action("Count", this, &my_server::Count, kCountParameterNames);
	
	// a new way of mapping enum values to strings. 
	zeep::xml::enum_map<Algorithm>::instance("Algorithm").add_enum()
		( "Vector", Vector )
		( "Dice", Dice )
		( "Jaccard", Jaccard )
		;

	// this is the old, macro based way of mapping the enums:
//	SOAP_XML_ADD_ENUM(Algorithm, Vector);
//	SOAP_XML_ADD_ENUM(Algorithm, Dice);
//	SOAP_XML_ADD_ENUM(Algorithm, Jaccard);

	const char* kFindParameterNames[] = {
		"db", "queryterms", "algorithm",
		"alltermsrequired", "booleanfilter", "resultoffset", "maxresultcount",
		"out"
	};
	
	register_action("Find", this, &my_server::Find, kFindParameterNames);

	const char* kDateTimeTestParameterNames[] = {
	 "in", "out"
	};

	register_action("DateTimeTest", this, &my_server::DateTimeTest, kDateTimeTestParameterNames);

	const char* kForceStopParameterNames[] = {
		"out"
	};
	
	register_action("ForceStop", this, &my_server::ForceStop, kForceStopParameterNames);

	const char* kPairTestParameterNames[] = {
		"in", "out"
	};

	zeep::xml::struct_serializer_impl<pair<int,int> >::set_struct_name("pair_of_ints");
	
	register_action("PairTest", this, &my_server::PairTest, kPairTestParameterNames);
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
	out.count = 3;

	WSSearchNS::Hit h = {};

	h.db = "sprot";
	h.id = "104k_thepa";
	h.title = "bla bla bla";
	h.score = 1.0f;
	h.v_ptime = boost::posix_time::microsec_clock::universal_time();
	
	out.hits.push_back(h);

	h.db = "sprot";
	h.id = "108_lyces";
	h.title = "aap <&> noot mies";
	h.score = 0.8f;
	h.opt_text = "Hallóóów";
	
	out.hits.push_back(h);

	h.db = "param";
	h.id = "param-id";
	h.title = m_param;
	h.score = 0.6f;
	h.opt_text.reset();
	
	out.hits.push_back(h);
}

void my_server::PairTest(const string& in, pair<int,int>& out)
{
	out.first = out.second = 1;
}

void my_server::DateTimeTest(
	const boost::posix_time::ptime&
								in,
	boost::posix_time::ptime&	out)
{
	log() << in;
	out = in;
}

void my_server::ForceStop(
	string&						out)
{
	exit(1);
}

int main(int argc, const char* argv[])
{
#if SOAP_SERVER_HAS_PREFORK
 	for (;;)
 	{
 		cout << "restarting server" << endl;
 		
	    sigset_t new_mask, old_mask;
	    sigfillset(&new_mask);
	    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
	
		zeep::http::preforked_server<my_server> server("bla bla");
		boost::thread t(
			boost::bind(&zeep::http::preforked_server<my_server>::run, &server, "0.0.0.0", 10333, 2));
		server.start();
		
	    pthread_sigmask(SIG_SETMASK, &old_mask, 0);
	
		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGHUP);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		sigaddset(&wait_mask, SIGCHLD);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
		int sig = 0;
		sigwait(&wait_mask, &sig);
	    pthread_sigmask(SIG_SETMASK, &old_mask, 0);
	
		server.stop();
		t.join();
		
		if (sig == SIGCHLD)
		{
			int status, pid;
			pid = waitpid(-1, &status, WUNTRACED);
	
			if (pid != -1 and WIFSIGNALED(status))
				cout << "child " << pid << " terminated by signal " << WTERMSIG(status) << endl;
			continue;
		}
		
		if (sig == SIGHUP)
			continue;
		
		break;
 	}
#elif defined(_MSC_VER)

	my_server server("blabla");
	server.bind("0.0.0.0", 10333);
    boost::thread t(boost::bind(&my_server::run, &server, 2));
	t.join();

#else
	for (;;) {
		sigset_t new_mask, old_mask;
		sigfillset(&new_mask);
		pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

		my_server server("blabla");
		server.bind("0.0.0.0", 10333);
		boost::thread t(boost::bind(&my_server::run, &server, 2));

		pthread_sigmask(SIG_SETMASK, &old_mask, 0);

		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		sigaddset(&wait_mask, SIGHUP);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
		int sig = 0;
		sigwait(&wait_mask, &sig);
	
		server.stop();
		t.join();

		if (sig == SIGHUP) {
			cout << "restarting server" << endl;
			continue;
		}

		break;
	}
#endif
	return 0;
}
