//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_SERIALIZE_H
#define SOAP_XML_SERIALIZE_H

#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <cassert>

#include <zeep/xml/node.hpp>
#include <zeep/exception.hpp>

#include <boost/foreach.hpp>
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

/// \def SOAP_XML_ADD_ENUM(e,v)
/// \brief A macro to add the name of an enum value to the serializer
///
/// To be able to correctly use enum values in a WSDL file or when serializing,
/// you have to specify the enum values.
///
/// E.g., if you have a struct name Algorithm with values 'vector', 'dice' and 'jaccard'
/// you would write:
///
///>	enum Algorithm { vector, dice, jaccard };
///>	SOAP_XML_ADD_ENUM(Algorithm, vector);
///>	SOAP_XML_ADD_ENUM(Algorithm, dice);
///>	SOAP_XML_ADD_ENUM(Algorithm, jaccard);
///
/// An alternative (better?) way to do this is:
///
///>	zeep::xml::enum_map<Algorithm>::instance("Algorithm").add_enum()
///>		("vector", vector)
///>		("dice", dice)
///>		("jaccard", jaccard);

/// \def SOAP_XML_SET_STRUCT_NAME(s)
/// \brief A macro to assign a name to a struct used in serialization.
///
/// By default, libzeep uses the typeid(s).name() as the name for an element.
/// That's often not what is intented. Calling this macro will make sure
/// the type name you used in your code will be used instead.
///
/// E.g., struct FindResult { ... } might end up with a fancy name in the
/// WSDL. To use FindResult instead, call SOAP_XML_SET_STRUCT_NAME(FindResult);
///
/// An alternative it to call, which allows different WSDL and struct names:
/// zeep::xml::serialize_struct<FindResult>::set_struct_name("FindResult");

namespace zeep { namespace xml {

#ifndef LIBZEEP_DOXYGEN_INVOKED
const std::string kPrefix = "ns";
#endif

///	All serializers and deserializers work on an object that contains
///	a pointer to the node just above the actual node holding their data. If any.
///
///	This means for the serializer that it has to create a new node, set the content
///	and add it to the passed in node.
///	For deserializers this means looking up the first child matching the name
///	in the passed-in node to fetch the data from.

struct serializer
{
					serializer(element* node, bool make_node = true)
						: m_node(node)
						, m_make_node(make_node) {}

	template<typename T>
	serializer&		operator&(const boost::serialization::nvp<T>& rhs);

	element*		m_node;
	bool			m_make_node;
};

struct deserializer
{
					deserializer(container* node) : m_node(node) {}

	template<typename T>
	deserializer&	operator&(const boost::serialization::nvp<T>& rhs);

	container*		m_node;
};

#ifndef LIBZEEP_DOXYGEN_INVOKED
typedef std::map<std::string,element*> type_map;
#endif

/// wsdl_creator is used by zeep::dispatcher to create WSDL files.

struct wsdl_creator
{
					wsdl_creator(type_map& types, element* node, bool make_node = true)
						: m_node(node), m_types(types), m_make_node(make_node) {}
	
	template<typename T>
	wsdl_creator&	operator&(const boost::serialization::nvp<T>& rhs);	

