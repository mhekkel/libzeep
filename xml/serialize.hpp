#ifndef XML_SERIALIZE_H
#define XML_SERIALIZE_H

#include <sstream>
#include <vector>
#include <map>

#include "xml/node.hpp"
#include "xml/exception.hpp"

#include <boost/mpl/if.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_class.hpp>
//#include <boost/type_traits/is_bool.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/lexical_cast.hpp>

//	Lots of template wizardry here...
//
//	The goal is to make a completely transparent XML serializer/deserializer
//	in order to translate SOAP messages into/out of native C++ types.

namespace xml
{

//	All serializers and deserializers work on an object that contains
//	a pointer to the node just above the actual node holding their data. If any.
//
//	This means for the serializer that it has to create a new node, set the content
//	and add it to the passed in node.
//	For deserializers this means looking up the first child matching the name
//	in the passed-in node to fetch the data from.

struct serializer
{
	node_ptr		m_node;

					serializer(
						node_ptr			node)
						: m_node(node) {}

	template<typename T>
	serializer&		operator&(
						const boost::serialization::nvp<T>&
											rhs);

	template<typename T>
	static void		serialize(
						node_ptr			parent,
						const std::string&	name,
						T&					data);
};

struct deserializer
{
	node_ptr		m_node;

					deserializer(
						node_ptr			node)
						: m_node(node) {}

	template<typename T>
	void			operator()(
						const std::string&	name,
						T&					value);

	template<typename T>
	deserializer&	operator&(
						const boost::serialization::nvp<T>&
											rhs);
};

// The actual (de)serializers:

// arithmetic types are ints, doubles, etc... simply use lexical_cast to convert these
template<typename T>
struct serialize_arithmetic
{
	static void	serialize(
					node_ptr			parent,
					const std::string&	name,
					T&					v,
					bool				)
				{
					node_ptr n(new node(name));
					n->content(boost::lexical_cast<std::string>(v));
					parent->add_child(n);
				}

	static void	deserialize(
					node&				n,
					T&					v)
				{
					v = boost::lexical_cast<T>(n.content());
				}
};

struct serialize_string
{
	static void	serialize(
					node_ptr			parent,
					const std::string&	name,
					std::string&		v,
					bool				)
				{
					node_ptr n(new node(name));
					n->content(v);
					parent->add_child(n);
				}

	static void	deserialize(
					node&				n,
					std::string&		v)
				{
					v = n.content();
				}
};

struct serialize_bool
{
	static void	serialize(
					node_ptr			parent,
					const std::string&	name,
					bool&				v,
					bool				)
				{
					node_ptr n(new node(name));
					n->content(v ? "true" : "false");
					parent->add_child(n);
				}

	static void	deserialize(
					node&				n,
					bool&				v)
				{
					v = n.content() == "true" or n.content() == "1";
				}
};

template<typename T>
struct serialize_struct
{
	static void	serialize(
					node_ptr			parent,
					const std::string&	name,
					T&					v,
					bool				make_node)
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

	static void	deserialize(
					node&				n,
					T&					v)
				{
					deserializer ds(n.shared_from_this());
					v.serialize(ds, 0);
				}
};

template<typename T>
struct serialize_vector
{
	static void	serialize(
					node_ptr			parent,
					const std::string&	name,
					std::vector<T>&		v,
					bool				);

	static void	deserialize(
					node&				n,
					std::vector<T>&		v);
};

template<typename T>
struct enum_map
{
	std::map<T,std::string>				m_name_mapping;
	
	static enum_map&
				instance()
				{
					static enum_map s_instance;
					return s_instance;
				}
};

#define XML_ADD_ENUM(e,v)	xml::enum_map<e>::instance().m_name_mapping[v] = BOOST_PP_STRINGIZE(v);

template<typename T>
struct serialize_enum
{
	typedef enum_map<T>					t_enum_map;
	typedef std::map<T,std::string>		t_map;
	
	static void	serialize(
					node_ptr			parent,
					const std::string&	name,
					T&					v,
					bool				)
				{
					node_ptr n(new node(name));
					n->content(t_enum_map::instance().m_name_mapping[v]);
					parent->add_child(n);
				}

	static void	deserialize(
					node&				n,
					T&					v)
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

	enum {
		is_vector = false,
		is_special = true
	};
};

template<>
struct serialize_type<bool>
{
	typedef serialize_bool		type;

	enum {
		is_vector = false,
		is_special = true
	};
};

template<>
struct serialize_type<std::string>
{
	typedef serialize_string	type;

	enum {
		is_vector = false,
		is_special = false
	};
};

template<typename T>
struct serialize_type<std::vector<T> >
{
	typedef serialize_vector<T>	type;

	enum {
		is_vector = true,
		is_special = true
	};
};

// and the wrappers for serializing

template<typename T>
serializer& serializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename serialize_type<T>::type	s_type;

	s_type::serialize(m_node, rhs.name(), rhs.value(), true);

	return *this;
}

template<typename T>
void serializer::serialize(
	node_ptr			parent,
	const std::string&	name,
	T&					data)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type		s_type;
	
	typedef typename serialize_type<value_type>::type s_type;
	s_type::serialize(parent, name, data, false);
}

//template<typename T>
//void serializer::serialize(
//	node_ptr			parent,
//	const std::string&	name,
//	std::vector<T>&		data)
//{
//	serialize_vector<T>::serialize(parent, name, data);
//}

template<typename T>
void deserializer::operator()(
	const std::string&	name,
	T&					value)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type		s_type;

	if (serialize_type<value_type>::is_vector)
	{
		for (node::iterator e = m_node->begin(); e != m_node->end(); ++e)
		{
			if (e->name() == name)
				s_type::deserialize(*e, value);
		}
	}
	else
	{
		node_ptr n = m_node->find_first_child(name);
		if (n)
			s_type::deserialize(*n, value);
	}
}

template<typename T>
deserializer& deserializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;

//	s_type::deserialize(*m_node, rhs.name(), rhs.value());

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

// and some deferred implementations for the vector type

template<typename T>
void serialize_vector<T>::serialize(
	node_ptr			parent,
	const std::string&	name,
	std::vector<T>&		v,
	bool				)
{
	typedef typename serialize_type<T>::type		s_type;
	
	for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
		s_type::serialize(parent, name, *i, true);
}

template<typename T>
void serialize_vector<T>::deserialize(
	node&				n,
	std::vector<T>&		v)
{
	typedef typename serialize_type<T>::type		s_type;
	
	T e;
	s_type::deserialize(n, e);
	v.push_back(e);
}

}

#endif
