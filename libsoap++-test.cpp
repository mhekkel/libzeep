#include <fstream>
#include <sstream>
#include <vector>

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

	struct FindResponse
	{
		int				count;
	};

	void				Find(
							const string&			db,
							const vector<string>&	queryterms,
							FindResponse&			out);
};

void my_server::Find(
	const string&			db,
	const vector<string>&	queryterms,
	FindResponse&			out)
{
	cout << "Find: " << db << endl;
	copy(queryterms.begin(), queryterms.end(), ostream_iterator<string>(cout, ", "));
	cout << endl;
	out.count = 1;
}

namespace xml
{

template<>
void serializer<my_server::FindResponse>::operator()(
	const my_server::FindResponse&	res,
	xml::node_ptr					msgbody)
{
	serializer<int> s1("count");
	s1(res.count, msgbody);
}

}

my_server::my_server()
{
	register_soap_call<void(const string&, const vector<string>&, FindResponse&)>(
		"Find", &my_server::Find, "db", "queryterms");
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
	
//	
//	xml::node_ptr n(new xml::node());
//	
//	for (xml::node::iterator child = n->begin(); child != n->end(); ++child)
//	{
//		cout << child->name() << endl;
//		
//		for (xml::node::attribute_iterator a = n->attribute_begin(); a != n->attribute_end(); ++a)
//			cout << "attr: " << a->name() << ", value: " << a->value() << endl;
//	}

	return 0;
}
