//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_SERIALIZE_H
#define SOAP_XML_SERIALIZE_H

#include <sstream>
#include <vector>
#include <map>

#include "soap/xml/node.hpp"
#include "soap/exception.hpp"

#include <boost/mpl/if.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/lexical_cast.hpp>

//	Lots of template wizardry here...
//
//	The goal is to make a completely transparent XML serializer/deserializer
//	in order to translate SOAP messages into/out of native C++ types.
//
//	The interface for the code below is compatible with the 'serialize' member
//	function required to use boost::serialization. 

namespace soap { namespace xml {

const std::string kPrefix = "ns";

//	All serializers and deserializers work on an object that contains
//	a pointer to the node just above the actual node holding their data. If any.
//
//	This means for the serializer that it has to create a new node, set the content
//	and add it to the passed in node.
//	For deserializers this means looking up the first child matching the name
//	in the passed-in node to fetch the data from.

struct serializer
{
					serializer(node_ptr node, bool make_node = true)
						: m_node(node)
						, m_make_node(make_node) {}

	template<typename T>
	serializer&		operator&(const boost::serialization::nvp<T>& rhs);

	node_ptr		m_node;
	bool			m_make_node;
};

struct deserializer
{
					deserializer(node_ptr node) : m_node(node) {}

	template<typename T>
	deserializer&	operator&(const boost::serialization::nvp<T>& rhs);

	node_ptr		m_node;
};

typedef std::map<std::string,node_ptr> type_map;

struct wsdl_creator
{
					wsdl_creator(type_map& types, node_ptr node, bool make_node = true)
						: m_node(node), m_types(types) {}
	
	template<typename T>
	wsdl_creator&	operator&(const boost::serialization::nvp<T>& rhs);	

	node_ptr		m_node;
	type_map&		m_types;
};

// The actual (de)serializers:

// arithmetic types are ints, doubles, etc... simply use lexical_cast to convert these
template<typename T> struct arithmetic_wsdl_name { };

template<> struct arithmetic_wsdl_name<int> {
	static const char* type_name() { return "xsd:int"; } 
};
template<> struct arithmetic_wsdl_name<unsigned int> {
	static const char* type_name() { return "xsd:unsignedInt"; } 
};
template<> struct arithmetic_wsdl_name<long int> {
	static const char* type_name() { return "xsd:int"; } 
};
template<> struct arithmetic_wsdl_name<long unsigned int> {
	static const char* type_name() { return "xsd:unsignedInt"; } 
};
template<> struct arithmetic_wsdl_name<long long> {
	static const char* type_name() { return "xsd:long"; } 
};
template<> struct arithmetic_wsdl_name<unsigned long long> {
	static const char* type_name() { return "xsd:unsignedLong"; } 
};
template<> struct arithmetic_wsdl_name<float> {
	static const char* type_name() { return "xsd:float"; } 
};
template<> struct arithmetic_wsdl_name<double> {
	static const char* type_name() { return "xsd:double"; } 
};

template<typename T>
struct serialize_arithmetic
{
	typedef typename boost::remove_const<
			typename boost::remove_reference<T>::type >::type	value_type;
	typedef arithmetic_wsdl_name<value_type>					wsdl_name;
	
	static void	serialize(node_ptr parent, const std::string& name, T& v, bool)
				{
					node_ptr n(new node(name));
					n->content(boost::lexical_cast<std::string>(v));
					parent->add_child(n);
				}

	static void	deserialize(node& n, T& v)
				{
					v = boost::lexical_cast<T>(n.content());
				}

	static node_ptr
				 to_wsdl(type_map& types, node_ptr parent,
					const std::string& name, T& v)
				{
					node_ptr n(new node("xsd:element"));
					n->add_attribute("name", name);
					n->add_attribute("type", wsdl_name::type_name());
					n->add_attribute("minOccurs", "1");
					n->add_attribute("maxOccurs", "1");
//					n->add_attribute("default", boost::lexical_cast<std::string>(v));
					parent->add_child(n);
					
					return n;
				}
};

struct serialize_string
{
	static void	serialize(node_ptr parent, const std::string& name, std::string& v, bool)
				{
					node_ptr n(new node(name));
					n->content(v);
					parent->add_child(n);
				}

	static void	deserialize(node& n, std::string& v)
				{
					v = n.content();
				}

