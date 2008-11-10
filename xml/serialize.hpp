#ifndef XML_SERIALIZE_H
#define XML_SERIALIZE_H

#include "xml/node.hpp"
#include "xml/exception.hpp"

#include <sstream>

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

template<typename T1>
class serializer
{
  public:
				
				serializer(
					const char*		name)
					: name(name) {}
				
	void		operator()(
					const T1&		value,
					node_ptr		node);

  private:
	const char*	name;
};

template<>
void serializer<int>::operator()(
	const int&			value,
	node_ptr			node)
{
	std::stringstream s;
	s << value;
	node_ptr n(new xml::node(name));
	n->content(s.str());
	node->add_child(n);
}

template<>
void serializer<unsigned int>::operator()(
	const unsigned int&	value,
	node_ptr			node)
{
	std::stringstream s;
	s << value;
	node_ptr n(new xml::node(name));
	n->content(s.str());
	node->add_child(n);
}

}

#endif
