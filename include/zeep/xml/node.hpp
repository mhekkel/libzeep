// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_NODE_HPP
#define SOAP_XML_NODE_HPP

#include <cassert>

#include <iterator>
#include <string>
#include <list>
#include <limits>

#include <zeep/config.hpp>
#include <zeep/exception.hpp>

#include <boost/range/iterator_range.hpp>

namespace zeep
{
namespace xml
{

class writer;

class node;
typedef node* node_ptr;
typedef std::list<node_ptr> node_set;

class element;
typedef element* element_ptr;
typedef std::list<element_ptr> element_set;

class root_node;
class container;
class xpath;

#ifndef LIBZEEP_DOXYGEN_INVOKED
extern const char kWhiteSpaceChar[]; // a static const char array containing a single space
#endif

// --------------------------------------------------------------------

/// Node is the abstract base class for all data contained in zeep XML documents.
/// The DOM tree consists of nodes that are linked to each other, each
/// node can have a parent and siblings pointed to by the next and
/// previous members. All nodes in a DOM tree share a common root node.
///
/// Nodes can have a name, and the XPath specification requires that a node can
/// have a so-called expanded-name. This name consists of a local-name and a
/// namespace which is a URI. And we can have a QName which is a concatenation of
/// a prefix (that points to a namespace URI) and a local-name separated by a colon.
///
/// To reduce storage requirements, names are stored in nodes as qnames, if at all.
/// the convenience functions name() and prefix() parse the qname(). ns() returns
/// the namespace URI for the node, if it can be resolved.
///
/// Nodes inherit the namespace of their parent unless they override it which means
/// resolving prefixes and namespaces is done hierarchically

class node
{
public:
	// All nodes should be part of a single root node
	virtual root_node* root(); ///< The root node for this node
	virtual const root_node *
	root() const; ///< The root node for this node

	// basic access
	container* parent() { return m_parent; }			 ///< The root node for this node
	const container* parent() const { return m_parent; } ///< The root node for this node

	node* next() { return m_next; }				///< The next sibling
	const node* next() const { return m_next; } ///< The next sibling

	node* prev() { return m_prev; }				///< The previous sibling
	const node* prev() const { return m_prev; } ///< The previous sibling

	/// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string lang() const;

	/// Nodes can have a name, and the XPath specification requires that a node can
	/// have a so-called expanded-name. This name consists of a local-name and a
	/// namespace which is a URI. And we can have a QName which is a concatenation of
	/// a prefix (that points to a namespace URI) and a local-name separated by a colon.
	///
	/// To reduce storage requirements, names are stored in nodes as qnames, if at all.
	virtual std::string qname() const;

	virtual std::string name() const;   ///< The name for the node as parsed from the qname.
	virtual std::string prefix() const; ///< The prefix for the node as parsed from the qname.
	virtual std::string ns() const;		///< Returns the namespace URI for the node, if it can be resolved.

	virtual std::string namespace_for_prefix(const std::string& prefix) const;
	///< Return the namespace URI for a prefix

	virtual std::string prefix_for_namespace(const std::string& uri) const;
	///< Return the prefix for a namespace URI

	/// return all content concatenated, including that of children.
	virtual std::string str() const = 0;

	/// both attribute and element implement str(const string&), others will throw
	virtual void str(const std::string& value) { throw exception("cannot set str for this node"); }

	/// write out the concatenated content to a stream, separated by sep.
	virtual void write_content(std::ostream& os, const char* sep = kWhiteSpaceChar) const;

	/// writing out
	virtual void write(writer& w) const = 0;

	/// Compare the node with \a n
	virtual bool equals(const node* n) const;

	/// Deep clone the node
	virtual node* clone() const;

	/// debug routine
	virtual void validate();

#ifndef LIBZEEP_DOXYGEN_INVOKED
protected:
	friend class container;
	friend class element;

	node();
	virtual ~node();

	virtual void insert_sibling(node* n, node* before);
	virtual void remove_sibling(node* n);

	void parent(container* p);
	void next(node* n);
	void prev(node* n);

private:
	container* m_parent;
	node* m_next;
	node* m_prev;

	node(const node &);
	node& operator=(const node &);
#endif
};

// --------------------------------------------------------------------

/// Container is an abstract base class for nodes that can have multiple children.
/// It provides iterators to iterate over children. Most often, you're only interested
/// in iteration zeep::xml::element children, that's why zeep::xml::container::iterator
/// iterates over only zeep::xml::element nodes, skipping all other nodes. If you want
/// to iterate all nodes, use zeep::xml::container::node_iterator instead.
///
/// An attempt has been made to make container conform to the STL container interface.

class container : public node
{
public:
	/// container tries hard to be stl::container-like.