	static node_ptr
				to_wsdl(type_map& types, node_ptr parent,
					const std::string& name, std::string& v)
				{
					node_ptr n(new node("xsd:element"));
					n->add_attribute("name", name);
					n->add_attribute("type", "xsd:string");
					n->add_attribute("minOccurs", "1");
					n->add_attribute("maxOccurs", "1");
//					n->add_attribute("default", v);
					parent->add_child(n);
					
					return n;
				}
};

struct serialize_bool
{
	static void	serialize(node_ptr parent, const std::string& name, bool v, bool)
				{
					node_ptr n(new node(name));
					n->content(v ? "true" : "false");
					parent->add_child(n);
				}

	static void	deserialize(node& n, bool& v)
				{
					v = n.content() == "true" or n.content() == "1";
				}

	static node_ptr
				to_wsdl(type_map& types, node_ptr parent,
					const std::string& name, bool v)
				{
					node_ptr n(new node("xsd:element"));
					n->add_attribute("name", name);
					n->add_attribute("type", "xsd:boolean");
					n->add_attribute("minOccurs", "1");
					n->add_attribute("maxOccurs", "1");
//					n->add_attribute("default", v ? "true" : "false");
					parent->add_child(n);
					
					return n;
				}
};

template<typename T>
struct serialize_struct
{
	static std::string	s_struct_name;
	
	static void	serialize(node_ptr parent, const std::string& name, T& v, bool make_node)
				{
					if (make_node)
					{
						node_ptr n(new node(name));
						serializer sr(n);
						v.serialize(sr, 0);
						parent->add_child(n);
					}
					else
					{
						serializer sr(parent);
						v.serialize(sr, 0);
					}
				}

	static void	deserialize(node& n, T& v)
				{
					deserializer ds(n.shared_from_this());
					v.serialize(ds, 0);
				}

	static node_ptr
				 to_wsdl(type_map& types, node_ptr parent,
					const std::string& name, T& v)
				{
					node_ptr result(new node("xsd:element"));
					result->add_attribute("name", name);
					result->add_attribute("type", kPrefix + ':' + s_struct_name);
					result->add_attribute("minOccurs", "1");
					result->add_attribute("maxOccurs", "1");
					parent->add_child(result);

					// we might be known already
					if (types.find(s_struct_name) != types.end())
						return result;

					node_ptr n(new node("xsd:complexType"));
					n->add_attribute("name", s_struct_name);
					types[name] = n;
					
					node_ptr sequence(new node("xsd:sequence"));
					n->add_child(sequence);
					
					wsdl_creator wsdl(types, sequence);
					v.serialize(wsdl, 0);
					
					return result;
				}
};

template<typename T>
std::string serialize_struct<T>::s_struct_name = typeid(T).name();

#define SOAP_XML_SET_STRUCT_NAME(s)	soap::xml::serialize_struct<s>::s_struct_name = BOOST_PP_STRINGIZE(s);

template<typename T>
struct serialize_vector
{
	static void	serialize(node_ptr parent, const std::string& name, std::vector<T>& v, bool);

	static void	deserialize(node& n, std::vector<T>& v);

	static node_ptr
				to_wsdl(type_map& types, node_ptr parent,
					const std::string& name, std::vector<T>& v);
};

template<typename T>
struct enum_map
{
	std::map<T,std::string>				m_name_mapping;
	std::string							m_name;
	
	static enum_map&
				instance(const char* name = NULL)
				{
					static enum_map s_instance;
					if (name and s_instance.m_name.empty())
						s_instance.m_name = name;
					return s_instance;
				}
};

#define SOAP_XML_ADD_ENUM(e,v)	soap::xml::enum_map<e>::instance(BOOST_PP_STRINGIZE(e)).m_name_mapping[v] = BOOST_PP_STRINGIZE(v);

template<typename T>
struct serialize_enum
{
	typedef enum_map<T>					t_enum_map;
	typedef std::map<T,std::string>		t_map;
	
	static void	serialize(node_ptr parent, const std::string& name, T& v, bool)
				{
					node_ptr n(new node(name));
					n->content(t_enum_map::instance().m_name_mapping[v]);
					parent->add_child(n);
				}

