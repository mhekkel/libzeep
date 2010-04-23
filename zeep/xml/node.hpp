//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_NODE_HPP
#define SOAP_XML_NODE_HPP

#include <iterator>
#include <string>
#include <list>

#include <boost/tr1/tuple.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "zeep/config.hpp"

namespace zeep { namespace xml {

class writer;

class node;
typedef node* node_ptr;
typedef std::list<node_ptr>		node_set;

class element;
typedef element* element_ptr;
typedef std::list<element_ptr>	element_set;

class root_node;
class container;

// --------------------------------------------------------------------

class node
{
  public:
	// All nodes should be part of a single root node
	virtual root_node*	root();
	virtual const root_node*
						root() const;
	
	// basic access
	node*				parent()									{ return m_parent; }
	const node*			parent() const								{ return m_parent; }

	node*				next()										{ return m_next; }
	const node*			next() const								{ return m_next; }
	
	node*				prev()										{ return m_prev; }
	const node*			prev() const								{ return m_prev; }

	// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string	lang() const;

	// Nodes can have a name, and the XPath specification requires that a node can
	// have a so-called expanded-name. This name consists of a local-name and a
	// namespace which is a URI. And we can have a QName which is a concatenation of
	// a prefix (that points to a namespace URI) and a local-name separated by a colon.
	//
	// To reduce storage requirements, names are stored in nodes as qnames, if at all.
	virtual std::string	qname() const;

	// the convenience functions name() and prefix() parse the qname(). ns() returns
	// the namespace URI for the node, if it can be resolved.
	virtual std::string	name() const;
	virtual std::string	prefix() const;
	virtual std::string	ns() const;

	// resolving prefixes and namespaces is done hierarchically
	virtual std::string	namespace_for_prefix(const std::string& prefix) const;
	virtual std::string	prefix_for_namespace(const std::string& uri) const;
	
	// return all content concatenated, including that of children.
	virtual std::string	str() const = 0;

	// writing out
	virtual void		write(writer& w) const = 0;

	virtual bool		equals(const node* n) const;

  protected:

	friend class container;
	friend class element;

						node();
	virtual				~node();

	virtual void		append_sibling(node* n);
	virtual void		remove_sibling(node* n);

	void				parent(node* p);
	void				next(node* n);
	void				prev(node* n);

  private:
	node*				m_parent;
	node*				m_next;
	node*				m_prev;

						node(const node&);
	node&				operator=(const node&);
};

// --------------------------------------------------------------------

class container : public node
{
  public:
	/// container tries hard to be a stl::container.
	
						~container();

	node*				child()										{ return m_child; }
	const node*			child() const								{ return m_child; }

	template<typename NODE_TYPE>
	std::list<NODE_TYPE*>
						children() const;

	class iterator : public std::iterator<std::bidirectional_iterator_tag, element>
	{
	  public:
						iterator() : m_current(nil)					{}
						iterator(element* e) : m_current(e)			{}
						iterator(const iterator& other)
							: m_current(other.m_current)			{}

		iterator&		operator=(const iterator& other)			{ m_current = other.m_current; return *this; }
		
		reference		operator*() const							{ return *m_current; }
		pointer			operator->() const							{ return m_current; }

		iterator&		operator++();
		iterator		operator++(int)								{ iterator iter(*this); operator++(); return iter; }

		iterator&		operator--();
		iterator		operator--(int)								{ iterator iter(*this); operator++(); return iter; }

		bool			operator==(const iterator& other) const		{ return m_current == other.m_current; }
		bool			operator!=(const iterator& other) const		{ return m_current != other.m_current; }

		pointer			base() const								{ return m_current; }

	  private:
		element*		m_current;
	};

	iterator			begin();
	iterator			end()										{ return iterator(); }

	class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const element>
	{
	  public:
						const_iterator() : m_current(nil)			{}
						const_iterator(const element* e) : m_current(e)	
																	{}
						const_iterator(const const_iterator& other)
							: m_current(other.m_current)			{}
						const_iterator(const iterator& other)
							: m_current(other.base())				{}