	~container();

	node* child() { return m_child; }
	const node* child() const { return m_child; }

	template <typename NodeType>
	std::list<NodeType *>
	children() const;

	template <class NodeType>
	class basic_iterator : public std::iterator<std::bidirectional_iterator_tag, NodeType *>
	{
	public:
		typedef typename std::iterator<std::bidirectional_iterator_tag, NodeType *> base_type;
		typedef typename base_type::reference reference;
		typedef typename base_type::pointer pointer;

		basic_iterator() : m_current(nullptr) {}
		basic_iterator(NodeType* e) : m_current(e) {}
		basic_iterator(const basic_iterator& other)
			: m_current(other.m_current) {}

		basic_iterator& operator=(const basic_iterator& other)
		{
			m_current = other.m_current;
			return* this;
		}
		basic_iterator& operator=(const NodeType* n)
		{
			m_current = n;
			return* this;
		}

		reference operator*() { return m_current; }
		pointer operator->() const { return m_current; }

		basic_iterator& operator++();
		basic_iterator operator++(int)
		{
			basic_iterator iter(*this);
			operator++();
			return iter;
		}

		basic_iterator& operator--();
		basic_iterator operator--(int)
		{
			basic_iterator iter(*this);
			operator++();
			return iter;
		}

		bool operator==(const basic_iterator& other) const
		{
			return m_current == other.m_current;
		}
		bool operator!=(const basic_iterator& other) const
		{
			return m_current != other.m_current;
		}

		template <class RNodeType>
		bool operator==(const RNodeType n) const { return m_current == n; }

		template <class RNodeType>
		bool operator!=(const RNodeType n) const { return m_current != n; }

		operator const pointer() const { return m_current; }
		operator pointer() { return m_current; }

	private:
		NodeType* m_current;
	};

	typedef basic_iterator<element> iterator;
	typedef basic_iterator<node> node_iterator;

	iterator begin();
	iterator end() { return iterator(); }

	node_iterator node_begin();
	node_iterator node_end() { return node_iterator(); }

	boost::iterator_range<node_iterator>
	nodes() { return boost::iterator_range<node_iterator>(node_begin(), node_end()); }

	typedef basic_iterator<const element> const_iterator;
	typedef basic_iterator<const node> const_node_iterator;

	const_iterator begin() const;
	const_iterator end() const { return const_iterator(); }

	const_node_iterator node_begin() const;
	const_node_iterator node_end() const { return const_node_iterator(); }

	boost::iterator_range<const_node_iterator>
	nodes() const { return boost::iterator_range<const_node_iterator>(node_begin(), node_end()); }

	///
	typedef iterator::value_type value_type;
	typedef iterator::reference reference;
	typedef iterator::pointer pointer;
	typedef iterator::difference_type difference_type;
	typedef unsigned long size_type;

	//						rbegin
	//						rend
	// size counts only the direct child nodes (not elements!)
	size_type size() const;
	size_type max_size() const { return std::numeric_limits<size_type>::max(); }
	bool empty() const;

	node* front() const;
	node* back() const;

	template <class NodeType>
	basic_iterator<NodeType>
	insert(basic_iterator<NodeType> position, NodeType* n);

	node_iterator insert(node* before, node* n);

	template <class Iterator>
	void insert(Iterator position, Iterator first, Iterator last);

	template <class Iterator>
	void erase(Iterator position);

	template <class Iterator>
	void erase(Iterator first, Iterator last);

	void swap(container& cnt);

	void clear();

	void push_front(node* n);
	void pop_front();
	void push_back(node* n);
	void pop_back();

	// old names
	virtual void append(node* n);
	virtual void remove(node* n); // remove does not delete n

	// xpath wrappers
	element_set find(const std::string& path) const { return find(path.c_str()); }
	element* find_first(const std::string& path) const { return find_first(path.c_str()); }

	element_set find(const char* path) const;
	element* find_first(const char* path) const;

	// xpath wrappers that can return attributes as well as elements:
	void find(const char* path, node_set& nodes) const;
	void find(const char* path, element_set& elements) const;
	node* find_first_node(const char* path) const;

	// debug routine
	virtual void validate();

protected:
	container();

	node* m_child;
	node* m_last;
};

// --------------------------------------------------------------------

/// All zeep::xml::document objects have exactly one zeep::xml::root_node member.
/// root_node is a container with only one child element.

class root_node : public container
{
public:
	root_node();
	~root_node();

