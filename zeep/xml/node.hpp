//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_NODE_HPP
#define SOAP_XML_NODE_HPP

#include <iterator>
#include <string>
#include <list>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace zeep { namespace xml {

class writer;

class node;
typedef node* node_ptr;
typedef std::list<node_ptr>		node_set;

class element;
typedef element* element_ptr;
typedef std::list<element_ptr>	element_set;

class document;

// --------------------------------------------------------------------
// local_name is used by elements and attributes, it is a pair consisting
// of the namespace uri and the local-name

struct exp_name
{
	std::string		ns_name;
	std::string		local_name;
};

// --------------------------------------------------------------------

class node
{
  public:
	// All nodes should be part of a document
	virtual document*	doc();
	virtual const document*
						doc() const;
	
	// basic access
	node*				parent()									{ return m_parent; }
	const node*			parent() const								{ return m_parent; }

	node*				next()										{ return m_next; }
	const node*			next() const								{ return m_next; }
	
	node*				prev()										{ return m_prev; }
	const node*			prev() const								{ return m_prev; }

	// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string	lang() const;
	
	// return all content concatenated, including that of children.
	virtual std::string	str() const = 0;

	// writing out
	virtual void		write(writer& w) const = 0;

	virtual bool		equals(const node* n) const;

  protected:

	friend class element;

						node();

	virtual				~node();

	virtual void		append_to_list(node* n);
	virtual void		remove_from_list(node* n);
	
	node*				m_parent;
	node*				m_next;
	node*				m_prev;

  private:

						node(const node&);
	node&				operator=(const node&);
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
						attribute(const std::string& qname, const std::string& value)
							: m_qname(qname), m_value(value) {}

	std::string			qname() const								{ return m_qname; }
	std::string			local_name() const;
	std::string			prefix() const;

	std::string			value() const								{ return m_value; }
	void				value(const std::string& v)					{ m_value = v; }

	virtual std::string	str() const									{ return value(); }

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

  private:
	std::string			m_qname, m_value;
};

typedef std::list<attribute*> attribute_set;

// --------------------------------------------------------------------
// just like an attribute, a name_space node is not a child of an element

class name_space : public node
{
  public:
						name_space(const std::string& prefix, const std::string& uri)
							: m_prefix(prefix), m_uri(uri) {}
	
	std::string			prefix() const								{ return m_prefix; }
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

class element : public node
{
  public:
						element(const std::string& qname);
						~element();

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

	virtual std::string	str() const;

	node*				child()										{ return m_child; }
	const node*			child() const								{ return m_child; }

	std::string			ns_name_for_prefix(const std::string& prefix) const;
	std::string			prefix_for_ns_name(const std::string& uri) const;
	
	std::string			qname() const								{ return m_qname; }
	std::string			prefix() const;
	std::string			local_name() const;
	std::string			ns_name() const								{ return ns_name_for_prefix(prefix()); }
	
	std::string			content() const;
	void				content(const std::string& content);

	// utility functions
						// find first element child having localname 'name'
	element*			find_first_child(const std::string& name);

	std::string			get_attribute(const std::string& qname) const;
	attribute*			get_attribute_node(const std::string& qname) const;
	void				set_attribute(const std::string& qname, const std::string& value);
	void				remove_attribute(const std::string& qname);

	void				set_name_space(const std::string& prefix,
							const std::string& uri);
//	void				remove_name_space(const std::string& uri);
	
	void				append(node* node);
	void				remove(node* node);
	
	// convenience routines
	void				add_text(const std::string& s);

	template<typename NODE_TYPE>
	std::list<NODE_TYPE*>
						children() const;
	attribute_set		attributes() const;
	name_space_list		name_spaces() const;

	// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string	lang() const;

  protected:
	std::string			m_qname;
	node*				m_child;
	attribute*			m_attribute;
	name_space*			m_name_space;
};

std::ostream& operator<<(std::ostream& lhs, const node& rhs);

bool operator==(const node& lhs, const node& rhs);

// very often, we want to iterate over child elements of an element
// therefore we have a templated version of children.

template<>
std::list<node*> element::children<node>() const;

template<>
std::list<element*> element::children<element>() const;

}
}

#endif