	static void	deserialize(node& n, T& v)
				{
					t_map& m = t_enum_map::instance().m_name_mapping;
					for (typename t_map::iterator e = m.begin(); e != m.end(); ++e)
					{
						if (e->second == n.content())
						{
							v = e->first;
							break;
						}
					}
				}

	static node_ptr
				to_wsdl(type_map& types, node_ptr parent, const std::string& name, T& v)
				{
					std::string type_name = t_enum_map::instance().m_name;
					
					node_ptr result(new node("xsd:element"));
					result->add_attribute("name", name);
					result->add_attribute("type", kPrefix + ':' + type_name);
					result->add_attribute("minOccurs", "1");
					result->add_attribute("maxOccurs", "1");
//					result->add_attribute("default", t_enum_map::instance().m_name_mapping[v]);
					parent->add_child(result);
					
					// we might be known already
					if (types.find(type_name) != types.end())
						return result;
										
					node_ptr n(new node("xsd:simpleType"));
					n->add_attribute("name", type_name);
					types[name] = n;
					
					node_ptr restriction(new node("xsd:restriction"));
					restriction->add_attribute("base", "xsd:string");
					n->add_child(restriction);
					
					t_map& m = t_enum_map::instance().m_name_mapping;
					for (typename t_map::iterator e = m.begin(); e != m.end(); ++e)
					{
						node_ptr en(new node("xsd:enumeration"));
						en->add_attribute("value", e->second);
						restriction->add_child(en);
					}
					
					return result;
				}
};

// now create a type factory for these serializers

template<typename T>
struct serialize_type
{
	typedef typename boost::mpl::if_c<
			boost::is_arithmetic<T>::value,
			serialize_arithmetic<T>,
			typename boost::mpl::if_c<
				boost::is_enum<T>::value,
				serialize_enum<T>,
				serialize_struct<T>
			>::type
		>::type					type;

	enum { is_vector = false };
};

template<>
struct serialize_type<bool>
{
	typedef serialize_bool		type;

	enum { is_vector = false };
};

template<>
struct serialize_type<std::string>
{
	typedef serialize_string	type;

	enum { is_vector = false };
};

template<typename T>
struct serialize_type<std::vector<T> >
{
	typedef serialize_vector<T>	type;

	enum { is_vector = true };
};

// and the wrappers for serializing

template<typename T>
serializer& serializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename serialize_type<T>::type	s_type;

	s_type::serialize(m_node, rhs.name(), rhs.value(), m_make_node);

	return *this;
}

template<typename T>
deserializer& deserializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;

	if (serialize_type<value_type>::is_vector)
	{
		for (node::iterator e = m_node->begin(); e != m_node->end(); ++e)
		{
			if (e->name() == rhs.name())
				s_type::deserialize(*e, rhs.value());
		}
	}
	else
	{
		node_ptr n = m_node->find_first_child(rhs.name());
		if (n)
			s_type::deserialize(*n, rhs.value());
	}

	return *this;
}

template<typename T>
wsdl_creator& wsdl_creator::operator&(const boost::serialization::nvp<T>& rhs)
{
	typedef typename serialize_type<T>::type	s_type;

	s_type::to_wsdl(m_types, m_node, rhs.name(), rhs.value());

	return *this;
}

// and some deferred implementations for the vector type

template<typename T>
void serialize_vector<T>::serialize(
	node_ptr parent, const std::string& name, std::vector<T>& v, bool)
{
	typedef typename serialize_type<T>::type		s_type;
	
	for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
		s_type::serialize(parent, name, *i, true);
}

template<typename T>
void serialize_vector<T>::deserialize(node& n, std::vector<T>& v)
{
	typedef typename serialize_type<T>::type		s_type;
	
	T e;
	s_type::deserialize(n, e);
	v.push_back(e);
}

template<typename T>
node_ptr serialize_vector<T>::to_wsdl(type_map& types,
	node_ptr parent, const std::string& name, std::vector<T>& v)
{
	typedef typename serialize_type<T>::type		s_type;
	
	T e;
	node_ptr result = s_type::to_wsdl(types, parent, name, e);

	result->remove_attribute("minOccurs");
	result->add_attribute("minOccurs", "0");
	
	result->remove_attribute("maxOccurs");
	result->add_attribute("maxOccurs", "unbounded");
	
	result->remove_attribute("default");
		
	return result;
}

}
}

#endif