	// All nodes should be part of a single root node
	virtual root_node* root();
	virtual const root_node *
	root() const;

	// root nodes have only one child element:
	element* child_element() const;
	void child_element(element* e);

	// string is the concatenation of the string-value of all
	// descendant text-nodes.
	virtual std::string str() const;

	// for adding other nodes, like processing instructions and comments
	virtual void append(node* n);

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;
};

// --------------------------------------------------------------------

/// A node containing a XML comment

class comment : public node
{
public:
	comment() {}

	comment(const std::string& text)
		: m_text(text) {}

	virtual std::string str() const { return m_text; }

	virtual std::string text() const { return m_text; }

	void text(const std::string& text) { m_text = text; }

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;

	virtual node* clone() const;

private:
	std::string m_text;
};

// --------------------------------------------------------------------

/// A node containing a XML processing instruction (like e.g. \<?php ?\>)

class processing_instruction : public node
{
public:
	processing_instruction() {}

	processing_instruction(const std::string& target, const std::string& text)
		: m_target(target), m_text(text) {}

	virtual std::string qname() const { return m_target; }
	virtual std::string str() const { return m_target + ' ' + m_text; }

	std::string target() const { return m_target; }
	void target(const std::string& target) { m_target = target; }

	virtual std::string text() const { return m_text; }
	void text(const std::string& text) { m_text = text; }

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;

	virtual node* clone() const;

private:
	std::string m_target;
	std::string m_text;
};

// --------------------------------------------------------------------

/// A node containing text.

class text : public node
{
public:
	text() {}
	text(const std::string& text)
		: m_text(text) {}

	virtual std::string str() const { return m_text; }

	virtual void str(const std::string& text) { m_text = text; }

	virtual void write_content(std::ostream& os, const char* sep = kWhiteSpaceChar) const
	{
		os << m_text;
	}

	void append(const std::string& text) { m_text.append(text); }

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;

	virtual node* clone() const;

protected:
	std::string m_text;
};

// --------------------------------------------------------------------

/// A node containing the contents of a CDATA section. Normally, these nodes are
/// converted to text nodes but you can specify to preserve them when parsing a
/// document.

class cdata : public text
{
public:
	cdata() {}
	cdata(const std::string& s)
		: text(s) {}

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;

	virtual node* clone() const;
};

// --------------------------------------------------------------------
/// An attribute is a node, has an element as parent, but is not a child of this parent (!)

class attribute : public node
{
public:
	attribute(const std::string& qname, const std::string& value, bool id = false)
		: m_qname(qname), m_value(value), m_id(id) {}

	std::string qname() const { return m_qname; }

	std::string value() const { return m_value; }
	void value(const std::string& v) { m_value = v; }

	virtual std::string str() const { return m_value; }
	virtual void str(const std::string& value) { m_value = value; }

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;

	virtual node* clone() const;

	virtual bool id() const { return m_id; }

private:
	std::string m_qname, m_value;
	bool m_id;
};

typedef std::list<attribute *> attribute_set;

// --------------------------------------------------------------------
/// Just like an attribute, a name_space node is not a child of an element

class name_space : public node
{
public:
	name_space(const std::string& prefix, const std::string& uri)
		: m_prefix(prefix), m_uri(uri) {}

	virtual std::string qname() const { return m_prefix; }
	virtual std::string ns() const { return ""; }
	virtual std::string prefix() const { return m_prefix; }

	void prefix(const std::string& p) { m_prefix = p; }

	std::string uri() const { return m_uri; }
	void uri(const std::string& u) { m_uri = u; }

	virtual std::string str() const { return uri(); }

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;

	virtual node* clone() const;

private:
	std::string m_prefix, m_uri;
};

typedef std::list<name_space *> name_space_list;

// --------------------------------------------------------------------

/// element is the most important zeep::xml::node object. It encapsulates a
/// XML element as found in the XML document. It has a qname, can have children,
/// attributes and a namespace.

class element : public container
{
public:
	element(const std::string& qname);
	~element();

	virtual void write(writer& w) const;

	virtual bool equals(const node* n) const;

	virtual node* clone() const;

	virtual std::string str() const;
	virtual void str(const std::string& value) { content(value); }

	virtual void write_content(std::ostream& os, const char* sep = kWhiteSpaceChar) const;

	std::string qname() const { return m_qname; }

	std::string namespace_for_prefix(const std::string& prefix) const;
	std::string prefix_for_namespace(const std::string& uri) const;

	std::string content() const;
	void content(const std::string& content);