	element*		m_node;
	type_map&		m_types;
	bool			m_make_node;
};

#ifndef LIBZEEP_DOXYGEN_INVOKED

// The actual (de)serializers:

// arithmetic types are ints, doubles, etc... simply use lexical_cast to convert these
template<typename T, int S = sizeof(T), bool = boost::is_unsigned<T>::value> struct arithmetic_wsdl_name {};

template<typename T> struct arithmetic_wsdl_name<T, 1, false> {
	static const char* type_name() { return "xsd:byte"; } 
};
template<typename T> struct arithmetic_wsdl_name<T, 1, true> {
	static const char* type_name() { return "xsd:unsignedByte"; } 
};
template<typename T> struct arithmetic_wsdl_name<T, 2, false> {
	static const char* type_name() { return "xsd:short"; } 
};
template<typename T> struct arithmetic_wsdl_name<T, 2, true> {
	static const char* type_name() { return "xsd:unsignedShort"; } 
};
template<typename T> struct arithmetic_wsdl_name<T, 4, false> {
	static const char* type_name() { return "xsd:int"; } 
};
template<typename T> struct arithmetic_wsdl_name<T, 4, true> {
	static const char* type_name() { return "xsd:unsignedInt"; } 
};
template<typename T> struct arithmetic_wsdl_name<T, 8, false> {
	static const char* type_name() { return "xsd:long"; } 
};
template<typename T> struct arithmetic_wsdl_name<T, 8, true> {
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
	
	static void	serialize(element* node, const std::string& name, T& v)
				{
					node->set_attribute(name, boost::lexical_cast<std::string>(v));
				}

	static void	serialize(element* parent, const std::string& name, T& v, bool)
				{
					element* n(new element(name));
					n->content(boost::lexical_cast<std::string>(v));
					parent->append(n);
				}

	static void	deserialize(const std::string& s, T& v)
				{
					v = boost::lexical_cast<T>(s);
				}

	static void	deserialize(element& n, T& v)
				{
					v = boost::lexical_cast<T>(n.content());
				}

	static element*
				 to_wsdl(type_map& types, element* parent,
					const std::string& name, T& v, bool)
				{
					element* n(new element("xsd:element"));
					n->set_attribute("name", name);
					n->set_attribute("type", wsdl_name::type_name());
					n->set_attribute("minOccurs", "1");
					n->set_attribute("maxOccurs", "1");
//					n->set_attribute("default", boost::lexical_cast<std::string>(v));
					parent->append(n);
					
					return n;
				}
};

struct serialize_string
{
	static void	serialize(element* node, const std::string& name, const std::string& v)
				{
					node->set_attribute(name, v);
				}

	static void	serialize(element* parent, const std::string& name, const std::string& v, bool)
				{
					element* n(new element(name));
					n->content(v);
					parent->append(n);
				}

	static void	deserialize(const std::string& s, std::string& v)
				{
					v = s;
				}

	static void	deserialize(element& n, std::string& v)
				{
					v = n.content();
				}

	static element*
				to_wsdl(type_map& types, element* parent,
					const std::string& name, std::string& v, bool)
				{
					element* n(new element("xsd:element"));
					n->set_attribute("name", name);
					n->set_attribute("type", "xsd:string");
					n->set_attribute("minOccurs", "1");
					n->set_attribute("maxOccurs", "1");
//					n->set_attribute("default", v);
					parent->append(n);
					
					return n;
				}
};

struct serialize_bool
{
	static void	serialize(element* node, const std::string& name, bool v)
				{
					node->set_attribute(name, v ? "true" : "false");
				}

	static void	serialize(element* parent, const std::string& name, bool v, bool)
				{
					element* n(new element(name));
					n->content(v ? "true" : "false");
					parent->append(n);
				}

	static void	deserialize(const std::string& s, bool& v)
				{
					v = s == "true" or s == "1";
				}

	static void	deserialize(element& n, bool& v)
				{
					deserialize(n.content(), v);
				}

	static element*
				to_wsdl(type_map& types, element* parent,
					const std::string& name, bool v, bool)
				{
					element* n(new element("xsd:element"));
					n->set_attribute("name", name);
					n->set_attribute("type", "xsd:boolean");
					n->set_attribute("minOccurs", "1");
					n->set_attribute("maxOccurs", "1");
//					n->set_attribute("default", v ? "true" : "false");
					parent->append(n);
					
					return n;
				}
};

template<class Stream, class Struct>
inline void serialize(Stream& stream, Struct& data, const unsigned int)
{
	data.serialize(stream, 0U);
}

template<typename T>
struct serialize_struct
{
	static std::string	s_struct_name;
	
	static void	serialize(element* parent, const std::string& name, T& v)
				{
					throw std::runtime_error("invalid serialization request");
				}

	static void	serialize(element* parent, const std::string& name, T& v, bool make_node)
				{
					if (make_node)
					{
						element* n(new element(name));
						serializer sr(n);
						zeep::xml::serialize(sr, v, 0U);
						parent->append(n);
					}
					else
					{
						serializer sr(parent);
						zeep::xml::serialize(sr, v, 0U);
					}
				}

	static void	deserialize(const std::string& s, T& v)
				{
					throw std::runtime_error("invalid deserialization request");
				}

