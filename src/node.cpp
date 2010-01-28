//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <vector>

#include "zeep/xml/node.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

node::node()
{
	m_children = new node_list;
	m_attributes = new attribute_list;
	m_parent = NULL;
}

node::node(
	const std::string&	name)
	: m_name(name)
	, m_children(new node_list)
	, m_attributes(new attribute_list)
	, m_parent(NULL)
{
}

node::node(
	const std::string&	name,
	const std::string&	prefix)
	: m_name(name)
	, m_prefix(prefix)
	, m_children(new node_list)
	, m_attributes(new attribute_list)
	, m_parent(NULL)
{
}

node::node(
	const std::string&	name,
	const std::string&	ns,
	const std::string&	prefix)
	: m_name(name)
	, m_ns(ns)
	, m_prefix(prefix)
	, m_children(new node_list)
	, m_attributes(new attribute_list)
	, m_parent(NULL)
{
}

node::~node()
{
	delete m_children;
	delete m_attributes;
}

void node::add_attribute(
	attribute_ptr		attr)
{
	m_attributes->push_back(attr);
}

void node::add_attribute(
	const string&		name,
	const string&		value)
{
	attribute_ptr attr(new attribute(name, value));
	add_attribute(attr);
}

void node::remove_attribute(const string& name)
{
	m_attributes->remove_if(boost::bind(&attribute::name, _1) == name);
}

void node::add_child(
	node_ptr			node)
{
	m_children->push_back(node);
	node->m_parent = this;
}

void node::add_content(
	const char*			text,
	unsigned long		length)
{
	assert(text);
	m_content.append(text, length);
}

node_ptr node::find_first_child(
	const string&		name) const
{
	node_ptr result;

	node_list::const_iterator iter = find_if(
		m_children->begin(), m_children->end(),
		boost::bind(&node::name, _1) == name);
	
	if (iter != m_children->end())
	{
		node& n = const_cast<node&>(*iter);
		result = n.shared_from_this();
	}

	return result;
}

// locate node based on path (NO XPATH YET!!!)
node_ptr node::find_child(const string& path) const
{
	node_ptr result;
	
	vector<string> pv;
	ba::split(pv, path, ba::is_any_of("/"));
	
	if (path.size() == 0 or pv.front().length() == 0)
		return result; // ?
	
	for (node_list::const_iterator n = children().begin(); n != children().end(); ++n)
	{
		if (n->name() == pv.front())
		{
			if (pv.size() == 1)
			{
				node& nn = const_cast<node&>(*n);
				result = nn.shared_from_this();
			}
			else
				result = n->find_child(ba::join(boost::make_iterator_range(pv.begin() + 1, pv.end()), "/"));
			break;
		}
	}
	
	return result;
}

// locate nodes based on path (NO XPATH YET!!!)
node_list node::find_all(const string& path) const
{
	node_list result;
	
	vector<string> pv;
	ba::split(pv, path, ba::is_any_of("/"));
	
	if (path.size() == 0 or pv.front().length() == 0)
		return result; // ?
	
	for (node_list::const_iterator n = children().begin(); n != children().end(); ++n)
	{
		if (n->name() == pv.front())
		{
			if (pv.size() == 1)
			{
				node& nn = const_cast<node&>(*n);
				result.push_back(nn.shared_from_this());
			}
			else
			{
				node_list l = n->find_all(ba::join(boost::make_iterator_range(pv.begin() + 1, pv.end()), "/"));
				result.insert(result.end(), l.begin(), l.end());
			}
		}
	}
	
	return result;
}

string node::get_attribute(
	const string&	name) const
{
	string result;

	for (attribute_list::iterator attr = attributes().begin(); attr != attributes().end(); ++attr)
	{
		if (attr->name() == name)
		{
			result = attr->value();
			break;
		}
	}
	
	return result;
}

string node::find_prefix(
	const string&		uri) const
{
	string result;
	bool done = false;
	
	for (attribute_list::const_iterator attr = m_attributes->begin(); attr != m_attributes->end(); ++attr)
	{
		if (attr->value() == uri and ba::starts_with(attr->name(), "xmlns"))
		{
			result = attr->name();
			
			if (result == "xmlns")
				result.clear();
			else if (result.length() > 5 and result[5] == ':')
				result.erase(0, 6);

			done = true;
			break;
		}
	}
	
	if (not done and m_parent != NULL)
		result = m_parent->find_prefix(uri);
	
	return result;
}

void node::write(
	ostream&			stream,
	int					level) const
{
	for (int i = 0; i < level; ++i)
		stream << ' ';
	
	stream << '<';
	
	if (m_prefix.length())
		stream << m_prefix << ':';
	
	stream << m_name;

	if (m_attributes)
	{
		for (attribute_list::const_iterator a = attributes().begin(); a != attributes().end(); ++a)
			stream << ' ' << a->name() << "=\"" << a->value() << '"';
	}

	string cont = m_content;
//	ba::trim(cont);
	
	ba::replace_all(cont, "&", "&amp;");
	ba::replace_all(cont, "<", "&lt;");
	ba::replace_all(cont, ">", "&gt;");

	if (cont.length() or m_children)
		stream << '>';
	
	if (cont.length())
		stream << cont;

	if (not m_children->empty())
	{
		stream << endl;

		for (node_list::const_iterator c = children().begin(); c != children().end(); ++c)
			c->write(stream, level + 1);

		for (int i = 0; i < level; ++i)
			stream << ' ';
	}

	if (cont.length() or m_children)
	{
		stream << "</";
		
		if (m_prefix.length())
			stream << m_prefix << ':';
		
		stream << m_name << ">" << endl;
	}	
	else
		stream << "/>" << endl;
}

ostream& operator<<(ostream& lhs, const node& rhs)
{
	rhs.write(lhs, 0);
	return lhs;
}

attribute_ptr make_attribute(const string& name, const string& value)
{
	return attribute_ptr(new attribute(name, value));
}

node_ptr make_node(const string& name,
	attribute_ptr attr1, attribute_ptr attr2,
	attribute_ptr attr3, attribute_ptr attr4,
	attribute_ptr attr5, attribute_ptr attr6,
	attribute_ptr attr7, attribute_ptr attr8)
{
	node_ptr result(new node(name));
	if (attr1) result->add_attribute(attr1);
	if (attr2) result->add_attribute(attr2);
	if (attr3) result->add_attribute(attr3);
	if (attr4) result->add_attribute(attr4);
	if (attr5) result->add_attribute(attr5);
	if (attr6) result->add_attribute(attr6);
	if (attr7) result->add_attribute(attr7);
	if (attr8) result->add_attribute(attr8);
	return result;
}


} // xml
} // zeep
