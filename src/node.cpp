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

#include <zeep/xml/node.hpp>
#include <zeep/xml/writer.hpp>
#include <zeep/xml/xpath.hpp>
#include <zeep/exception.hpp>

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

root_node* node::root()
{
	root_node* result = nil;
	if (m_parent != nil)
		result = m_parent->root();
	return result;
}

const root_node* node::root() const
{
	const root_node* result = nil;
	if (m_parent != nil)
		result = m_parent->root();
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

node* node::clone() const
{
	assert(false);
	throw zeep::exception("cannot clone this");
	return nil;
}

string node::lang() const
{
	string result;
	if (m_parent != nil)
		result = m_parent->lang();
	return result;
}

void node::insert_sibling(node* n, node* before)
{
	node* p = this;
	while (p->m_next != nil and p->m_next != before)
		p = p->m_next;

	if (p->m_next != before and before != nil)
		throw zeep::exception("before argument in insert_sibling is not valid");
	
	p->m_next = n;
	n->m_prev = p;
	n->m_parent = m_parent;
	n->m_next = before;
	
	if (before != nil)
		before->m_prev = n;
}

void node::remove_sibling(node* n)
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

void node::parent(container* n)
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

	string p;

	if (s != string::npos)
		p = qn.substr(0, s);

	return p;
}

string node::ns() const
{
	string result, p = prefix();
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
	, m_last(nil)
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
		m_last = m_child = n;
		m_child->m_next = m_child->m_prev = nil;
	}
	else
	{
		m_last->insert_sibling(n, nil);
		m_last = n;
	}
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
		m_child->remove_sibling(n);
	
	if (n == m_last)
	{
		m_last = m_child;
		while (m_last != nil)
			m_last = m_last->m_next;
	}
	
	n->m_next = n->m_prev = n->m_parent = nil;
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

container::size_type container::size() const
{
	size_type result = 0;
	for (node* n = m_child; n != m_last; n = n->m_next)
		++result;
	return result;
}

bool container::empty() const
{
	return m_child == nil;
}

node* container::front() const
{
	return m_child;
}

node* container::back() const
{
	return m_last;
}

void container::swap(container& cnt)
{
	std::swap(m_child, cnt.m_child);
	std::swap(m_last, cnt.m_last);
	for (node* n = m_child; n != m_last; n = n->m_next)
		n->m_parent = this;
	for (node* n = cnt.m_child; n != cnt.m_last; n = n->m_next)
		n->m_parent = &cnt;
}

void container::clear()
{
	delete m_child;
	m_child = m_last = nil;
}

void container::push_front(node* n)
{
	if (n->m_next != nil or n->m_prev != nil)
		throw exception("attempt to insert a node that has next or prev");
	if (n->m_parent != nil)
		throw exception("attempt to insert node that already has a parent");

	n->parent(this);
	n->m_next = m_child;
	if (m_child != nil)
		m_child->m_prev = n;

	m_child = n;
	if (m_last == nil)
		m_last = m_child;
}

void container::pop_front()
{
	if (m_child != nil)
	{
		node* n = m_child;

		m_child = m_child->m_next;
		if (m_child != nil)
			m_child->m_prev = nil;
		
		if (n == m_last)
			m_last = nil;
		
		n->m_next = nil;
		delete n;
	}
}

void container::push_back(node* n)
{
	if (n->m_next != nil or n->m_prev != nil)
		throw exception("attempt to insert a node that has next or prev");
	if (n->m_parent != nil)
		throw exception("attempt to insert node that already has a parent");

	n->parent(this);

	if (m_child == nil)
	{
		m_last = m_child = n;
		m_child->m_next = m_child->m_prev = nil;
	}
	else
	{
		m_last->insert_sibling(n, nil);
		m_last = n;
	}
}

void container::pop_back()
{
	if (m_last != nil)
	{
		if (m_last == m_child)
		{
			delete m_child;
			m_child = m_last = nil;
		}
		else
		{
			node* n = m_last;
			m_last = m_last->m_prev;
			m_last->m_next = nil;
			
			n->m_prev = nil;
			delete n;
		}
	}
}

container::node_iterator container::insert(node* position, node* n)
{
	if (n->m_next != nil or n->m_prev != nil)
		throw exception("attempt to insert a node that has next or prev");
	if (n->m_parent != nil)
		throw exception("attempt to insert node that already has a parent");

	n->parent(this);
	
	if (m_child == position)
	{
		m_last = m_child = n;
		m_child->m_next = m_child->m_prev = nil;
	}
	else
	{
		m_child->insert_sibling(n, position);
		
		if (m_last == nil)
			m_last = m_child;
	}

	return node_iterator(n);
}