		const_iterator&	operator=(const const_iterator& other)		{ m_current = other.m_current; return *this; }
		
		reference		operator*() const							{ return *m_current; }
		pointer			operator->() const							{ return m_current; }

		const_iterator&	operator++();
		const_iterator	operator++(int)								{ const_iterator iter(*this); operator++(); return iter; }

		const_iterator&	operator--();
		const_iterator	operator--(int)								{ const_iterator iter(*this); operator++(); return iter; }

		bool			operator==(const const_iterator& other) const
																	{ return m_current == other.m_current; }

		bool			operator!=(const const_iterator& other) const
																	{ return m_current != other.m_current; }

		pointer			base() const								{ return m_current; }

	  private:
		const element*	m_current;
	};

	const_iterator		begin() const;
	const_iterator		end() const									{ return const_iterator(); }

	/// 
	typedef iterator::value_type		value_type;
	typedef iterator::reference			reference;
	typedef iterator::pointer			pointer;
	typedef iterator::difference_type	difference_type;

	virtual void		append(node* node);
	virtual void		remove(node* node);

	// xpath wrappers
	element_set			find(const std::string& path) const;
	element*			find_first(const std::string& path) const;

  protected:
						container();

	node*				m_child;
	node*				m_last;
};

// --------------------------------------------------------------------

class root_node : public container
{
  public:
						root_node();
						~root_node();

	// All nodes should be part of a single root node
	virtual root_node*	root();
	virtual const root_node*
						root() const;

	// root nodes have only one child element:
	element*			child_element() const;
	void				child_element(element* e);

	// string is the concatenation of the string-value of all
	// descendant text-nodes.
	virtual std::string	str() const;

	// for adding other nodes, like processing instructions and comments
	virtual void		append(node* node);

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;
};

// --------------------------------------------------------------------

class comment : public node
{
  public:
						comment() {}

						comment(const std::string& text)
							: m_text(text) {}

	virtual std::string	str() const									{ return m_text; }

	virtual std::string	text() const								{ return m_text; }

	void				text(const std::string& text)				{ m_text = text; }

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

  private:
	std::string			m_text;
};

// --------------------------------------------------------------------

class processing_instruction : public node
{
  public:
						processing_instruction() {}

						processing_instruction(const std::string& target, const std::string& text)
							: m_target(target), m_text(text) {}

	virtual std::string	qname() const								{ return m_target; }
	virtual std::string	str() const									{ return m_target + ' ' + m_text; }

	std::string			target() const								{ return m_target; }
	void				target(const std::string& target)			{ m_target = target; }

	virtual std::string	text() const								{ return m_text; }
	void				text(const std::string& text)				{ m_text = text; }

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

  private:
	std::string			m_target;
	std::string			m_text;
};

// --------------------------------------------------------------------

class text : public node
{
  public:
						text() {}
						text(const std::string& text)
							: m_text(text) {}

	virtual std::string	str() const									{ return m_text; }

	void				str(const std::string& text)				{ m_text = text; }

	void				append(const std::string& text)				{ m_text.append(text); }

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

  private:
	std::string			m_text;
};

// --------------------------------------------------------------------
// an attribute is a node, has an element as parent, but is not a child of this parent (!)

class attribute : public node
{
  public:
						attribute(const std::string& qname, const std::string& value, bool id = false)
							: m_qname(qname), m_value(value), m_id(id) {}

	std::string			qname() const								{ return m_qname; }

	std::string			value() const								{ return m_value; }
	void				value(const std::string& v)					{ m_value = v; }

	virtual std::string	str() const									{ return value(); }

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;
	
	virtual bool		id() const									{ return m_id; }

  private:
	std::string			m_qname, m_value;
	bool				m_id;
};

typedef std::list<attribute*> attribute_set;

// --------------------------------------------------------------------
// just like an attribute, a name_space node is not a child of an element

class name_space : public node
{
  public:
						name_space(const std::string& prefix, const std::string& uri)
							: m_prefix(prefix), m_uri(uri) {}

	virtual std::string	qname() const								{ return m_prefix; }
	virtual std::string	ns() const									{ return ""; }
	virtual std::string	prefix() const								{ return m_prefix; }
	
