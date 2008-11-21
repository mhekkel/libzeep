#include <iostream>
#include <boost/algorithm/string.hpp>
#include <vector>

#include "soap/xml/node.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace soap { namespace xml {

void node::add_attribute(
	attribute_ptr		attr)
{
	if (not m_attributes)
		m_attributes = attr;
	else
	{
		attribute_ptr after = m_attributes;
		while (after->m_next)
			after = after->m_next;
		after->m_next = attr;
	}
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
	if (m_attributes)
	{
		attribute_ptr a = m_attributes;
		while (a->m_next)
		{
			if (a->m_next->m_name == name)
				a->m_next = a->m_next->m_next;
			else
				a = a->m_next;
		}
	}
}

void node::add_child(
	node_ptr			node)
{
	if (not m_children)
		m_children = node;
	else
	{
		node_ptr after = m_children;
		while (after->m_next)
			after = after->m_next;
		after->m_next = node;
	}
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
	node_ptr child = m_children;

	while (child and child->m_name != name)
		child = child->m_next;

	return child;
}

// locate node based on path (NO XPATH YET!!!)
node_ptr node::find_child(const string& path)
{
	node_ptr result(shared_from_this());
	
	vector<string> pv;
	ba::split(pv, path, ba::is_any_of("/"));
	
	if (path.size() == 0)
		return result; // ?

	vector<string>::iterator p = pv.begin();
	if (p->length() == 0)
		++p;
	
	if (p == pv.end())
		return node_ptr();
	
	while (p != pv.end() and result)
	{
		bool found = false;
		for (node::iterator n = result->begin(); n != result->end(); ++n)
		{
			if (n->name() == *p)
			{
				++p;
				result = n->shared_from_this();
				found = true;
				break;
			}
		}
		
		if (not found)
			result = node_ptr();
	}
	
	return result;
}

string node::get_attribute(
	const string&	name)
{
	string result;

	for (node::attribute_iterator attr = attribute_begin(); attr != attribute_end(); ++attr)
	{
		if (attr->name() == name)
		{
			result = attr->value();
			break;
		}
	}
	
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
		for (const_attribute_iterator a = attribute_begin(); a != attribute_end(); ++a)
		{
//			stream << endl;
//
//			for (int i = 0; i < level; ++i)
//				stream << ' ';

			stream << ' ' << a->name() << "=\"" << a->value() << '"';
		}
	}

	string cont = m_content;
	ba::trim(cont);
	
	ba::replace_all(cont, "&", "&amp;");
	ba::replace_all(cont, "<", "&lt;");
	ba::replace_all(cont, ">", "&gt;");

	if (cont.length() or m_children)
		stream << '>';
	
	if (cont.length())
		stream << cont;

	if (m_children)
	{
		stream << endl;

		for (const_iterator c = begin(); c != end(); ++c)
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

} // xml
} // soap
