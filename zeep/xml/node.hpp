//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_NODE_HPP
#define SOAP_XML_NODE_HPP

#include <string>
#include <list>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace zeep { namespace xml {

class attribute;
typedef boost::shared_ptr<attribute>	attribute_ptr;
class									attribute_list;

class node;
typedef boost::shared_ptr<node>			node_ptr;
class									node_list;

class document;

class node : public boost::noncopyable, public boost::enable_shared_from_this<node>
{
  public:
						node();

						node(
							const std::string&	name);

						node(
							const std::string&	name,
							const std::string&	prefix);

						node(
							const std::string&	name,
							const std::string&	ns,
							const std::string&	prefix);

//						node(
//							const node&			rhs);
	
	virtual				~node();

	std::string			ns() const								{ return m_ns; }
	void				ns(const std::string	ns)				{ m_ns = ns; }
	
	std::string			prefix() const							{ return m_prefix; }
	void				prefix(const std::string
												prefix)			{ m_prefix = prefix; }
	
	std::string			name() const							{ return m_name; }
	void				name(const std::string&	name)			{ m_name = name; }
	
	std::string			content() const							{ return m_content; }
	void				content(const std::string&
												content)		{ m_content = content; }

	// utility functions
	node_ptr			find_first_child(
							const std::string&	name) const;

	node_ptr			find_child(
							const std::string&	path) const;
							
	node_list			find_all(
							const std::string&	path) const;

	std::string			get_attribute(
							const std::string&	name) const;

	// for constructing trees

	void				add_attribute(
							attribute_ptr		attr);
	
	void				add_attribute(
							const std::string&	name,
							const std::string&	value);

	void				remove_attribute(const std::string& name);
	
	void				add_child(
							node_ptr			node);

	void				add_content(
							const char*			text,
							unsigned long		length);

	void				add_content(const std::string& data);

	// writing out

	void				write(
							std::ostream&		stream,
							int					level) const;

	node_list&			children()								{ return *m_children; }
	const node_list&	children() const						{ return *m_children; }
	
	attribute_list&		attributes()							{ return *m_attributes; }
	const attribute_list&
						attributes() const						{ return *m_attributes; }

	std::string			find_prefix(
							const std::string&	uri) const;

  private:

	std::string			m_name;
	std::string			m_ns;
	std::string			m_prefix;
	std::string			m_content;
	node_list*			m_children;
	attribute_list*		m_attributes;
	node*				m_parent;
};

class node_list : public boost::noncopyable
{
  public:
						node_list() {}

						node_list(
							const node_list&
											other)
							: m_nodes(other.m_nodes) {}

	node_list&			operator=(
							const node_list&
											other)				{
																	m_nodes = other.m_nodes;
																	return *this;
																}

	template<typename Compare>
	void				sort(Compare comp)						{ std::sort(m_nodes.begin(), m_nodes.end(), comp); }

	template<typename NODE>
	class iterator_base : public boost::iterator_facade<iterator_base<NODE>, NODE, boost::bidirectional_traversal_tag>
	{
	    friend class boost::iterator_core_access;
	    friend class node_list;
	    template<class> friend class iterator_base;
	  public:

		typedef typename boost::iterator_facade<iterator_base<NODE>, NODE, boost::bidirectional_traversal_tag>::reference reference;

						iterator_base() {}
		explicit		iterator_base(std::list<node_ptr>::iterator iter)
							: m_iter(iter) {}

		template<class OTHER_NODE>
						iterator_base(
							const iterator_base<OTHER_NODE>& other)
							: m_iter(other.m_iter) {}
		
		reference		dereference() const						{ return *m_iter->get(); }
		void			increment()								{ ++m_iter; }
		void			decrement()								{ --m_iter; }
		bool			equal(
							const iterator_base& rhs) const		{ return m_iter == rhs.m_iter; }

	  private:
		std::list<node_ptr>::iterator
						m_iter;
	};

	typedef iterator_base<node>			iterator;

	iterator			begin()									{ return iterator(m_nodes.begin()); }
	iterator			end()									{ return iterator(m_nodes.end()); }

	typedef iterator_base<const node>	const_iterator;

	const_iterator		begin() const							{ return const_iterator(const_cast<std::list<node_ptr>&>(m_nodes).begin()); }
	const_iterator		end() const								{ return const_iterator(const_cast<std::list<node_ptr>&>(m_nodes).end()); }

