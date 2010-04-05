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
#include <typeinfo>

#include "zeep/xml/node.hpp"
#include "zeep/xml/document.hpp"
#include "zeep/xml/writer.hpp"
#include "zeep/exception.hpp"

#define nil NULL

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

node::node()
	: m_parent(nil)
	, m_next(nil)
	, m_prev(nil)
{
}

node::~node()
{
	if (m_next)
		delete m_next;
}

document* node::doc()
{
	document* result = nil;
	if (m_parent != nil)
		result = m_parent->doc();
	return result;
}

const document* node::doc() const
{
	const document* result = nil;
	if (m_parent != nil)
		result = m_parent->doc();
	return result;
}

bool node::equals(const node* n) const
{
	bool result = typeid(this) == typeid(n);

	if (result)
	{
		if (m_next != nil and n->m_next != nil)
			result = m_next->equals(n->m_next);
		else
			result = (m_next == nil and n->m_next == nil);
	}
	
	return result;
}

string node::lang() const
{
	string result;
	if (m_parent != nil)
		result = m_parent->lang();
	return result;
}

void node::append_to_list(node* n)
{
	if (m_next != nil)
		m_next->append_to_list(n);
	else
	{
		m_next = n;
		n->m_prev = this;
		n->m_parent = m_parent;
		n->m_next = nil;
	}
}

void node::remove_from_list(node* n)
{
	assert (this != n);
	if (this == n)
		throw exception("inconsistent node tree");

	if (m_next == n)
	{
		m_next = n->m_next;
		if (m_next != nil)
			m_next->m_prev = m_next;
		n->m_next = n->m_prev = n->m_parent = nil;
	}
	else if (m_next != nil)
		m_next->remove_from_list(n);
	else
		throw exception("remove for a node not found in the list");
}

// --------------------------------------------------------------------
// comment

void comment::write(writer& w) const
{
	w.write_comment(m_text);
}

bool comment::equals(const node* n) const
{
	return
		node::equals(n) and
		dynamic_cast<const comment*>(n) != nil and
		m_text == static_cast<const comment*>(n)->m_text;
}

// --------------------------------------------------------------------
// processing_instruction

void processing_instruction::write(writer& w) const
{
	w.write_processing_instruction(m_target, m_text);
}

bool processing_instruction::equals(const node* n) const
{
	return
		node::equals(n) and
		dynamic_cast<const processing_instruction*>(n) != nil and
		m_text == static_cast<const processing_instruction*>(n)->m_text;
}

// --------------------------------------------------------------------
// text

void text::write(writer& w) const
{
	w.write_text(m_text);
}

bool text::equals(const node* n) const
{
	return
		node::equals(n) and
		dynamic_cast<const text*>(n) != NULL and
		m_text == static_cast<const text*>(n)->m_text;
}

// --------------------------------------------------------------------
// attribute

string attribute::local_name() const
{
	string result = m_qname;
	
	string::size_type d = m_qname.find(':');
	if (d != string::npos)
		result = result.substr(d + 1);

	return result;
}

string attribute::prefix() const
{
	string result = m_qname;
	
	string::size_type d = m_qname.find(':');
	if (d != string::npos)
		result = result.substr(0, d);

	return result;
}

void attribute::write(writer& w) const
{
	assert(false);
}

bool attribute::equals(const node* n) const
{
	bool result = false;
	if (node::equals(n))
	{
		const attribute* a = static_cast<const attribute*>(n);
		
		result = m_qname == a->m_qname and
				 m_value == a->m_value;
	}
	return result;
}

// --------------------------------------------------------------------
// name_space

void name_space::write(writer& w) const
{
	assert(false);
}

bool name_space::equals(const node* n) const
{
	bool result = false;
	if (node::equals(n))
	{
		const name_space* ns = static_cast<const name_space*>(n);
		result = m_prefix == ns->m_prefix and m_uri == ns->m_uri;
	}
	return result;
}

// --------------------------------------------------------------------
// element

element::element(const std::string& qname)
	: m_qname(qname)
	, m_child(nil)
	, m_attribute(nil)
	, m_name_space(nil)
{
}

element::~element()
{
	delete m_child;
	delete m_attribute;
	delete m_name_space;
}

string element::local_name() const
{
	string result = m_qname;
	
	string::size_type d = m_qname.find(':');
	if (d != string::npos)
		result = result.substr(d + 1);

	return result;
}

