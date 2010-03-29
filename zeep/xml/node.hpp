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
class node_set;

class document;

// --------------------------------------------------------------------

class node
{
  public:
	// All nodes should be part of a document
	document*			doc();
	const document*		doc() const;
	
	// basic access
	node*				parent()									{ return m_parent; }
	const node*			parent() const								{ return m_parent; }

	node*				next()										{ return m_next; }
	const node*			next() const								{ return m_next; }
	
	node*				prev()										{ return m_prev; }
	const node*			prev() const								{ return m_prev; }
	
	// return all content concatenated, including that of children.
	virtual std::string	str() const									{ return ""; }

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

// node_set is templated to allow for specialised node_sets for elements
class node_set
{
  public:
	struct iterator
	{
		friend class node_set;
		
		typedef ptrdiff_t							difference_type;
		typedef std::bidirectional_iterator_tag		iterator_category;
		typedef node								value_type;
		typedef node*								pointer;
		typedef node&								reference;
	
							iterator() {}

							iterator(std::list<pointer>::iterator e)
								: m_impl(e) {}

							iterator(const iterator& other)
								: m_impl(other.m_impl) {}

		iterator&			operator=(const iterator& other)
							{
								if (this != &other)
									m_impl = other.m_impl;
								return *this;
							}
		
		reference			operator*() const		{ return *(*m_impl); }
		pointer				operator->() const		{ return *m_impl; }
		
		iterator&			operator++()			{ ++m_impl; return *this; }
		iterator			operator++(int)			{ iterator tmp(*this); operator++(); return tmp; }
		
		iterator&			operator--()			{ --m_impl; return *this; }
		iterator			operator--(int)			{ iterator tmp(*this); operator--(); return tmp; }
		
		bool				operator==(const iterator& other) const
													{ return m_impl == other.m_impl; }
		bool				operator!=(const iterator& other) const
													{ return m_impl != other.m_impl; }
	
	  private:
		std::list<node_ptr>::iterator				m_impl;
	};
	
	struct const_iterator
	{
		friend class node_set;

		typedef ptrdiff_t							difference_type;
		typedef std::bidirectional_iterator_tag		iterator_category;
		typedef node								value_type;
		typedef const node*							pointer;
		typedef const node&							reference;
		
							const_iterator() {}

							const_iterator(std::list<node_ptr>::const_iterator e)
								: m_impl(e) {}

							const_iterator(const const_iterator& other)
								: m_impl(other.m_impl) {}

		const_iterator&		operator=(const const_iterator& other)
							{
								if (this != &other)
									m_impl = other.m_impl;
								return *this;
							}
	
		reference			operator*() const		{ return *(*m_impl); }
		pointer				operator->() const		{ return *m_impl; }
		
		const_iterator&		operator++()			{ ++m_impl; return *this; }
		const_iterator		operator++(int)			{ const_iterator tmp(*this); operator++(); return tmp; }
		
		const_iterator&		operator--()			{ --m_impl; return *this; }
		const_iterator		operator--(int)			{ const_iterator tmp(*this); operator--(); return tmp; }
		
		bool				operator==(const const_iterator& other) const
													{ return m_impl == other.m_impl; }
		bool				operator!=(const const_iterator& other) const
													{ return m_impl != other.m_impl; }
	
	  private:
		std::list<node_ptr>::const_iterator			m_impl;
	};
	
	typedef node									node_type;
	typedef node_type*								pointer;
	typedef const node_type*						const_pointer;
	typedef node_type&								reference;
	typedef const node_type&						const_reference;
	typedef std::reverse_iterator<iterator>			reverse_iterator;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;
	typedef std::size_t								size_type;
	typedef ptrdiff_t								difference_type;

						node_set() {}

						node_set(const node_set& other)
							: m_nodes(other.m_nodes) {}
							
	node_set&			operator=(const node_set& other)
						{
							if (this != &other)
								m_nodes = other.m_nodes;
							return *this;
						}

	iterator			begin()						{ return iterator(m_nodes.begin()); }
	iterator			end()						{ return iterator(m_nodes.end()); }
	
	reverse_iterator	rbegin()					{ return reverse_iterator(end()); }
	reverse_iterator	rend()						{ return reverse_iterator(begin()); }
	
	const_iterator		begin() const				{ return const_iterator(m_nodes.begin()); }
	const_iterator		end() const					{ return const_iterator(m_nodes.begin()); }
	
	const_reverse_iterator
						rbegin() const				{ return const_reverse_iterator(end()); }
	const_reverse_iterator
						rend() const				{ return const_reverse_iterator(begin()); }
	
	void				clear()						{ m_nodes.clear(); }
	
	size_type			size() const				{ return m_nodes.size(); }
	
	bool				empty() const				{ return m_nodes.empty(); }

	void				swap(node_set& other)		{ m_nodes.swap(other.m_nodes); }
	
	reference			front()						{ return *m_nodes.front(); }
	const_reference		front() const				{ return *m_nodes.front(); }
	
	reference			back()						{ return *m_nodes.back(); }
	const_reference		back() const				{ return *m_nodes.back(); }

	void				push_front(node_ptr n)		{ m_nodes.push_front(n); }
	void				pop_front()					{ m_nodes.pop_front(); }
	
	void				push_back(node_ptr n)		{ m_nodes.push_back(n); }
	void				pop_back()					{ m_nodes.pop_back(); }

	iterator			insert(iterator pos, node_ptr n)
													{ return iterator(m_nodes.insert(pos.m_impl, n)); }
	iterator			erase(iterator pos)			{ return iterator(m_nodes.erase(pos.m_impl)); }

	void				remove(node_ptr n)			{ m_nodes.remove(n); }
	
//	template<typename PRED>
//	void				remove_if(PRED);
	
	void				unique()					{ m_nodes.unique(); }

  private:
	std::list<node_ptr>	m_nodes;
};

// --------------------------------------------------------------------

class comment : public node
{
  public:
						comment() {}

						comment(const std::string& text)
							: m_text(text) {}

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

//	node*				child()										{ return m_child; }
//	const node*			child() const								{ return m_child; }

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

  private:
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