	template<typename InputIterator>
	void				insert(
							iterator		pos,
							InputIterator	first,
							InputIterator	last)				{ m_nodes.insert(pos.m_iter, first.m_iter, last.m_iter); }

	void				push_back(
							node_ptr		node)				{ m_nodes.push_back(node); }

	bool				empty() const							{ return m_nodes.empty(); }

	node_ptr			front()									{ return m_nodes.front(); }

	node_ptr			back()									{ return m_nodes.back(); }

  private:
	std::list<node_ptr>	m_nodes;
};

class attribute
{
	friend class node;
  public:
						attribute();
						
						attribute(
							const std::string&	name,
							const std::string&	value)
							: m_name(name)
							, m_value(value) {}

	std::string			name() const							{ return m_name; }
	void				name(const std::string& name)			{ m_name = name; }

	std::string			value() const							{ return m_value; }
	void				value(const std::string& value)			{ m_value = value; }

	bool				operator==(const attribute& a)			{ return m_name == a.m_name and m_value == a.m_value; }
	
  private:
	std::string			m_name;
	std::string			m_value;
};

class attribute_list
{
  public:
						attribute_list() {}

						attribute_list(const attribute_list& other) : m_attributes(other.m_attributes) {}

	attribute_list&		operator=(const attribute_list& other)		{ m_attributes = other.m_attributes; return *this; };

	void				push_back(
							attribute_ptr	attr)					{ m_attributes.push_back(attr); }

	template<typename _Predicate>
	void				remove_if(
							_Predicate		pred)					{ m_attributes.erase(std::remove_if(m_attributes.begin(), m_attributes.end(), pred), m_attributes.end()); }

	template<typename Compare>
	void				sort(Compare comp)							{ m_attributes.sort(comp); }

	void				sort();
	
	template<typename ATTR>
	class iterator_base : public boost::iterator_facade<iterator_base<ATTR>, ATTR, boost::bidirectional_traversal_tag>
	{
	    friend class boost::iterator_core_access;
	    template<class> friend class iterator_base;

	  public:
		typedef typename boost::iterator_facade<iterator_base<ATTR>, ATTR, boost::bidirectional_traversal_tag>::reference reference;

						iterator_base() {}

		template<class OTHER_ATTR>
						iterator_base(iterator_base<OTHER_ATTR> const& other)
							: m_iter(other.m_iter) {}

		explicit		iterator_base(std::list<attribute_ptr>::iterator iter)
							: m_iter(iter) {}
		
		reference		dereference() const						{ return *m_iter->get(); }
		void			increment()								{ ++m_iter; }
		void			decrement()								{ --m_iter; }
		bool			equal(
							const iterator_base& rhs) const		{ return m_iter == rhs.m_iter; }

	  private:
		std::list<attribute_ptr>::iterator
						m_iter;
	};

	typedef iterator_base<attribute>							iterator;

	iterator			begin()									{ return iterator(m_attributes.begin()); }
	iterator			end()									{ return iterator(m_attributes.end()); }

	typedef iterator_base<const attribute>						const_iterator;

	const_iterator		begin() const							{ return const_iterator(const_cast<std::list<attribute_ptr>&>(m_attributes).begin()); }
	const_iterator		end() const								{ return const_iterator(const_cast<std::list<attribute_ptr>&>(m_attributes).end()); }

	size_t				size() const							{ return m_attributes.size(); }
	bool				empty() const							{ return m_attributes.empty(); }

  private:
	std::list<attribute_ptr>
						m_attributes;
};

std::ostream& operator<<(std::ostream& lhs, const node& rhs);

bool operator==(const node& lhs, const node& rhs);
bool operator==(const node_list& lhs, const node_list& rhs);

//bool operator==(const attribute& lhs, const attribute& rhs);
bool operator==(const attribute_list& lhs, const attribute_list& rhs);

// inlines
// a set of convenience routines to create a nodes along with attributes in one call
attribute_ptr make_attribute(const std::string& name, const std::string& value);
node_ptr make_node(const std::string& name,
	attribute_ptr attr1 = attribute_ptr(), attribute_ptr attr2 = attribute_ptr(),
	attribute_ptr attr3 = attribute_ptr(), attribute_ptr attr4 = attribute_ptr(),
	attribute_ptr attr5 = attribute_ptr(), attribute_ptr attr6 = attribute_ptr(),
	attribute_ptr attr7 = attribute_ptr(), attribute_ptr attr8 = attribute_ptr());

}
}

#endif