	std::string get_attribute(const std::string& qname) const;
	attribute* get_attribute_node(const std::string& qname) const;
	/// the DOCTYPE can specify some attributes as ID
	void set_attribute(const std::string& qname, const std::string& value, bool id = false);
	void remove_attribute(const std::string& qname);
	void remove_attribute(attribute* attr);

	/// to set the default namespace, pass an empty string as prefix
	void set_name_space(const std::string& prefix,
						const std::string& uri);
	//	void				remove_name_space(const std::string& uri);

	/// The add_text method checks if the last added child is a text node,
	/// and if so, it appends the string to this node's value. Otherwise,
	/// it adds a new text node child with the new text.
	void add_text(const std::string& s);

	/// The set_text method replaces any text node with the new text
	void set_text(const std::string& s);

	/// to iterate over the attribute nodes
	attribute_set attributes() const;
	/// to iterate over the namespace nodes
	name_space_list name_spaces() const;

	/// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string lang() const;

	/// content of the xml:id attribute, or the attribute that was defined to be
	/// of type ID by the DOCTYPE.
	std::string id() const;

	/// as a service to the user, we define an attribute iterator here
	class attribute_iterator : public std::iterator<std::bidirectional_iterator_tag, attribute>
	{
	public:
		attribute_iterator() : m_current(nullptr) {}
		attribute_iterator(attribute* e) : m_current(e) {}
		attribute_iterator(const attribute_iterator& other)
			: m_current(other.m_current) {}

		attribute_iterator& operator=(const attribute_iterator& other)
		{
			m_current = other.m_current;
			return* this;
		}

		reference operator*() const { return* m_current; }
		pointer operator->() const { return m_current; }

		attribute_iterator& operator++()
		{
			m_current = dynamic_cast<attribute *>(m_current->next());
			return* this;
		}
		attribute_iterator operator++(int)
		{
			attribute_iterator iter(*this);
			operator++();
			return iter;
		}

		attribute_iterator& operator--()
		{
			m_current = dynamic_cast<attribute *>(m_current->prev());
			return* this;
		}
		attribute_iterator operator--(int)
		{
			attribute_iterator iter(*this);
			operator++();
			return iter;
		}

		bool operator==(const attribute_iterator& other) const { return m_current == other.m_current; }
		bool operator!=(const attribute_iterator& other) const { return m_current != other.m_current; }

		pointer base() const { return m_current; }

	private:
		attribute* m_current;
	};

	attribute_iterator attr_begin() { return attribute_iterator(m_attribute); }
	attribute_iterator attr_end() { return attribute_iterator(); }

	class const_attribute_iterator : public std::iterator<std::bidirectional_iterator_tag, const attribute>
	{
	public:
		const_attribute_iterator() : m_current(nullptr) {}
		const_attribute_iterator(attribute* e) : m_current(e) {}
		const_attribute_iterator(const attribute_iterator& other)
			: m_current(other.base()) {}
		const_attribute_iterator(const const_attribute_iterator& other)
			: m_current(other.m_current) {}

		const_attribute_iterator& operator=(const const_attribute_iterator& other)
		{
			m_current = other.m_current;
			return* this;
		}

		reference operator*() const { return* m_current; }
		pointer operator->() const { return m_current; }

		const_attribute_iterator &
		operator++()
		{
			m_current = dynamic_cast<const attribute *>(m_current->next());
			return* this;
		}
		const_attribute_iterator
		operator++(int)
		{
			const_attribute_iterator iter(*this);
			operator++();
			return iter;
		}

		const_attribute_iterator &
		operator--()
		{
			m_current = dynamic_cast<const attribute *>(m_current->prev());
			return* this;
		}
		const_attribute_iterator
		operator--(int)
		{
			const_attribute_iterator iter(*this);
			operator++();
			return iter;
		}

		bool operator==(const const_attribute_iterator& other) const { return m_current == other.m_current; }
		bool operator!=(const const_attribute_iterator& other) const { return m_current != other.m_current; }

		pointer base() const { return m_current; }

	private:
		const attribute* m_current;
	};

	const_attribute_iterator attr_begin() const { return const_attribute_iterator(m_attribute); }
	const_attribute_iterator attr_end() const { return const_attribute_iterator(); }

#ifndef LIBZEEP_DOXYGEN_INVOKED
protected:
	void add_name_space(name_space* ns);

