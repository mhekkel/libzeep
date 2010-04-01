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
//#include <boost/flyweight.hpp>
#include "zeep/xml/attribute.hpp"

namespace zeep { namespace xml {

class writer;

class node;
class comment;
class processing_instruction;
class text;
class element;
typedef node* node_ptr;
typedef std::list<node_ptr>	node_set;

class document;

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

class element : public node
{
  public:
						element(const std::string& name)
							: m_name(name), m_child(NULL) {}

						element(const std::string& name,
							const std::string& prefix)
							: m_name(name), m_prefix(prefix), m_child(NULL) {}

						element(const std::string& name,
							const std::string& ns, const std::string& prefix)
							: m_name(name), m_ns(ns), m_prefix(prefix), m_child(NULL) {}

						~element();

	virtual std::string	str() const;

	node*				child()										{ return m_child; }
	const node*			child() const								{ return m_child; }

	std::string			ns() const									{ return m_ns; }
	void				ns(const std::string& ns)					{ m_ns = ns; }
	
	std::string			prefix() const								{ return m_prefix; }
	void				prefix(const std::string& prefix)			{ m_prefix = prefix; }
	
	std::string			name() const								{ return m_name; }
	void				name(const std::string& name)				{ m_name = name; }
	
	std::string			content() const;
	void				content(const std::string& content);

	// utility functions
	node_ptr			find_first_child(const std::string& name) const;

	node_ptr			find_child(const std::string& path) const;	
							
	node_set			find_all(const std::string&	path) const;

	std::string			get_attribute(const std::string& name) const;
	void				set_attribute(const std::string& ns,
							const std::string& name, const std::string& value);
	void				remove_attribute(const std::string& name);
	
	void				add(node_ptr node);
	void				remove(node_ptr node);
	
	// convenience routine
	void				add_text(const std::string& s);

	node_set			children();
	const node_set		children() const;
	
	attribute_list&		attributes()								{ return m_attributes; }
	const attribute_list&
						attributes() const							{ return m_attributes; }
//
//	std::string			find_prefix(const std::string& uri) const;

	virtual void		write(writer& w) const;

	virtual bool		equals(const node* n) const;

	// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string	lang() const;

  protected:
	std::string			m_name;
	std::string			m_ns;
	std::string			m_prefix;
	attribute_list		m_attributes;
	node*				m_child;
};

std::ostream& operator<<(std::ostream& lhs, const node& rhs);

bool operator==(const node& lhs, const node& rhs);

}
}

#endif