	static void	deserialize(element& n, T& v)
				{
					deserializer ds(&n);
					zeep::xml::serialize(ds, v, 0U);
				}

	static element*
				 to_wsdl(type_map& types, element* parent,
					const std::string& name, T& v, bool make_node)
				{
					if (make_node)
					{
						element* result(new element("xsd:element"));
						result->set_attribute("name", name);
						result->set_attribute("type", kPrefix + ':' + s_struct_name);
						result->set_attribute("minOccurs", "1");
						result->set_attribute("maxOccurs", "1");
						parent->append(result);
	
						// we might be known already
						if (types.find(s_struct_name) != types.end())
							return result;
	
						element* n(new element("xsd:complexType"));
						n->set_attribute("name", s_struct_name);
						types[s_struct_name] = n;
						
						element* sequence(new element("xsd:sequence"));
						n->append(sequence);
						
						wsdl_creator wsdl(types, sequence);
						zeep::xml::serialize(wsdl, v, 0U);
						
						return result;
					}
					else
					{
						wsdl_creator wc(types, parent);
						zeep::xml::serialize(wc, v, 0U);
						return parent;
					}
				}
	
	static void	set_struct_name(const std::string& name)
				{
					s_struct_name = name;
				}
};

template<typename T>
std::string serialize_struct<T>::s_struct_name = typeid(T).name();

#endif

#define SOAP_XML_SET_STRUCT_NAME(s)	zeep::xml::serialize_struct<s>::s_struct_name = BOOST_PP_STRINGIZE(s);

#ifndef LIBZEEP_DOXYGEN_INVOKED

template<typename T, typename C = std::vector<T> >
struct serialize_container
{
	static void	serialize(element* parent, const std::string& name, C& v)
				{
					throw std::runtime_error("invalid serialization request");
				}
	static void	serialize(element* parent, const std::string& name, C& v, bool);

	static void	deserialize(const std::string& s, C& v)
				{
					throw std::runtime_error("invalid deserialization request");
				}

	static void	deserialize(element& n, C& v);

	static element*
				to_wsdl(type_map& types, element* parent,
					const std::string& name, C& v, bool);
};

template<typename T>
struct enum_map
{
	typedef typename std::map<T,std::string>	name_mapping_type;
	
	name_mapping_type							m_name_mapping;
	std::string									m_name;
	
	static enum_map&
				instance(const char* name = NULL)
				{
					static enum_map s_instance;
					if (name and s_instance.m_name.empty())
						s_instance.m_name = name;
					return s_instance;
				}

	class add_enum_helper
	{
		friend struct enum_map;
					add_enum_helper(name_mapping_type& mapping)
						: m_mapping(mapping) {}
		
		name_mapping_type&
					m_mapping;

	  public:
		add_enum_helper&
					operator()(const std::string& name, T value)
					{
						m_mapping[value] = name;
						return *this;
					}
	};
	
	add_enum_helper	add_enum()
					{
						return add_enum_helper(m_name_mapping);
					}
};

#endif

#define SOAP_XML_ADD_ENUM(e,v)	zeep::xml::enum_map<e>::instance(BOOST_PP_STRINGIZE(e)).m_name_mapping[v] = BOOST_PP_STRINGIZE(v);

#ifndef LIBZEEP_DOXYGEN_INVOKED

template<typename T>
struct serialize_enum
{
	typedef enum_map<T>					t_enum_map;
	typedef std::map<T,std::string>		t_map;
	
	static void	serialize(element* node, const std::string& name, T& v)
				{
					node->set_attribute(name, t_enum_map::instance().m_name_mapping[v]);
				}

	static void	serialize(element* parent, const std::string& name, T& v, bool)
				{
					element* n(new element(name));
					n->content(t_enum_map::instance().m_name_mapping[v]);
					parent->append(n);
				}

	static void	deserialize(const std::string& s, T& v)
				{
					t_map& m = t_enum_map::instance().m_name_mapping;
					for (typename t_map::iterator e = m.begin(); e != m.end(); ++e)
					{
						if (e->second == s)
						{
							v = e->first;
							break;
						}
					}
				}

	static void	deserialize(element& n, T& v)
				{
					deserialize(n.content(), v);
				}

