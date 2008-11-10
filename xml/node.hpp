#ifndef XMLIO_H
#define XMLIO_H

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace xml
{

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
							: name_(name) {}

						node(
							const std::string&	name,
							const std::string&	ns)
							: name_(name)
							, ns_(ns) {}

						node(
							const std::string&	name,
							const std::string&	ns,
							const std::string&	prefix)
							: name_(name)
							, ns_(ns)
							, prefix_(prefix) {}

						node(
							const node&			rhs);

	std::string			ns() const								{ return ns_; }
	void				ns(const std::string	ns)				{ ns_ = ns; }
	
	std::string			prefix() const							{ return prefix_; }
	void				prefix(const std::string
												prefix)			{ prefix_ = prefix; }
	
	std::string			name() const							{ return name_; }
	void				name(const std::string&	name)			{ name_ = name; }
	
	std::string			content() const							{ return content_; }
	void				content(const std::string&
												content)		{ content_ = content; }

	node_ptr			find_child(
							const std::string&	xpath);

	std::string			get_attribute(
							const std::string&	name);

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
	
	iterator			begin()									{ return iterator(children_); }
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
	
	const_iterator		begin() const							{ return const_iterator(children_); }
	const_iterator		end() const								{ return const_iterator(); }

	class attribute_iterator : public boost::iterator_facade<attribute_iterator, attribute, boost::forward_traversal_tag>
	{
	  public:
						attribute_iterator() {}

						attribute_iterator(
							attribute_ptr&		attr) : attr_(attr)	{}

						attribute_iterator(
							const attribute_iterator&
												rhs) : attr_(rhs.attr_)	{}
		
		attribute&		dereference() const						{ return *attr_; }
		bool			equal(
							const attribute_iterator&
												rhs) const		{ return attr_ == rhs.attr_; }
		void			increment();

	  private:
		attribute_ptr	attr_;
	};
	
	attribute_iterator	attribute_begin()						{ return attribute_iterator(attributes_); }
	attribute_iterator	attribute_end()							{ return attribute_iterator(); }

	class const_attribute_iterator : public boost::iterator_facade<const_attribute_iterator, attribute const, boost::forward_traversal_tag>
	{
	  public:
						const_attribute_iterator() {}

						const_attribute_iterator(
							attribute_ptr		attr) : attr_(attr) {}

						const_attribute_iterator(
							const const_attribute_iterator&
												rhs) : attr_(rhs.attr_)	{}
		
		attribute const&
						dereference() const						{ return *attr_; }
		bool			equal(
							const const_attribute_iterator&
												rhs) const		{ return attr_ == rhs.attr_; }
		void			increment();

	  private:
		attribute_ptr	attr_;
	};
	
	const_attribute_iterator
						attribute_begin() const					{ return const_attribute_iterator(attributes_); }
	const_attribute_iterator
						attribute_end() const					{ return const_attribute_iterator(); }

	// for constructing trees

	void				add_attribute(
							attribute_ptr		attr);
	
	void				add_child(
							node_ptr			node);

	void				add_content(
							const char*			text,
							unsigned long		length);

	// writing out

	void				write(
							std::ostream&		stream,
							int					level) const;

	node_ptr			children() const						{ return children_; }

	node_ptr			next() const							{ return next_; }

  private:

	std::string			name_;
	std::string			ns_;
	std::string			prefix_;
	std::string			content_;
	attribute_ptr		attributes_;
	node_ptr			next_;
	node_ptr			children_;
};

class attribute
{
	friend class node;
  public:
						attribute();
						
						attribute(
							const std::string&	name,
							const std::string&	value)
							: name_(name)
							, value_(value) {}
	
	std::string			name() const							{ return name_; }
	void				name(const std::string& name)			{ name_ = name; }

	std::string			value() const							{ return value_; }
	void				value(const std::string& value)			{ value_ = value; }
	
  private:
	friend class node::attribute_iterator;
	friend class node::const_attribute_iterator;

	attribute_ptr		next() const							{ return next_; }

	attribute_ptr		next_;
	std::string			name_;
	std::string			value_;
};

std::ostream& operator<<(std::ostream& lhs, const node& rhs);

// inlines

inline void node::attribute_iterator::increment()
{
	attr_ = attr_->next();
}

inline void node::const_attribute_iterator::increment()
{
	attr_ = attr_->next();
}


}

#endif