	std::string m_qname;
	attribute* m_attribute;
	name_space* m_name_space;
#endif
};

/// This is probably only useful for debugging purposes
std::ostream& operator<<(std::ostream& lhs, const node& rhs);

bool operator==(const node& lhs, const node& rhs);

/// very often, we want to iterate over child elements of an element
/// therefore we have a templated version of children.

template <>
std::list<node *> container::children<node>() const;

template <>
std::list<container *> container::children<container>() const;

template <>
std::list<element *> container::children<element>() const;

// iterator inlines, specialised by the two types

template <>
inline container::basic_iterator<element> &container::basic_iterator<element>::operator++()
{
	if (m_current == nullptr or m_current->next() == nullptr)
		m_current = nullptr;
	else
	{
		for (node* n = m_current->next(); n != nullptr; n = n->next())
		{
			m_current = dynamic_cast<element *>(n);
			if (m_current != nullptr)
				break;
		}
	}
	return* this;
}

template <>
inline container::basic_iterator<element> &container::basic_iterator<element>::operator--()
{
	if (m_current == nullptr or m_current->prev() == nullptr)
		m_current = nullptr;
	else
	{
		for (node* n = m_current->prev(); n != nullptr; n = n->prev())
		{
			m_current = dynamic_cast<element *>(n);
			if (m_current != nullptr)
				break;
		}
	}
	return* this;
}

template <>
inline container::basic_iterator<const element> &container::basic_iterator<const element>::operator++()
{
	if (m_current == nullptr or m_current->next() == nullptr)
		m_current = nullptr;
	else
	{
		for (const node* n = m_current->next(); n != nullptr; n = n->next())
		{
			m_current = dynamic_cast<const element *>(n);
			if (m_current != nullptr)
				break;
		}
	}
	return* this;
}

template <>
inline container::basic_iterator<const element> &container::basic_iterator<const element>::operator--()
{
	if (m_current == nullptr or m_current->prev() == nullptr)
		m_current = nullptr;
	else
	{
		for (const node* n = m_current->prev(); n != nullptr; n = n->prev())
		{
			m_current = dynamic_cast<const element *>(n);
			if (m_current != nullptr)
				break;
		}
	}
	return* this;
}

inline container::iterator container::begin()
{
	element* first = nullptr;

	for (node* n = m_child; n != nullptr; n = n->next())
	{
		first = dynamic_cast<element *>(n);
		if (first != nullptr)
			break;
	}
	return iterator(first);
}

inline container::node_iterator container::node_begin()
{
	return node_iterator(m_child);
}

inline container::const_iterator container::begin() const
{
	const element* first = nullptr;

	for (const node* n = m_child; n != nullptr; n = n->next())
	{
		first = dynamic_cast<const element *>(n);
		if (first != nullptr)
			break;
	}
	return const_iterator(first);
}

template <>
inline container::basic_iterator<node> &container::basic_iterator<node>::operator++()
{
	assert(m_current != nullptr);
	m_current = m_current->next();
	return* this;
}

template <>
inline container::basic_iterator<node> &container::basic_iterator<node>::operator--()
{
	assert(m_current != nullptr);
	m_current = m_current->prev();
	return* this;
}

template <>
inline container::basic_iterator<const node> &container::basic_iterator<const node>::operator++()
{
	assert(m_current != nullptr);
	m_current = m_current->next();
	return* this;
}

template <>
inline container::basic_iterator<const node> &container::basic_iterator<const node>::operator--()
{
	assert(m_current != nullptr);
	m_current = m_current->prev();
	return* this;
}

//inline container::iterator container::begin()
//{
//	element* first = nullptr;
//
//	for (node* n = m_child; n != nullptr; n = n->next())
//	{
//		first = dynamic_cast<element*>(n);
//		if (first != nullptr)
//			break;
//	}
//	return iterator(first);
//}
//
//inline container::const_iterator container::begin() const
//{
//	const element* first = nullptr;
//
//	for (const node* n = m_child; n != nullptr; n = n->next())
//	{
//		first = dynamic_cast<const element*>(n);
//		if (first != nullptr)
//			break;
//	}
//	return const_iterator(first);
//}

template <class NodeType>
container::basic_iterator<NodeType>
container::insert(basic_iterator<NodeType> position, NodeType* n)
{
	insert(*position, n);
	return basic_iterator<NodeType>(n);
}

template <class Iterator>
void container::insert(Iterator position, Iterator first, Iterator last)
{
	node* p = *position;
	for (Iterator i = first; i != last; ++i)
	{
		insert(p, *i);
		p = *i;
	}
}

template <class Iterator>
void container::erase(Iterator position)
{
	node* n = *position;

	remove(n);
	delete n;
}

template <class Iterator>
void container::erase(Iterator first, Iterator last)
{
	while (first != last)
	{
		node* n = *first++;
		remove(n);
		delete n;
	}
}

} // namespace xml
} // namespace zeep

#endif
