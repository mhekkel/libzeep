#ifndef XMLIO_H
#define XMLIO_H

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace soap { namespace xml {

class attribute;
typedef boost::shared_ptr<attribute>	attribute_ptr;

class node;
typedef boost::shared_ptr<node>			node_ptr;

class document;

class node : public boost::noncopyable, public boost::enable_shared_from_this<node>
{
  public:
						node();

						node(
							const std::string&	name)
							: m_name(name) {}

						node(
							const std::string&	name,
							const std::string&	prefix)
							: m_name(name)
							, m_prefix(prefix) {}

						node(
							const std::string&	name,
							const std::string&	ns,
							const std::string&	prefix)
							: m_name(name)
							, m_ns(ns)
							, m_prefix(prefix) {}

						node(
							const node&			rhs);

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

	node_ptr			find_child(
							const std::string&	xpath);

	std::string			get_attribute(
							const std::string&	name);

	template<typename F>
	void				do_to_all(
							F					func);

	class iterator : public boost::iterator_facade<iterator, node, boost::forward_traversal_tag>
	{
	  public:
						iterator() {}

						iterator(
							node_ptr&			n) : n(n)		{}

						iterator(
							const iterator&		rhs) : n(rhs.n)	{}
		
		node&			dereference() const						{ return *n; }
		bool			equal(
							const iterator&		rhs) const		{ return n == rhs.n; }
		void			increment()								{ n = n->next(); }

	  private:
		node_ptr		n;
	};
	
	iterator			begin()									{ return iterator(m_children); }
	iterator			end()									{ return iterator(); }

	class const_iterator : public boost::iterator_facade<const_iterator, node const, boost::forward_traversal_tag>
	{
	  public:
						const_iterator() {}

						const_iterator(
							node_ptr			n) : n(n)		{}

						const_iterator(
							const const_iterator&
												rhs) : n(rhs.n)	{}
		
		node const&		dereference() const						{ return *n; }
		bool			equal(
							const const_iterator&
												rhs) const		{ return n == rhs.n; }
		void			increment()								{ n = n->next(); }

	  private:
		node_ptr		n;
	};
	
	const_iterator		begin() const							{ return const_iterator(m_children); }
	const_iterator		end() const								{ return const_iterator(); }

	class attribute_iterator : public boost::iterator_facade<attribute_iterator, attribute, boost::forward_traversal_tag>
	{
	  public:
						attribute_iterator() {}

						attribute_iterator(
							attribute_ptr&		attr) : m_attr(attr)	{}

						attribute_iterator(
							const attribute_iterator&
												rhs) : m_attr(rhs.m_attr)	{}
		
		attribute&		dereference() const						{ return *m_attr; }
		bool			equal(
							const attribute_iterator&
												rhs) const		{ return m_attr == rhs.m_attr; }
		void			increment();

	  private:
		attribute_ptr	m_attr;
	};
	
	attribute_iterator	attribute_begin()						{ return attribute_iterator(m_attributes); }
	attribute_iterator	attribute_end()							{ return attribute_iterator(); }

	class const_attribute_iterator : public boost::iterator_facade<const_attribute_iterator, attribute const, boost::forward_traversal_tag>
	{
	  public:
						const_attribute_iterator() {}

						const_attribute_iterator(
							attribute_ptr		attr) : m_attr(attr) {}

						const_attribute_iterator(
							const const_attribute_iterator&
												rhs) : m_attr(rhs.m_attr)	{}
		
		attribute const&
						dereference() const						{ return *m_attr; }
		bool			equal(
							const const_attribute_iterator&
												rhs) const		{ return m_attr == rhs.m_attr; }
		void			increment();

	  private:
		attribute_ptr	m_attr;
	};
	
	const_attribute_iterator
						attribute_begin() const					{ return const_attribute_iterator(m_attributes); }
	const_attribute_iterator
						attribute_end() const					{ return const_attribute_iterator(); }

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

	// writing out

	void				write(
							std::ostream&		stream,
							int					level) const;

	node_ptr			children() const						{ return m_children; }

	node_ptr			next() const							{ return m_next; }

	node_ptr			find_first_child(
							const std::string&	name) const;

  private:

	std::string			m_name;
	std::string			m_ns;
	std::string			m_prefix;
	std::string			m_content;
	attribute_ptr		m_attributes;
	node_ptr			m_next;
	node_ptr			m_children;
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
	
  private:
	friend class node::attribute_iterator;
	friend class node::const_attribute_iterator;

	attribute_ptr		next() const							{ return m_next; }

	attribute_ptr		m_next;
	std::string			m_name;
	std::string			m_value;
};

std::ostream& operator<<(std::ostream& lhs, const node& rhs);

// inlines

inline void node::attribute_iterator::increment()
{
	m_attr = m_attr->next();
}

inline void node::const_attribute_iterator::increment()
{
	m_attr = m_attr->next();
}

template<typename F>
void node::do_to_all(
	F			func)
{
	func(*this);
	for (iterator child = begin(); child != end(); ++child)
		child->do_to_all(func);
}

}
}

#endif
