#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>

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

#include <boost/fusion/sequence.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/include/invoke.hpp>
#include <boost/fusion/include/iterator_range.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/pop_front.hpp>
#include <boost/fusion/include/push_front.hpp>


#include <iostream>

#include <getopt.h>

using namespace std;

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

namespace f = boost::fusion;

namespace rs
{
	namespace ft = boost::function_types;
	
	template<typename Iterator>
	struct parameter_deserializer
	{
		typedef Iterator result_type;
		
		xml::node_ptr	m_node;
		
		parameter_deserializer(
			xml::node_ptr	node)
			: m_node(node) {}
		
		template<typename T>
		Iterator operator()(T& t, Iterator i) const
		{
			xml::deserializer d(m_node);
			d(*i++, t);
			return i;
		}
	};
	
	template<typename Function>
	struct handler_param_types;
	
	template<class Class, typename T1, typename R>
	struct handler_param_types<void(Class::*)(T1, R&)>
	{
		typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
		
		typedef typename f::vector<t_1>	type;
	};
	
	template<class Class, typename T1, typename T2, typename R>
	struct handler_param_types<void(Class::*)(T1, T2, R&)>
	{
		typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
		typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;

		typedef typename f::vector<t_1,t_2>	type;
	};

	template<class Class, typename T1, typename T2, typename T3, typename R>
	struct handler_param_types<void(Class::*)(T1, T2, T3, R&)>
	{
		typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
		typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
		typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;

		typedef typename f::vector<t_1,t_2,t_3>	type;
	};
	
	struct handler_base
	{
				handler_base(
					const std::string&	action)
					: m_action(action) {}

		virtual xml::node_ptr
				call(
					xml::node_ptr		in) = 0;
		
		const std::string&
				get_action() const		{ return m_action; }
		
		std::string	m_action;	
	};
	
	template<class Class, typename Function>
	struct handler : public handler_base
	{
		typedef typename handler_param_types<Function>::type	p_t;
		
				handler(
					const char*			action,
					Class*				object,
					Function			method,
					const char**		names)
					: handler_base(action)
					, m_object(object)
					, m_method(method)
					, m_names(names) {}
		
		virtual xml::node_ptr
				call(
					xml::node_ptr		in)
				{
					p_t a;

					boost::fusion::accumulate(a, &m_names[0],
						parameter_deserializer<const char**>(in));
					
					f::invoke(m_method, f::push_front(a, m_object));
					
					return in;
				}

		Class*		m_object;
		Function	m_method;
		const char**
					m_names;
	};
	
	class dispatcher
	{
	  public:
		
		template<
			class Class,
			typename Function
		>
		void	register_action(
					const char*		action,
					Class*			server,
					Function		call,
					const char**	arg)
				{
					typedef typename ft::parameter_types<Function>	Sequence;
					BOOST_STATIC_ASSERT(f::result_of::size<Sequence>::value == 5);
					
//					typedef typename f::result_of::pop_front<Sequence>::type	Arguments;
					
					m_handlers.push_back(new handler<Class,Function>(action, server, call, arg));
				}

		xml::node_ptr
				dispatch(
					const char*		action,
					xml::node_ptr	in)
				{
					std::vector<handler_base*>::iterator cb = find_if(
						m_handlers.begin(), m_handlers.end(),
						boost::bind(&handler_base::get_action, _1) == action);
					return (*cb)->call(in);
				}
		
		std::vector<handler_base*>
				m_handlers;
	};
	
//	template<typename Function>
//	void register_func(
//		Function	f)
//	{
//		invoker<Function> i(f);
//	}
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

	xml::soap::dispatcher
						m_dispatcher;
};

my_server::my_server()
	: m_dispatcher("http://www.hekkelman.com/ws")
{
	m_dispatcher.register_soap_call("Find", this, &my_server::Find,
		"db", "queryterms", "algorithm",
//		"alltermsrequired", "booleanfilter", "resultoffset", "maxresultcount",
		"out");

	rs::dispatcher d2;
	const char* kFindParameterNames[] = {
		"db", "queryterms", "algorithm",
//		"alltermsrequired", "booleanfilter", "resultoffset", "maxresultcount",
		"out"
	};
	
	d2.register_action("Find", this, &my_server::Find, kFindParameterNames);
	
	xml::node_ptr in;
	d2.dispatch("Find", in);
}

void my_server::Find(
	const string&			db,
	const vector<string>&	queryterms,
	const string&			algorithm,
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
		xml::document doc(data);

		xml::soap::envelope env(doc);
		xml::node_ptr req = env.request();
		
		cout << "request:" << endl << *req << endl;
		
		if (req->name() != "Find" or req->ns() != "http://mrs.cmbi.ru.nl/mrsws/search")
			throw xml::exception("Invalid request");

		my_server s;
		
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