	static element*
				to_wsdl(type_map& types, element* parent, const std::string& name, T& v, bool)
				{
					std::string type_name = t_enum_map::instance().m_name;
					
					element* result(new element("xsd:element"));
					result->set_attribute("name", name);
					result->set_attribute("type", kPrefix + ':' + type_name);
					result->set_attribute("minOccurs", "1");
					result->set_attribute("maxOccurs", "1");
//					result->set_attribute("default", t_enum_map::instance().m_name_mapping[v]);
					parent->append(result);
					
					// we might be known already
					if (types.find(type_name) != types.end())
						return result;
										
					element* n(new element("xsd:simpleType"));
					n->set_attribute("name", type_name);
					types[type_name] = n;
					
					element* restriction(new element("xsd:restriction"));
					restriction->set_attribute("base", "xsd:string");
					n->append(restriction);
					
					t_map& m = t_enum_map::instance().m_name_mapping;
					for (typename t_map::iterator e = m.begin(); e != m.end(); ++e)
					{
						element* en(new element("xsd:enumeration"));
						en->set_attribute("value", e->second);
						restriction->append(en);
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

	enum { is_container = false };
};

template<>
struct serialize_type<bool>
{
	typedef serialize_bool		type;

	enum { is_container = false };
};

template<>
struct serialize_type<std::string>
{
	typedef serialize_string	type;

	enum { is_container = false };
};

template<typename T>
struct serialize_type<std::vector<T> >
{
	typedef serialize_container<T, std::vector<T> > type;

	enum { is_container = true };
};

template<typename T>
struct serialize_type<std::list<T> >
{
	typedef serialize_container<T, std::list<T> >	 type;

	enum { is_container = true };
};

// and the wrappers for serializing

template<typename T>
serializer& serializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename serialize_type<T>::type	s_type;

	if (rhs.name()[0] == '@')
		s_type::serialize(m_node, rhs.name() + 1, rhs.value());
	else
		s_type::serialize(m_node, rhs.name(), rhs.value(), m_make_node);

	return *this;
}

template<typename T>
deserializer& deserializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;

	if (serialize_type<value_type>::is_container)
	{
		BOOST_FOREACH (element* e, *m_node)
		{
			if (e->name() == rhs.name())
				s_type::deserialize(*e, rhs.value());
		}
	}
	else if (rhs.name()[0] == '@' and dynamic_cast<element*>(m_node) != nullptr)
		s_type::deserialize(static_cast<element*>(m_node)->get_attribute(rhs.name() + 1), rhs.value());
	else
	{
		element* n = m_node->find_first(rhs.name());
		if (n)
			s_type::deserialize(*n, rhs.value());
	}

	return *this;
}

template<typename T>
wsdl_creator& wsdl_creator::operator&(const boost::serialization::nvp<T>& rhs)
{
	typedef typename serialize_type<T>::type	s_type;

	s_type::to_wsdl(m_types, m_node, rhs.name(), rhs.value(), m_make_node);

	return *this;
}

// and some deferred implementations for the container types

template<typename T, typename C>
void serialize_container<T,C>::serialize(
	element* parent, const std::string& name, C& v, bool)
{
	typedef typename serialize_type<T>::type		s_type;
	
	for (typename C::iterator i = v.begin(); i != v.end(); ++i)
		s_type::serialize(parent, name, *i, true);
}

template<typename T, typename C>
void serialize_container<T,C>::deserialize(element& n, C& v)
{
	typedef typename serialize_type<T>::type		s_type;
	
	T e;
	s_type::deserialize(n, e);
	v.push_back(e);
}

template<typename T, typename C>
element* serialize_container<T,C>::to_wsdl(type_map& types,
	element* parent, const std::string& name, C& v, bool)
{
	typedef typename serialize_type<T>::type		s_type;
	
	T e;
	element* result = s_type::to_wsdl(types, parent, name, e, true);

	result->remove_attribute("minOccurs");
	result->set_attribute("minOccurs", "0");
	
	result->remove_attribute("maxOccurs");
	result->set_attribute("maxOccurs", "unbounded");
	
	result->remove_attribute("default");
		
	return result;
}

#endif

}
}

#endif
