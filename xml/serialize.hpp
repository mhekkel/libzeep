#ifndef XML_SERIALIZE_H
#define XML_SERIALIZE_H

#include <sstream>

#include "xml/node.hpp"
#include "xml/exception.hpp"

#include <boost/serialization/nvp.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/remove_pointer.hpp>

namespace xml
{

template<typename T1>
class deserializer
{
  public:
				
				deserializer(
					const char*		name)
					: name(name) {}
				
	void		operator()(
					node_ptr		node,
					T1&				value);

	node_ptr	get_node(
					node_ptr		node);

  private:
	const char*	name;
};

template<typename T1>
node_ptr deserializer<T1>::get_node(
	node_ptr			node)
{
	for (node::iterator n = node->begin(); n != node->end(); ++n)
	{
		if (n->name() == name)
			return n->shared_from_this();
	}
	
	throw exception("element not found");
}

template<>
void deserializer<int>::operator()(
	node_ptr			node,
	int&				value)
{
	node = get_node(node);
	std::stringstream s(node->content());
	s >> value;
}

template<>
void deserializer<unsigned int>::operator()(
	node_ptr			node,
	unsigned int&		value)
{
	node = get_node(node);
	std::stringstream s(node->content());
	s >> value;
}

template<>
void deserializer<bool>::operator()(
	node_ptr			node,
	bool&				value)
{
	node = get_node(node);
	value = node->content() == "true" or node->content() == "1";
}

template<>
void deserializer<std::string>::operator()(
	node_ptr			node,
	std::string&		value)
{
	node = get_node(node);
	value = node->content();
}

template<>
void deserializer<std::vector<std::string> >::operator()(
	node_ptr			node,
	std::vector<std::string>&
						value)
{
	for (node::iterator n = node->begin(); n != node->end(); ++n)
	{
		if (n->name() == name)
			value.push_back(n->content());
	}
}

class serializer
{
  public:
				serializer()
					: root(new node())
				{
				}

				serializer(
					node_ptr	root)
					: root(root)
				{
				}

	virtual		~serializer()
				{
				}
	
	node_ptr	root;
};

template<typename T>
struct serialize_pod
{
	static void	serialize(
					serializer&			s,
					const std::string&	name,
					T&					v)
				{
					node_ptr n(new node(name));
					
					std::stringstream ss;
					ss << v;
					n->content(ss.str());
					
					s.root->add_child(n);
				}
};

struct serialize_string
{
	static void	serialize(
					serializer&			s,
					const std::string&	name,
					std::string&		v)
				{
					node_ptr n(new node(name));
					n->content(v);
					s.root->add_child(n);
				}
};

struct serialize_bool
{
	static void	serialize(
					serializer&			s,
					const std::string&	name,
					bool&				v)
				{
					node_ptr n(new node(name));
					n->content(v ? "true" : "false");
					s.root->add_child(n);
				}
};

template<typename T>
struct serialize_struct
{
	static void	serialize(
					serializer&			s,
					const std::string&	name,
					T&					v)
				{
					node_ptr n(new node(name));
					node_ptr saved_root(s.root);
					saved_root->add_child(n);
					s.root = n;
					v.serialize(s, 0);
					s.root = saved_root;
				}
};

template<typename T>
struct serialize_vector
{
	static void	serialize(
					serializer&			s,
					const std::string&	name,
					std::vector<T>&		v);
};

template<typename T, bool>
struct serialize_type
{
	typedef serialize_struct<T>	type;
};

template<typename T>
struct serialize_type<T,true>
{
	typedef serialize_pod<T>	type;
};

template<>
struct serialize_type<std::string,false>
{
	typedef serialize_string	type;
};

template<>
struct serialize_type<bool,boost::is_arithmetic<bool>::value>
{
	typedef serialize_bool		type;
};

template<typename T>
struct serialize_type<std::vector<T>,false>
{
	typedef serialize_vector<T>	type;
};

template<typename T>
struct serialize_type_factory : public serialize_type<T, boost::is_arithmetic<T>::value>
{
};

template<typename T>
serializer&	operator&(serializer& lhs, T& rhs)
{
	typedef typename T::second_type									second_type;
	typedef typename boost::remove_pointer<second_type>::type		value_type;
	typedef typename serialize_type_factory<value_type>::type		serialize_t;
	
	serialize_t::serialize(lhs, rhs.name(), rhs.value());

	return lhs;
}

template<typename T>
void serialize_vector<T>::serialize(
	serializer&			s,
	const std::string&	name,
	std::vector<T>&		v)
{
	typedef typename serialize_type_factory<T>::type		serialize_t;
	
	for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
		serialize_t::serialize(s, name, *i);
}

}

#endif