string element::prefix() const
{
	string result = m_qname;
	
	string::size_type d = m_qname.find(':');
	if (d != string::npos)
		result = result.substr(0, d);

	return result;
}

string element::str() const
{
	string result;
	
	const node* child = m_child;
	while (child != nil)
	{
		result += child->str();
		child = child->next();
	}
	
	return result;
}

string element::content() const
{
	string result;
	
	const node* child = m_child;
	while (child != nil)
	{
		if (dynamic_cast<const text*>(child) != nil)
			result += child->str();
		child = child->next();
	}
	
	return result;
}

void element::content(const string& s)
{
	node* child = m_child;
	
	// find the first text child node
	while (child != nil and dynamic_cast<text*>(child) == nil)
		child = child->next();
	
	// if there was none, add it
	if (child == nil)
		append(new text(s));
	else
	{
		// otherwise, replace its content
		static_cast<text*>(child)->str(s);
		
		// and remove any other text nodes we might have
		while (child->m_next != nil)
		{
			node* next = child->m_next;
			
			if (dynamic_cast<text*>(next) != nil)
			{
				child->remove_from_list(next);
				delete next;
			}
			else
				child = next;
		}
	}
}

void element::append(node_ptr n)
{
	if (m_child == nil)
	{
		m_child = n;
		m_child->m_parent = this;
		m_child->m_next = m_child->m_prev = nil;
	}
	else
		m_child->append_to_list(n);
}

void element::remove(node_ptr n)
{
	assert(n->m_parent == this);
	
	if (m_child == n)
	{
		m_child = m_child->m_next;
		if (m_child != nil)
			m_child->m_prev = nil;
	}
	else
		m_child->remove_from_list(n);
}

void element::add_text(const std::string& s)
{
	node* child = m_child;

	while (child != nil)
		child = child->m_next;

	text* t = dynamic_cast<text*>(child);
	
	if (t != nil)
		t->str(t->str() + s);
	else
		append(new text(s));
}

template<>
node_set element::children<node_set>() const
{
	node_set result;
	
	node* child = m_child;
	while (child != nil)
	{
		result.push_back(child);
		child = child->next();
	}
	
	return result;
}

template<>
element_set element::children<element_set>() const
{
	element_set result;
	
	node* child = m_child;
	while (child != nil)
	{
		if (typeid(*child) == typeid(element))
			result.push_back(static_cast<element*>(child));
		child = child->next();
	}
	
	return result;
}

element* element::find_first_child(const std::string& name)
{
	node* result = m_child;
	while (result != nil)
	{
		if (dynamic_cast<element*>(result) != nil and static_cast<element*>(result)->local_name() == name)
			break;
		result = result->next();
	}
	return static_cast<element*>(result);
}

attribute_set element::attributes() const
{
	attribute_set result;
	
	node* attr = m_attribute;
	while (attr != nil)
	{
		result.push_back(static_cast<attribute*>(attr));
		attr = attr->next();
	}
	
	return result;
}

string element::get_attribute(const string& qname) const
{
	string result;

	for (attribute* attr = m_attribute; attr != nil; attr = static_cast<attribute*>(attr->next()))
	{
		if (attr->qname() == qname)
		{
			result = attr->value();
			break;
		}
	}

	return result;
}

attribute* element::get_attribute_node(const string& qname) const
{
	attribute* attr = m_attribute;

	while (attr != nil)
	{
		if (attr->qname() == qname)
			break;
		attr = static_cast<attribute*>(attr->next());
	}

	return attr;
}

void element::set_attribute(const string& qname, const string& value)
{
	attribute* attr = get_attribute_node(qname);
	if (attr != nil)
		attr->value(value);
	else
	{
		attr = new attribute(qname, value);
		
		if (m_attribute == nil)
		{
			m_attribute = attr;
			m_attribute->m_parent = this;
		}
		else
			m_attribute->append_to_list(attr);
	}
}

void element::remove_attribute(const string& qname)
{
	attribute* n = get_attribute_node(qname);
	
	if (n != nil)
	{
		assert(n->m_parent == this);
		
		if (m_attribute == n)
		{
			m_attribute = static_cast<attribute*>(m_attribute->m_next);
			if (m_attribute != nil)
				m_attribute->m_prev = nil;
		}
		else
			m_attribute->remove_from_list(n);
	}	
}