	void				prefix(const std::string& p)				{ m_prefix = p; }
	
	std::string			uri() const									{ return m_uri; }
	void				uri(const std::string& u)					{ m_uri = u; }

	virtual std::string	str() const									{ return uri(); }
	
	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

  private:
	std::string			m_prefix, m_uri;
};

typedef std::list<name_space*>	name_space_list;

// --------------------------------------------------------------------

class element : public container
{
  public:
						element(const std::string& qname);
						~element();

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

	virtual std::string	str() const;

	std::string			qname() const								{ return m_qname; }
	
	std::string			namespace_for_prefix(const std::string& prefix) const;
	std::string			prefix_for_namespace(const std::string& uri) const;
	
	std::string			content() const;
	void				content(const std::string& content);

	std::string			get_attribute(const std::string& qname) const;
	attribute*			get_attribute_node(const std::string& qname) const;
						// the DOCTYPE can specify some attributes as ID						
	void				set_attribute(const std::string& qname, const std::string& value, bool id = false);
	void				remove_attribute(const std::string& qname);

	void				set_name_space(const std::string& prefix,
							const std::string& uri);
//	void				remove_name_space(const std::string& uri);
	
	// The add_text method checks if the last added child is a text node,
	// and if so, it appends the string to this node's value. Otherwise,
	// it adds a new text node child with the new text.
	void				add_text(const std::string& s);

	// to iterate over the attribute and namespace nodes
	attribute_set		attributes() const;
	name_space_list		name_spaces() const;

	// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string	lang() const;
	
	// content of the xml:id attribute, or the attribute that was defined to be
	// of type ID by the DOCTYPE.
	std::string			id() const;

  protected:
	std::string			m_qname;
	attribute*			m_attribute;
	name_space*			m_name_space;
};

std::ostream& operator<<(std::ostream& lhs, const node& rhs);

bool operator==(const node& lhs, const node& rhs);

// very often, we want to iterate over child elements of an element
// therefore we have a templated version of children.

template<>
std::list<node*> container::children<node>() const;

template<>
std::list<container*> container::children<container>() const;

template<>
std::list<element*> container::children<element>() const;

// iterator inlines

inline container::iterator& container::iterator::operator++()
{
	if (m_current->next() == nil)
		m_current = nil;
	else
	{
		for (node* n = m_current->next(); n != nil; n = n->next())
		{
			m_current = dynamic_cast<element*>(n);
			if (m_current != nil)
				break;
		}
	}
	return *this;
}

inline container::iterator& container::iterator::operator--()
{
	if (m_current->prev() == nil)
		m_current = nil;
	else
	{
		for (node* n = m_current->prev(); n != nil; n = n->prev())
		{
			m_current = dynamic_cast<element*>(n);
			if (m_current != nil)
				break;
		}
	}
	return *this;
}

inline container::const_iterator& container::const_iterator::operator++()
{ 
	if (m_current->next() == nil)
		m_current = nil;
	else
	{
		for (const node* n = m_current->next(); n != nil; n = n->next())
		{
			m_current = dynamic_cast<const element*>(n);
			if (m_current != nil)
				break;
		}
	}
	return *this;
}

inline container::const_iterator& container::const_iterator::operator--()
{
	if (m_current->prev() == nil)
		m_current = nil;
	else
	{
		for (const node* n = m_current->prev(); n != nil; n = n->prev())
		{
			m_current = dynamic_cast<const element*>(n);
			if (m_current != nil)
				break;
		}
	}
	return *this;
}

inline container::iterator container::begin()
{
	element* first = nil;
	
	for (node* n = m_child; n != nil; n = n->next())
	{
		first = dynamic_cast<element*>(n);
		if (first != nil)
			break;
	}
	return iterator(first);
}

inline container::const_iterator container::begin() const
{
	const element* first = nil;
	
	for (const node* n = m_child; n != nil; n = n->next())
	{
		first = dynamic_cast<const element*>(n);
		if (first != nil)
			break;
	}
	return const_iterator(first);
}

}
}

#endif
