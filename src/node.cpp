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
#include "zeep/xml/writer.hpp"
#include "zeep/xml/xpath.hpp"
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

root* node::root_node()
{
	root* result = nil;
	if (m_parent != nil)
		result = m_parent->root_node();
	return result;
}

const root* node::root_node() const
{
	const root* result = nil;
	if (m_parent != nil)
		result = m_parent->root_node();
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
	node* p = this;
	while (p->m_next != nil)
		p = p->m_next;

	p->m_next = n;
	n->m_prev = p;
	n->m_parent = m_parent;
	n->m_next = nil;
}

void node::remove_from_list(node* n)
{
	assert (this != n);
	if (this == n)
		throw exception("inconsistent node tree");

	node* p = this;
	while (p != nil and p->m_next != n)
		p = p->m_next;

	if (p != nil and p->m_next == n)
	{
		p->m_next = n->m_next;
		if (p->m_next != nil)
			p->m_next->m_prev = m_next;
		n->m_next = n->m_prev = n->m_parent = nil;
	}
	else
		throw exception("remove for a node not found in the list");
}

void node::parent(node* n)
{
	assert(m_parent == nil);
	m_parent = n;
}

string node::qname() const
{
	return "";
}

string node::name() const
{
	string qn = qname();
	string::size_type s = qn.find(':');
	if (s != string::npos)
		qn.erase(0, s + 1);
	return qn;
}

string node::prefix() const
{
	string qn = qname();
	string::size_type s = qn.find(':');
	if (s != string::npos)
		qn.erase(s);
	return qn;
}

string node::ns() const
{
	string result, p = prefix();
	if (not p.empty())
		result = namespace_for_prefix(p);
	return result;
}

string node::namespace_for_prefix(const string& prefix) const
{
	string result;
	if (m_parent != nil)
		result = m_parent->namespace_for_prefix(prefix);
	return result;
}

string node::prefix_for_namespace(const string& uri) const
{
	string result;
	if (m_parent != nil)
		result = m_parent->prefix_for_namespace(uri);
	return result;
}

// --------------------------------------------------------------------
// container_node

container::container()
	: m_child(nil)
{
}

container::~container()
{
	delete m_child;
}

template<>
node_set container::children<node>() const
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
element_set container::children<element>() const
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

void container::append(node_ptr n)
{
	n->parent(this);
	
	if (m_child == nil)
	{
		m_child = n;
		m_child->m_next = m_child->m_prev = nil;
	}
	else
		m_child->append_to_list(n);
}

void container::remove(node_ptr n)
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

element_set container::find(const std::string& path) const
{
	return xpath(path).evaluate<element>(*this);
}

element* container::find_first(const std::string& path) const
{
	element_set s = xpath(path).evaluate<element>(*this);
	
	element* result = nil;
	if (not s.empty())
		result = s.front();
	return result;
}

// --------------------------------------------------------------------
// root

root::root()
{
}

root::~root()
{
}

root* root::root_node()
{
	return this;
}

const root*	root::root_node() const
{
	return this;
}

string root::str() const
{
	string result;
	element* e = child_element();
	if (e != nil)
		result = e->str();
	return result;
}

element* root::child_element() const
{
	element* result = nil;

	node* n = m_child;
	while (n != nil and result == nil)
	{
		result = dynamic_cast<element*>(n);
		n = n->next();
	}
	
	return result;
}

void root::child_element(element* child)
{
	element* e = child_element();
	if (e != nil)
	{
		container::remove(e);
		delete e;
	}
	
	container::append(child);
}

void root::write(writer& w) const
{
	node* child = m_child;
	while (child != nil)
	{
		child->write(w);
		child = child->next();
	}
}

bool root::equals(const node* n) const
{
	bool result = false;
	if (typeid(n) == typeid(*this))
	{
		result = true;
		
		const root* e = static_cast<const root*>(n);
		
		if (m_child != nil and e->m_child != nil)
			result = m_child->equals(e->m_child);
		else
			result = m_child == nil and e->m_child == nil;
	}

	return result;
}

void root::append(node* n)
{
	if (dynamic_cast<processing_instruction*>(n) == nil and
		dynamic_cast<comment*>(n) == nil)
	{
		throw exception("can only append comment and processing instruction nodes to a root");
	}
	
	container::append(n);
}

// --------------------------------------------------------------------
// comment

void comment::write(writer& w) const
{
	w.comment(m_text);
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
	w.processing_instruction(m_target, m_text);
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
	w.content(m_text);
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
	, m_attribute(nil)
	, m_name_space(nil)
{
}

element::~element()
{
	delete m_attribute;
	delete m_name_space;
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
		while (child->next() != nil)
		{
			node* next = child->next();
			
			if (dynamic_cast<text*>(next) != nil)
			{
				container::remove(next);
				delete next;
			}
			else
				child = next;
		}
	}
}

void element::add_text(const std::string& s)
{
	text* textNode = nil;
	
	if (m_child != nil)
	{
		node* child = m_child;
	
		while (child->m_next != nil)
			child = child->m_next;
	
		textNode = dynamic_cast<text*>(child);
	}
	
	if (textNode != nil)
		textNode->append(s);
	else
		append(new text(s));
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

name_space_list element::name_spaces() const
{
	name_space_list result;
	
	node* ns = m_name_space;
	while (ns != nil)
	{
		result.push_back(static_cast<name_space*>(ns));
		ns = ns->next();
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
			m_attribute->parent(this);
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

string element::namespace_for_prefix(const string& prefix) const
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
		result = static_cast<element*>(m_parent)->namespace_for_prefix(prefix);
	
	return result;
}

string element::prefix_for_namespace(const string& uri) const
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
		result = static_cast<element*>(m_parent)->prefix_for_namespace(uri);
	
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
			m_name_space->parent(this);
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
	w.start_element(m_qname);

	attribute* attr = m_attribute;
	while (attr != nil)
	{
		w.attribute(attr->qname(), attr->value());
		attr = static_cast<attribute*>(attr->next());
	}
	
	name_space* ns = m_name_space;
	while (ns != nil)
	{
		if (ns->prefix().empty())
			w.attribute("xmlns", ns->uri());
		else
			w.attribute(string("xmlns:") + ns->prefix(), ns->uri());
		ns = static_cast<name_space*>(ns->next());
	}

	node* child = m_child;
	while (child != nil)
	{
		child->write(w);
		child = child->next();
	}

	w.end_element();
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
	else if (typeid(rhs) == typeid(root))
		cout << "root";
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
	else
		cout << typeid(rhs).name();
		
	return lhs;
}

} // xml
} // zeep