string element::ns_name_for_prefix(const string& prefix) const
{
	string result;
	
	for (name_space* ns = m_name_space; ns != nil; ns = static_cast<name_space*>(ns->next()))
	{
		if (ns->prefix() == prefix)
		{
			result = ns->uri();
			break;
		}
	}
	
	if (result.empty() and dynamic_cast<element*>(m_parent) != nil)
		result = static_cast<element*>(m_parent)->ns_name_for_prefix(prefix);
	
	return result;
}

string element::prefix_for_ns_name(const string& uri) const
{
	string result;
	
	for (name_space* ns = m_name_space; ns != nil; ns = static_cast<name_space*>(ns->next()))
	{
		if (ns->uri() == uri)
		{
			result = ns->prefix();
			break;
		}
	}
	
	if (result.empty() and dynamic_cast<element*>(m_parent) != nil)
		result = static_cast<element*>(m_parent)->prefix_for_ns_name(uri);
	
	return result;
}

void element::set_name_space(const string& prefix, const string& uri)
{
	name_space* ns;
	for (ns = m_name_space; ns != nil; ns = static_cast<name_space*>(ns->next()))
	{
		if (ns->prefix() == prefix)
		{
			ns->uri(uri);
			break;
		}
	}
	
	if (ns == nil)
	{
		ns = new name_space(prefix, uri);

		if (m_name_space == nil)
		{
			m_name_space = ns;
			m_name_space->m_parent = this;
		}
		else
			m_name_space->append_to_list(ns);
	}
}

string element::lang() const
{
	string result = get_attribute("xml:lang");
	if (result.empty())
		result = node::lang();
	return result;
}

void element::write(writer& w) const
{
	attribute_list attrs;
	
	attribute* attr = m_attribute;
	while (attr != nil)
	{
		attrs.push_back(make_pair(attr->qname(), attr->value()));
		attr = static_cast<attribute*>(attr->next());
	}
	
	name_space* ns = m_name_space;
	while (ns != nil)
	{
		if (ns->prefix().empty())
			attrs.push_back(make_pair("xmlns", ns->uri()));
		else
			attrs.push_back(make_pair(string("xmlns") + ':' + ns->prefix(), ns->uri()));
		ns = static_cast<name_space*>(ns->next());
	}

	if (m_child == nil)
		w.write_empty_element(m_qname, attrs);
	else
	{
		w.write_start_element(m_qname, attrs);

		node* child = m_child;
		while (child != nil)
		{
			child->write(w);
			child = child->next();
		}
		
		w.write_end_element(m_qname);
	}
}

bool element::equals(const node* n) const
{
	bool result = false;
	if (node::equals(n) and
		m_qname == static_cast<const element*>(n)->m_qname)
	{
		result = true;
		
		const element* e = static_cast<const element*>(n);
		
		if (m_child != nil and e->m_child != nil)
			result = m_child->equals(e->m_child);
		else
			result = m_child == nil and e->m_child == nil;
		
		if (result and m_attribute != nil and e->m_attribute != nil)
			result = m_attribute->equals(e->m_attribute);
		else
			result = m_attribute == nil and e->m_attribute == nil;

		if (result and m_name_space != nil and e->m_name_space != nil)
			result = m_name_space->equals(e->m_name_space);
		else
			result = m_name_space == nil and e->m_name_space == nil;

	}

	return result;
}

// --------------------------------------------------------------------
// operator<<

ostream& operator<<(ostream& lhs, const node& rhs)
{
	if (typeid(rhs) == typeid(node))
		cout << "base class???";
	else if (typeid(rhs) == typeid(element))
	{
		cout << "element <" << static_cast<const element&>(rhs).qname();
		
		const element* e = static_cast<const element*>(&rhs);
		foreach (const attribute* attr, e->attributes())
			cout << ' ' << attr->qname() << "=\"" << attr->value() << '"';
		
		cout << '>';
	}
	else if (typeid(rhs) == typeid(comment))
		cout << "comment";
	else if (typeid(rhs) == typeid(processing_instruction))
		cout << "processing_instruction";
	else if (typeid(rhs) == typeid(document))
		cout << "document";
	else
		cout << typeid(rhs).name();
		
	return lhs;
}

} // xml
} // zeep
