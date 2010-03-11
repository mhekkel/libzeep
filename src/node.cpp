//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <vector>

#include "zeep/xml/node.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

struct node_content_imp
{
	void			add_content(const string& text);
	void			add_child(node_ptr child);

	void			write(ostream& os, int level);
	
	bool			empty() const			{ return m_order.empty(); }

	const node_list&
					children() const		{ return m_children; }

	string			content() const;

	attribute_list	m_attributes;
	list<string>	m_content;
	node_list		m_children;
	vector<bool>	m_order;
};

void node_content_imp::add_content(const string& text)
{
	m_content.push_back(text);
	m_order.push_back(true);
}

void node_content_imp::add_child(node_ptr child)
{
	m_children.push_back(child);
	m_order.push_back(false);
}

void node_content_imp::write(ostream& os, int level)
{
	assert(m_order.size() == m_children.size() + m_content.size());

	node_list::iterator child = m_children.begin();
	list<string>::iterator content = m_content.begin();
	
	for (vector<bool>::iterator selector = m_order.begin(); selector != m_order.end(); ++selector)
	{
		if (*selector)	// next is text
		{
			string text = *content;
			
			ba::replace_all(text, "&", "&amp;");
			ba::replace_all(text, "<", "&lt;");
			ba::replace_all(text, ">", "&gt;");
			ba::replace_all(text, "\"", "&quot;");
			ba::replace_all(text, "\n", "&#10;");
			ba::replace_all(text, "\r", "&#13;");
			ba::replace_all(text, "\t", "&#9;");
			
			os << text;
			
			++content;
		}
		else
		{
			child->write(os, level + 1);
			++child;
		}
	}
}

string node_content_imp::content() const
{
	string result;
	
	assert(m_order.size() == m_children.size() + m_content.size());

	node_list::const_iterator child = m_children.begin();
	list<string>::const_iterator content = m_content.begin();
	
	for (vector<bool>::const_iterator selector = m_order.begin(); selector != m_order.end(); ++selector)
	{
		if (*selector)	// next is text
			result += *content++;
		else
		{
			result += ' ';
			++child;
		}
	}
	
	return result;
}

node::node()
	: m_parent(NULL)
	, m_content(new node_content_imp)
{
}

node::node(
	const std::string&	name)
	: m_name(name)
	, m_parent(NULL)
	, m_content(new node_content_imp)
{
}

node::node(
	const std::string&	name,
	const std::string&	prefix)
	: m_name(name)
	, m_prefix(prefix)
	, m_parent(NULL)
	, m_content(new node_content_imp)
{
}

node::node(
	const std::string&	name,
	const std::string&	ns,
	const std::string&	prefix)
	: m_name(name)
	, m_ns(ns)
	, m_prefix(prefix)
	, m_parent(NULL)
	, m_content(new node_content_imp)
{
}

node::~node()
{
	delete m_content;
}

node_list& node::children()
{
	return m_content->m_children;
}

const node_list& node::children() const
{
	return m_content->m_children;
}

attribute_list& node::attributes()
{
	return m_content->m_attributes;
}

const attribute_list& node::attributes() const
{
	return m_content->m_attributes;
}

string node::content() const
{
	return m_content->content();
}

void node::add_attribute(
	attribute_ptr		attr)
{
	m_content->m_attributes.push_back(attr);
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
	m_content->m_attributes.remove_if(boost::bind(&attribute::name, _1) == name);
}

void node::add_child(
	node_ptr			node)
{
	m_content->add_child(node);
	node->m_parent = this;
}

void node::add_content(
	const char*			text,
	unsigned long		length)
{
	assert(text);
	string data(text, length);
	add_content(data);
}

void node::add_content(
	const string&		text)
{
	m_content->add_content(text);
}

node_ptr node::find_first_child(
	const string&		name) const
{
	node_ptr result;

	const node_list& children = m_content->children();

	node_list::const_iterator iter = find_if(
		children.begin(), children.end(),
		boost::bind(&node::name, _1) == name);
	
	if (iter != children.end())
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

	for (attribute_list::const_iterator attr = attributes().begin(); attr != attributes().end(); ++attr)
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
	
	for (attribute_list::const_iterator attr = m_content->m_attributes.begin(); attr != m_content->m_attributes.end(); ++attr)
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
//	for (int i = 0; i < level; ++i)
//		stream << ' ';
	
	stream << '<';
	
	if (m_prefix.length())
		stream << m_prefix << ':';
	
	stream << m_name;

	if (not m_content->m_attributes.empty())
	{
		for (attribute_list::const_iterator a = attributes().begin(); a != attributes().end(); ++a)
		{
			string value = a->value();
			
			ba::replace_all(value, "&", "&amp;");
			ba::replace_all(value, "<", "&lt;");
			ba::replace_all(value, ">", "&gt;");
			ba::replace_all(value, "\"", "&quot;");
//			ba::replace_all(value, "'", "&apos;");
			ba::replace_all(value, "\t", "&#9;");
			ba::replace_all(value, "\n", "&#10;");
			ba::replace_all(value, "\r", "&#13;");

			stream << ' ' << a->name() << "=\"" << value << '"';
		}
	}

//	if (m_content->empty())
//		stream << "/>";
//	else
//	{
		stream << '>';
		m_content->write(stream, level);
		stream << "</" << m_name << '>';
//	}
}

ostream& operator<<(ostream& lhs, const node& rhs)
{
	rhs.write(lhs, 0);
	return lhs;
}

void attribute_list::sort()
{
	m_attributes.sort(
		boost::bind(&attribute::name,
			boost::bind(&attribute_ptr::get, _1)) <
		boost::bind(&attribute::name,
			boost::bind(&attribute_ptr::get, _2)));
}

attribute_ptr make_attribute(const string& name, const string& value)
{
	return attribute_ptr(new attribute(name, value));
}

bool operator==(const node& lhs, const node& rhs)
{
	bool result = lhs.name() == rhs.name();

	if (result and lhs.children().empty())
		result = lhs.content() == rhs.content();

	if (result)
		result = lhs.attributes() == rhs.attributes();

	if (result)
		result = lhs.children() == rhs.children();

	return result;
}

bool operator==(const node_list& lhs, const node_list& rhs)
{
	bool result = true;
	
	node_list::const_iterator lci = lhs.begin();
	node_list::const_iterator rci = rhs.begin();
		
	for (; result and lci != lhs.end() and rci != rhs.end(); ++lci, ++rci)
		result = *lci == *rci;

	if (result)
		result = lci == lhs.end();

	if (result)
		result = rci == rhs.end();

	return result;
}

bool operator==(const attribute_list& lhs, const attribute_list& rhs)
{
	bool result = true;
	
	attribute_list::iterator lai, rai;
	for (lai = lhs.begin(), rai = rhs.begin(); result and lai != lhs.end() and rai != rhs.end(); ++lai, ++rai)
		result = *lai == *rai;

	if (result)
		result = (lai == lhs.end());
	
	if (result)
		result = (rai == rhs.end());
	
	return result;
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
