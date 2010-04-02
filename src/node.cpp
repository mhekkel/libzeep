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
		
		result = m_name == a->m_name and
				 m_value == a->m_value and
				 m_prefix == a->m_prefix;
	}
	return result;
}

// --------------------------------------------------------------------
// element

element::~element()
{
	delete m_child;
	delete m_attribute;
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
		add(new text(s));
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
				child->m_next = next->m_next;
				if (child->m_next != nil)
					child->m_next->m_prev = child;
				
				delete next;
			}
			else
				child = next;
		}
	}
}

void element::add(node_ptr n)
{
	n->m_parent = this;
	n->m_next = nil;

	if (m_child == nil)
	{
		m_child = n;
		n->m_prev = nil;
	}
	else
	{
		node* child = m_child;
		while (child->m_next != nil)
			child = child->m_next;
		child->m_next = n;
		n->m_prev = child;
	}
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
	{
		node* child = m_child;
		while (child->m_next != nil and child->m_next != n)
			child = child->m_next;
		
		assert(child != nil);
		assert(child->m_next == n);
		
		if (child != nil and child->m_next == n)
		{
			child->m_next = n->m_next;
			if (child->m_next != nil)
				child->m_next->m_prev = child;
		}
	}
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
		add(new text(s));
}

node_set element::children() const
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

string element::get_attribute(const string& name) const
{
	string result;

	for (attribute* attr = m_attribute; attr != nil; attr = static_cast<attribute*>(attr->next()))
	{
		if (attr->name() == name)
		{
			result = attr->value();
			break;
		}
	}

	return result;
}

attribute* element::get_attribute_node(const string& name) const
{
	attribute* attr = m_attribute;

	while (attr != nil)
	{
		if (attr->name() == name)
			break;
		attr = static_cast<attribute*>(attr->next());
	}

	return attr;
}

void element::set_attribute(const string& ns, const string& name, const string& value)
{
	attribute* attr = get_attribute_node(name);
	if (attr != nil)
		attr->value(value);
	else
	{
		attr = new attribute(name, "", value);
		attr->m_next = m_attribute;
		m_attribute = attr;
	}
#pragma message("fix me")
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
		attrs.push_back(make_pair(attr->name(), attr->value()));
		attr = static_cast<attribute*>(attr->next());
	}
	
	if (m_child == nil)
		w.write_empty_element(m_ns, m_name, attrs);
	else
	{
		w.write_start_element(m_ns, m_name, attrs);

		node* child = m_child;
		while (child != nil)
		{
			child->write(w);
			child = child->next();
		}
		
		w.write_end_element(m_ns, m_name);
	}
}

bool element::equals(const node* n) const
{
	bool result = false;
	if (node::equals(n) and
		m_ns == static_cast<const element*>(n)->m_ns and
		m_name == static_cast<const element*>(n)->m_name and
		m_prefix == static_cast<const element*>(n)->m_prefix)
	{
		result = true;
		
		const element* e = static_cast<const element*>(n);
		
		if (m_child != nil and e->m_child != nil)
			result = m_child->equals(e->m_child);
		
		if (result and m_attribute != nil and e->m_attribute != nil)
			result = m_attribute->equals(e->m_attribute);
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
		cout << "element <" << static_cast<const element&>(rhs).name();
		
		const element* e = static_cast<const element*>(&rhs);
		foreach (const attribute* attr, e->attributes())
			cout << ' ' << attr->name() << "=\"" << attr->value() << '"';
		
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