//void container::private_insert(node* position, node* n)
//{
//	if (n->m_parent != nil or n->m_next != nil or n->m_prev != nil)
//		throw exception("attempt to insert a node that has parent and/or next/prev");
//	
//	if (position == nil)
//		push_back(n);
//	else
//	{
//		if (position->m_parent != this)
//			throw exception("position is not a child node of this container");
//
//		node* child = m_child;
//		while (child != nil and child != position)
//			child = child->m_next;
//		if (child == nil)
//			throw zeep::exception("position is not a valid child node");
//		
//		n->parent(this);
//		n->m_next = position;
//		position->m_prev = n;
//
//		n->m_prev = position->m_prev;
//		if (n->m_prev != nil)
//			n->m_prev->m_next = n;
//		
//		if (position == m_child)
//			m_child = n;
//	}
//}
//
//void container::private_erase(node* n)
//{
////	if (n == nil)
////		throw exception("attempt to erase nil node");
////	
////	if (n->m_parent != this)
////		throw exception("cannot erase node, invalid parent");
////	
////	if (m_child == n)
////	{
////		m_child = m_child->m_next;
////		if (m_child != nil)
////			m_child->m_prev = nil;
////	}
////	else
////		m_child->remove_sibling(n);
////	
////	if (n == m_last)
////	{
////		m_last = m_child;
////		while (m_last->m_next != nil)
////			m_last = m_last->m_next;
////	}
////	
////	n->m_next = n->m_prev = n->m_parent = nil;
//	remove(n);
//	delete n;
//}

// --------------------------------------------------------------------
// root_node

root_node::root_node()
{
}

root_node::~root_node()
{
}

root_node* root_node::root()
{
	return this;
}

const root_node* root_node::root() const
{
	return this;
}

string root_node::str() const
{
	string result;
	element* e = child_element();
	if (e != nil)
		result = e->str();
	return result;
}

element* root_node::child_element() const
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

void root_node::child_element(element* child)
{
	element* e = child_element();
	if (e != nil)
	{
		container::remove(e);
		delete e;
	}
	
	container::append(child);
}

void root_node::write(writer& w) const
{
	node* child = m_child;
	while (child != nil)
	{
		child->write(w);
		child = child->next();
	}
}

bool root_node::equals(const node* n) const
{
	bool result = false;
	if (typeid(n) == typeid(*this))
	{
		result = true;
		
		const root_node* e = static_cast<const root_node*>(n);
		
		if (m_child != nil and e->m_child != nil)
			result = m_child->equals(e->m_child);
		else
			result = m_child == nil and e->m_child == nil;
	}

	return result;
}

void root_node::append(node* n)
{
	if (dynamic_cast<processing_instruction*>(n) == nil and
		dynamic_cast<comment*>(n) == nil)
	{
		throw exception("can only append comment and processing instruction nodes to a root_node");
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

node* comment::clone() const
{
	return new comment(m_text);
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

node* processing_instruction::clone() const
{
	return new processing_instruction(m_target, m_target);
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

node* text::clone() const
{
	return new text(m_text);
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

node* attribute::clone() const
{
	return new attribute(m_qname, m_value, m_id);
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

node* name_space::clone() const
{
	return new name_space(m_prefix, m_uri);
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
	text* textNode = dynamic_cast<text*>(m_last);
	
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

void element::set_attribute(const string& qname, const string& value, bool id)
{
	attribute* attr = get_attribute_node(qname);
	if (attr != nil)
		attr->value(value);
	else
	{
		attr = new attribute(qname, value, id);
		
		if (m_attribute == nil)
		{
			m_attribute = attr;
			m_attribute->parent(this);
		}
		else
			m_attribute->insert_sibling(attr, nil);
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
			m_attribute->remove_sibling(n);
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
		add_name_space(new name_space(prefix, uri));
}

void element::add_name_space(name_space* ns)
{
	if (m_name_space == nil)
	{
		m_name_space = ns;
		m_name_space->parent(this);
	}
	else
		m_name_space->insert_sibling(ns, nil);
}

string element::lang() const
{
	string result = get_attribute("xml:lang");
	if (result.empty())
		result = node::lang();
	return result;
}

string element::id() const
{
	string result;
	
	attribute* attr = m_attribute;
	while (attr != nil)
	{
		if (attr->id())
		{
			result = attr->value();
			break;
		}
		attr = static_cast<attribute*>(attr->next());
	}
	
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

node* element::clone() const
{
	element* result = new element(m_qname);
	
	attribute* attr = m_attribute;
	while (attr != nil)
	{
		result->set_attribute(attr->qname(), attr->value(), attr->id());
		attr = static_cast<attribute*>(attr->next());
	}
	
	name_space* ns = m_name_space;
	while (ns != nil)
	{
		result->add_name_space(static_cast<name_space*>(ns->clone()));
		ns = static_cast<name_space*>(ns->next());
	}

	node* child = m_child;
	while (child != nil)
	{
		result->push_back(child->clone());
		child = child->next();
	}
	
	return result;
}

// --------------------------------------------------------------------
// operator<<

ostream& operator<<(ostream& lhs, const node& rhs)
{
	if (typeid(rhs) == typeid(node))
		cout << "base class???";
	else if (typeid(rhs) == typeid(root_node))
		cout << "root_node";
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
