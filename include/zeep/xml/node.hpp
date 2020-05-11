// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>

#include <memory>
#include <list>
#include <iterator>
#include <string>
#include <limits>
#include <list>
#include <set>

#include <zeep/config.hpp>
#include <zeep/exception.hpp>

namespace zeep
{
namespace xml
{

class node;
class element;
class text;
class attribute;
class name_space;
class comment;
class cdata;
class processing_instruction;
class document;

template<typename> class basic_node_list;

using node_set = std::list<node*>;
using element_set = std::list<element*>;

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
///
/// Nodes are stored in a node_list, a generic list class that resembles std::list

class node
{
  public:
	friend class element;
	friend class document;
	template<typename> friend class basic_node_list;
	friend class node_list;

	using parent_type = element;

	virtual ~node();

	/// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string lang() const;

	/// Nodes can have a name, and the XPath specification requires that a node can
	/// have a so-called expanded-name. This name consists of a local-name and a
	/// namespace which is a URI. And we can have a QName which is a concatenation of
	/// a prefix (that points to a namespace URI) and a local-name separated by a colon.
	///
	/// To reduce storage requirements, names are stored in nodes as qnames, if at all.
	virtual std::string get_qname() const;
	virtual void set_qname(const std::string& qn)
	{
		assert(false);
	}

	virtual void set_qname(const std::string& prefix, const std::string& name)
	{
		set_qname(prefix.empty() ? name : prefix + ':' + name);
	}

	virtual std::string name() const;   ///< The name for the node as parsed from the qname.
	virtual std::string get_prefix() const; ///< The prefix for the node as parsed from the qname.
	virtual std::string get_ns() const;		///< Returns the namespace URI for the node, if it can be resolved.

	virtual std::string namespace_for_prefix(const std::string& prefix) const;
	///< Return the namespace URI for a prefix

	virtual std::pair<std::string,bool> prefix_for_namespace(const std::string& uri) const;
	///< Return the prefix for a namespace URI

	virtual std::string prefix_tag(const std::string& tag, const std::string& uri) const;
	///< Prefix the \a tag with the namespace prefix for \a uri

	/// return all content concatenated, including that of children.
	virtual std::string str() const = 0;

	/// Set text, what really happens depends on the type of the subclass implementing this method
	virtual void set_text(const std::string& value) = 0;

	// --------------------------------------------------------------------
	// low level routines

	// basic access

	// All nodes should have a single root node
	virtual element* root(); ///< The root node for this node
	virtual const element* root() const; ///< The root node for this node

	element* parent() { return m_parent; }				///< The parent node for this node
	const element* parent() const { return m_parent; }	///< The parent node for this node

	node* next() { return m_next; }				///< The next sibling
	const node* next() const { return m_next; } ///< The next sibling

	node* prev() { return m_prev; }				///< The previous sibling
	const node* prev() const { return m_prev; } ///< The previous sibling

	/// Compare the node with \a n
	virtual bool equals(const node* n) const;

	/// debug routine
	virtual void validate();

	/// return an exact copy of this node, including all data in sub nodes
	virtual node* clone() const = 0;

	/// return a copy of this node, including all data in sub nodes, but
	/// in contrast with clone the data is moved from this node to the cloned
	/// node. This node will be empty afterwards.
	virtual node* move() = 0;

	struct format_info
	{
		bool indent = false;
		bool indent_attributes = false;
		bool collapse_tags = true;
		bool suppress_comments = false;
		bool escape_white_space = false;
		bool escape_double_quote = true;
		size_t indent_width = 0;
		size_t indent_level = 0;
		float version = 1.0f;
	};

	virtual void write(std::ostream& os, format_info fmt) const = 0;

#ifndef LIBZEEP_DOXYGEN_INVOKED
  protected:

	friend class element;

	node() = default;
	node(const node& n) = delete;
	node(node&& n) = delete;
	node& operator=(const node& n) = delete;
	node& operator=(node&& n) = delete;

	virtual void insert_sibling(node* n, node* before);
	virtual void remove_sibling(node* n);

	void parent(element* p);
	void next(node* n);
	void prev(node* n);

  protected:
	element* m_parent = nullptr;
	node* m_next = nullptr;
	node* m_prev = nullptr;
#endif
};

// --------------------------------------------------------------------
/// internal node just for storing text

class node_with_text : public node
{
  public:
	node_with_text() {}
	node_with_text(const std::string& s) : m_text(s) {}

	virtual std::string str() const { return m_text; }

	virtual std::string get_text() const { return m_text; }
	virtual void set_text(const std::string& text) { m_text = text; }

  protected:
	std::string m_text;
};

// --------------------------------------------------------------------
/// A node containing a XML comment

class comment : public node_with_text
{
  public:
	comment() {}
	comment(comment&& c) : node_with_text(std::move(c.m_text)) {}
	comment(const std::string& text) : node_with_text(text) {}

	virtual bool equals(const node* n) const;

	virtual node* clone() const;
	virtual node* move();

  protected:

	virtual void write(std::ostream& os, format_info fmt) const;
};

// --------------------------------------------------------------------
/// A node containing a XML processing instruction (like e.g. \<?php ?\>)

class processing_instruction : public node_with_text
{
  public:
	processing_instruction() {}

	processing_instruction(processing_instruction&& pi)
		: node_with_text(std::move(pi.m_text))
		, m_target(std::move(pi.m_target))
	{}

	processing_instruction(const std::string& target, const std::string& text)
		: node_with_text(text), m_target(target) {}

	virtual std::string get_qname() const { return m_target; }

	std::string get_target() const { return m_target; }
	void set_target(const std::string& target) { m_target = target; }

	virtual bool equals(const node* n) const;

	virtual node* clone() const;
	virtual node* move();

  protected:

	virtual void write(std::ostream& os, format_info fmt) const;

  private:
	std::string m_target;
};

// --------------------------------------------------------------------
/// A node containing text.

class text : public node_with_text
{
  public:
	text() {}

	text(text&& t)
		: node_with_text(std::move(t.m_text)) {}

	text(const std::string& text)
		: node_with_text(text) {}

	/// \brief append \a text to the stored text
	void append(const std::string& text) { m_text.append(text); }

	virtual bool equals(const node* n) const;

	/// \brief returns true if this text contains only whitespace characters
	bool is_space() const;

	virtual node* clone() const;
	virtual node* move();

  protected:

	virtual void write(std::ostream& os, format_info fmt) const;
};

// --------------------------------------------------------------------
/// A node containing the contents of a CDATA section. Normally, these nodes are
/// converted to text nodes but you can specify to preserve them when parsing a
/// document.

class cdata : public text
{
  public:
	cdata() {}
	cdata(cdata&& cd) : text(std::move(cd)) {}
	cdata(const std::string& s)	: text(s) {}

	virtual bool equals(const node* n) const;

	virtual node* clone() const;
	virtual node* move();

  protected:

	virtual void write(std::ostream& os, format_info fmt) const;
};

// --------------------------------------------------------------------
/// An attribute is a node, has an element as parent, but is not a child of this parent (!)

class attribute : public node
{
  public:
	friend class element;

	using parent_type = element;

	attribute(const attribute& attr)
		: m_qname(attr.m_qname), m_value(attr.m_value), m_id(attr.m_id) {}

	attribute(attribute&& attr)
		: m_qname(std::move(attr.m_qname)), m_value(std::move(attr.m_value)), m_id(attr.m_id) {}

	attribute(const std::string& qname, const std::string& value, bool id = false)
		: m_qname(qname), m_value(value), m_id(id) {}
	
	attribute& operator=(attribute&& attr)
	{
		std::swap(m_qname, attr.m_qname);
		std::swap(m_value, attr.m_value);
		m_id = attr.m_id;
		return *this;
	}

	bool operator==(const attribute& a) const
	{
		return m_qname == a.m_qname and m_value == a.m_value;
	}

	bool operator!=(const attribute& a) const
	{
		return not operator==(a);
	}

	bool operator<(const attribute& ns) const
	{
		return m_qname < ns.m_qname;
	}

	virtual std::string get_qname() const { return m_qname; }
	virtual void set_qname(const std::string& qn) { m_qname = qn; }

	virtual void set_qname(const std::string& prefix, const std::string& name)
	{
		node::set_qname(prefix, name);
	}

	/// \b Is this attribute an xmlns attribute?
	bool is_namespace() const
	{
		return m_qname.compare(0, 5, "xmlns") == 0 and (m_qname[5] == 0 or m_qname[5] == ':');
	}

	std::string value() const { return m_value; }
	void value(const std::string& v) { m_value = v; }

	/// \b same as value, but checks to see if this really is a namespace attribute
	std::string uri() const;

	virtual std::string str() const { return m_value; }

	virtual void set_text(const std::string& value) { m_value = value; }

	virtual bool equals(const node* n) const;

	/// \brief returns whether this attribute is an ID attribute, as defined in an accompanying DTD
	virtual bool is_id() const { return m_id; }

	template<size_t N>
	decltype(auto) get() const
	{
		     if constexpr (N == 0) return name();
		else if constexpr (N == 1) return value();
	}

	void swap(attribute& a)
	{
		std::swap(m_qname, a.m_qname);
		std::swap(m_value, a.m_value);
	}

	virtual node* clone() const;
	virtual node* move();

  protected:

	virtual void write(std::ostream& os, format_info fmt) const;

  private:
	std::string m_qname, m_value;
	bool m_id;
};

// --------------------------------------------------------------------
/// \brief generic iterator class.
///
/// We have two container classes (node_list specializations)
/// One is for attributes and name_spaces. The other is the
/// node_list for nodes in elements. However, this list can
/// present itself as node_list for elements.

template<typename NodeType, typename ContainerNodeType = std::remove_const_t<NodeType>>
class iterator_impl
{
  public:
	friend class element;
	template<typename> friend class basic_node_list;
	template<typename,typename> friend class iterator_impl;
	friend class node_list;

	using node_type = NodeType;

	using container_node_type = std::remove_cv_t<ContainerNodeType>;
	using container_type = basic_node_list<container_node_type>;

	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = node_type;
	using difference_type = std::ptrdiff_t;
	using pointer = value_type*;
	using reference = value_type&;

	iterator_impl() = default;

	iterator_impl(const iterator_impl& i) = default;

	template<typename OtherNodeType, typename OtherContainerNodeType>
	iterator_impl(const iterator_impl<OtherNodeType, OtherContainerNodeType>& i)
		: m_container(i.m_container)
		, m_current(i.m_current)
		, m_at_end(i.m_at_end)
	{
		skip();
		if (m_current == nullptr)
			m_at_end = true;
	}

	// create iterator pointing to begin of parent element
	iterator_impl(const container_type& container)
		: m_container(&container), m_at_end(false)
	{
		m_current = m_container->m_head;

		skip();

		m_at_end = m_current == nullptr;
	}

	// create iterator pointing to end of parent element
	iterator_impl(const container_type& container, node_type* current)
		: m_container(&container), m_current(const_cast<std::remove_cv_t<node_type>*>(current)), m_at_end(true)
	{
		assert(current == nullptr or dynamic_cast<node_type*>(current) != nullptr);
#if DEBUG
		if (m_current != nullptr)
		{
			const node* n;
			for (n = m_container->m_head; n != nullptr; n = n->next())
			{
				if (n == m_current)
					break;
			}
			assert(n == current);
		}
#endif
	}

	iterator_impl(node_type* current)
		: m_container(&current->parent()->nodes())
		, m_current(const_cast<std::remove_const_t<node_type>*>(current))
		, m_at_end(current == nullptr)
	{
		assert(current == nullptr or dynamic_cast<node_type*>(current) != nullptr);
#if DEBUG
		if (m_current != nullptr)
		{
			const node* n;
			for (n = m_container->m_head; n != nullptr; n = n->next())
			{
				if (n == m_current)
					break;
			}
			assert(n == current);
		}
#endif
	}

	iterator_impl(iterator_impl&& i)
		: m_container(i.m_container)
		, m_current(i.m_current)
		, m_at_end(i.m_at_end)
	{
		i.m_container = nullptr;
		i.m_current = nullptr;
		i.m_at_end = true;
	}

	template<typename Iterator, std::enable_if_t<
			not std::is_same_v<std::remove_const_t<typename Iterator::value_type>, element> and
			std::is_base_of<value_type, typename Iterator::value_type>::value, int> = 0>
	iterator_impl(const Iterator& i)
		: m_container(const_cast<container_type*>(i.m_container))
		, m_current(i.m_current)
		, m_at_end(i.m_at_end) {}

	iterator_impl& operator=(const iterator_impl& i)
	{
		if (this != &i)
		{
			m_container = i.m_container;
			m_current = i.m_current;
			m_at_end = i.m_at_end;
		}
		return *this;
	}

	iterator_impl& operator=(iterator_impl&& i)
	{
		if (this != &i)
		{
			m_container = i.m_container;	i.m_container = nullptr;
			m_current = i.m_current;		i.m_current = nullptr;
			m_at_end = i.m_at_end;			i.m_at_end = true;
		}
		return *this;
	}

	template<typename Iterator, std::enable_if_t<
			std::is_base_of<value_type, typename Iterator::value_type>::value, int> = 0>
	iterator_impl& operator=(const Iterator& i)
	{
		m_container = i.m_container;
		m_current = i.m_current;
		m_at_end = i.m_at_end;
		return *this;
	}

	reference operator*()			{ return *current(); }
	pointer operator->() const		{ return current(); }

	iterator_impl& operator++()
	{
		if (not m_at_end and m_current == nullptr and m_container != nullptr)
			m_current = m_container->m_head;
		else if (m_current != nullptr)
		{
			m_current = m_current->next();
			skip();
		}

		m_at_end = m_current == nullptr;
		return *this;
	}

	iterator_impl operator++(int)
	{
		iterator_impl iter(*this);
		operator++();
		return iter;
	}

	iterator_impl& operator--()
	{
		if (m_container != nullptr)
		{
			if (m_at_end)
			{
				m_current = m_container->m_tail;
				m_at_end = false;
			}
			else
			{
				while (m_current != nullptr)
				{
					m_current = m_current->prev();
					if (dynamic_cast<node_type*>(m_current) != nullptr)
						break;
				}
			}
		}
		return *this;
	}
	
	iterator_impl operator--(int)
	{
		iterator_impl iter(*this);
		operator--();
		return iter;
	}

	bool operator==(const iterator_impl& other) const
	{
		return m_container == other.m_container and
			m_at_end == other.m_at_end and m_current == other.m_current;
	}

	bool operator!=(const iterator_impl& other) const
	{
		return not operator==(other);
	}

	template <class RNodeType>
	bool operator==(const RNodeType* n) const { return m_current == n; }

	template <class RNodeType>
	bool operator!=(const RNodeType n) const { return m_current != n; }

	iterator_impl& operator+=(difference_type i)
	{
		if (i > 0)
			while (i-- > 0) operator++();
		else
			while (i++ < 0) operator--();
		return *this;
	}

	iterator_impl& operator-=(difference_type i)
	{
		operator+=(-i);
		return *this;
	}

	iterator_impl operator+(difference_type i) const
	{
		auto result = *this;
		result += i;
		return result;
	}

	friend iterator_impl operator+(difference_type i, const iterator_impl& iter)
	{
		auto result = iter;
		result += i;
		return result;
	}

	iterator_impl operator-(difference_type i) const
	{
		auto result = *this;
		result -= i;
		return result;
	}

	friend iterator_impl operator-(difference_type i, const iterator_impl& iter)
	{
		auto result = iter;
		result -= i;
		return result;
	}

	difference_type operator-(const iterator_impl& other) const
	{
		return std::distance(*this, other);
	}

	operator const pointer() const	{ return current(); }
	operator pointer()				{ return current(); }

  private:
	// node_type* current()			{ return dynamic_cast<node_type*>(m_current); }
	node_type* current() const		{ return dynamic_cast<node_type*>(m_current); }

	inline void skip() {}

	const container_type*	m_container = nullptr;
	node*					m_current = nullptr;
	bool					m_at_end = true;
};

template<> void iterator_impl<element,node>::skip();
template<> void iterator_impl<const element,node>::skip();

// --------------------------------------------------------------------
// basic_node_list, a base class for containers of nodes

template<typename NodeType>
class basic_node_list
{
  public:
	template<typename,typename> friend class iterator_impl;
	friend class element;

	using node_type = NodeType;

	// element is a container of elements
	using value_type = node_type;
	using allocator_type = std::allocator<value_type>;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

  protected:

	basic_node_list(element& e)
		: m_element(e), m_head(nullptr), m_tail(nullptr)
	{
	}

  public:

	virtual ~basic_node_list()
	{
		delete m_head;
	}

	bool operator==(const basic_node_list& l) const
	{
		bool result = true;
		auto a = begin(), b = l.begin();
		for (; result and a != end() and b != l.end(); ++a, ++b)
			result = a->equals(b.current());
		return result and a == end() and b == l.end();
	}

	bool operator!=(const basic_node_list& l) const
	{
		return not operator==(l);
	}

	using iterator = iterator_impl<node_type>;
	using const_iterator = iterator_impl<const node_type>;

	iterator begin()				{ return iterator(*this); }
	iterator end()					{ return iterator(*this, nullptr); }

	const_iterator cbegin()			{ return const_iterator(*this); }
	const_iterator cend()			{ return const_iterator(*this, nullptr); }

	const_iterator begin() const	{ return const_iterator(*this); }
	const_iterator end() const		{ return const_iterator(*this, nullptr); }

	value_type& front()				{ return *begin(); }
	const value_type& front() const	{ return *begin(); }

	value_type& back()				{ auto tmp = end(); --tmp; return *tmp; }
	const value_type& back() const	{ auto tmp = end(); --tmp; return *tmp; }

	bool empty() const				{ return m_head == nullptr; }
	size_t size() const				{ return std::distance(begin(), end()); }

	void clear()
	{
		delete m_head;
		m_head = m_tail = nullptr;
	}

	void swap(basic_node_list& l) noexcept
	{
		std::swap(m_head, l.m_head);
		std::swap(m_tail, l.m_tail);

		for (auto& n: *this)
			n.m_parent = &m_element;
		
		for (auto& n: l)
			n.m_parent = &l.m_element;
	}

	template<typename Compare>
	void sort(Compare comp)
	{
		for (auto a = begin(); a + 1 != end(); ++a)
		{
			for (auto b = a + 1; b != end(); ++b)
			{
				if (comp(*b, *a))
					a->swap(*b);
			}
		}
	}

  protected:
	// proxy methods for every insertion

	iterator insert_impl(const_iterator pos, node_type* n)
	{
		assert(n != nullptr);
		assert(n->next() == nullptr);
		assert(n->prev() == nullptr);
		// assert(&pos.m_container == this);

		if (n == nullptr)
			throw exception("Invalid pointer passed to insert");

		if (n->parent() != nullptr or n->next() != nullptr or n->prev() != nullptr)
			throw exception("attempt to add a node that already has a parent or siblings");
		
		n->parent(&m_element);

		// insert at end, most often this is the case
		if (pos.m_current == nullptr)
		{
			if (m_head == nullptr)
				m_tail = m_head = n;
			else
			{
				m_tail->insert_sibling(n, nullptr);
				m_tail = n;
			}
		}
		else
		{
			assert(m_head != nullptr);

			if (pos.m_current == m_head)
			{
				n->m_next = m_head;
				m_head->m_prev = n;
				m_head = n;
			}
			else
				m_head->insert_sibling(n, pos.m_current);
		}

// #if defined(DEBUG)
// 		validate();
// #endif

		return iterator(*this, n);
	}

	iterator erase_impl(const_iterator pos)
	{
		if (pos == end())
			return pos;

		if (pos->m_parent != &m_element)
			throw exception("attempt to remove node whose parent is invalid");

		node_type* n = const_cast<node_type*>(&*pos);
		node_type* cur;

		if (m_head == n)
		{
			m_head = static_cast<node_type*>(m_head->m_next);
			if (m_head != nullptr)
				m_head->m_prev = nullptr;
			else
				m_tail = nullptr;
			
			n->m_next = n->m_prev = n->m_parent = nullptr;
			delete n;

			cur = m_head;
		}
		else
		{
			cur = static_cast<node_type*>(n->m_next);

			if (m_tail == n)
				m_tail = static_cast<node_type*>(n->m_prev);

			node* p = m_head;
			while (p != nullptr and p->m_next != n)
				p = p->m_next;

			if (p != nullptr and p->m_next == n)
			{
				p->m_next = n->m_next;
				if (p->m_next != nullptr)
					p->m_next->m_prev = p;
				n->m_next = n->m_prev = n->m_parent = nullptr;
			}
			else
				throw exception("remove for a node not found in the list");

			delete n;
		}

// #if defined(DEBUG)
// 		validate();
// #endif

		return iterator(*this, cur);
	}

  private:
	element&	m_element;
	node_type*	m_head = nullptr;
	node_type*	m_tail = nullptr;
};

// --------------------------------------------------------------------
// node_list

class node_list : public basic_node_list<node>
{
  public:

	using basic_list = basic_node_list;

	using iterator = typename basic_list::iterator;
	using const_iterator = typename basic_list::const_iterator;

	node_list(element& e)
		: basic_list(e)
	{
	}

	node_list(element& e, const node_list& l)
		: basic_list(e)
	{
		for (auto& n: l)
			emplace_back(n);
	}

	node_list(element& e, node_list&& l)
		: basic_list(e)
	{
		for (auto&& n: l)
			emplace_back(std::move(n));
	}

	using basic_list::clear;
	using basic_list::begin;
	using basic_list::end;
	
	node_list& operator=(const node_list& l)
	{
		if (this != &l)
		{
			clear();

			for (auto& n: l)
				emplace_back(n);
		}

		return *this;
	}

	node_list& operator=(node_list&& l)
	{
		if (this != &l)
		{
			clear();
			swap(l);
		}

		return *this;
	}

	bool operator==(const node_list& l) const
	{
		bool result = true;
		auto a = begin(), b = l.begin();
		for (; result and a != end() and b != l.end(); ++a, ++b)
			result = a->equals(b.current());
		return result and a == end() and b == l.end();
	}

	bool operator!=(const node_list& l) const
	{
		return not operator==(l);
	}

	// insert a copy of e
	void insert(const_iterator pos, const node& e)
	{
		insert_impl(pos, e.clone());
	}

	// insert a copy of e, moving its data
	void insert(const_iterator pos, node&& e)
	{
		insert_impl(pos, e.move());
	}

	// iterator insert(const_iterator pos, size_t count, const value_type& n);

	template<typename InputIter>
	iterator insert(const_iterator pos, InputIter first, InputIter last)
	{
		for (auto i = first; i != last; ++i, ++pos)
			insert(pos, i->clone());
		return pos;
	}

	template<typename InsertNodeType>
	iterator insert(const_iterator pos, std::initializer_list<InsertNodeType> nodes)
	{
		for (auto& n: nodes)
			pos = insert_impl(pos, n.move());
		return pos;
	}

	iterator emplace(const_iterator pos, const node& n)
	{
		auto i = insert_impl(pos, n.clone());
		return iterator(*this, i);
	}

	iterator emplace(const_iterator pos, node&& n)
	{
		auto i = insert_impl(pos, n.move());
		return iterator(*this, i);
	}

	iterator erase(const_iterator pos)
	{
		return erase_impl(pos);
	}

	iterator erase(iterator first, iterator last)
	{
		while (first != last)
		{
			auto next = first;
			++next;

			erase(first);
			first = next;
		}
		return last;
	}

	// iterator erase(const_iterator first, const_iterator last);

	void push_front(const node& e)
	{
		emplace(begin(), e);
	}

	void push_front(node&& e)
	{
		emplace(begin(), std::forward<node>(e));
	}

	template<typename... Args>
	node& emplace_front(Args&&... args)
	{
		return *emplace(begin(), std::forward<Args>(args)...);
	}

	void pop_front()
	{
		erase(begin());
	}

	void push_back(const node& e)
	{
		emplace(end(), e);
	}

	void push_back(node&& e)
	{
		emplace(end(), std::forward<node>(e));
	}

	template<typename ENodeType, std::enable_if_t<std::is_base_of_v<node,ENodeType>, int> = 0>
	ENodeType& emplace_back(const ENodeType& n)
	{
		auto i = insert_impl(end(), static_cast<ENodeType*>(n.clone()));
		return static_cast<ENodeType&>(*i);
	}

	template<typename ENodeType, std::enable_if_t<std::is_base_of_v<node,ENodeType>, int> = 0>
	ENodeType& emplace_back(ENodeType&& n)
	{
		auto i = insert_impl(end(), static_cast<ENodeType*>(n.move()));
		return static_cast<ENodeType&>(*i);
	}

	template<typename... Args>
	node& emplace_back(Args&&... args)
	{
		return *emplace(end(), std::forward<Args>(args)...);
	}

	void pop_back()
	{
		erase(std::prev(end()));
	}
};

// --------------------------------------------------------------------
/// set of attributes of name_spaces. Is a node_list but with a set interface

class attribute_set : public basic_node_list<attribute>
{
  public:
	using node_list = basic_node_list<attribute>;

	using node_type = typename node_list::node_type;
	using iterator = typename node_list::iterator;
	using const_iterator = typename node_list::const_iterator;
	using size_type = std::size_t;

	attribute_set(element& e) : node_list(e) {}

	attribute_set(element& e, attribute_set&& as)
		: node_list(e)
	{
		for (auto& a: as)
			emplace(std::move(a));
	}

	attribute_set(element& e, const attribute_set& as)
		: node_list(e)
	{
		for (auto& a: as)
			emplace(a);
	}

	using node_list::clear;

	attribute_set& operator=(const attribute_set& l)
	{
		if (this != &l)
		{
			clear();

			for (auto& n: l)
				emplace(n);
		}

		return *this;
	}

	attribute_set& operator=(attribute_set&& l)
	{
		if (this != &l)
		{
			clear();
			swap(l);
		}

		return *this;
	}


	using key_type = std::string;

	bool contains(const key_type& key) const
	{
		return find(key) != nullptr;
	}

	const_iterator find(const key_type& key) const
	{
		const node_type* result = nullptr;
		for (auto& a: *this)
		{
			if (a.get_qname() == key)
			{
				result = &a;
				break;
			}
		}
		return const_iterator(*this, result);
	}

	iterator find(const key_type& key)
	{
		return const_cast<const attribute_set&>(*this).find(key);
	}

	template<typename... Args>
	std::pair<iterator,bool> emplace(Args... args)
	{
		node_type a(std::forward<Args>(args)...);
		return emplace(std::move(a));
	}

	std::pair<iterator,bool> emplace(node_type&& a)
	{
		key_type key = a.get_qname();
		bool inserted = false;

		auto i = find(key);
		if (i != node_list::end())
			*i = std::move(a);	// move assign value of a
		else
		{
			i = node_list::insert_impl(node_list::end(), static_cast<node_type*>(a.move()));
			inserted = true;
		}

		return std::make_pair(i, inserted);
	}

	iterator erase(const_iterator pos)
	{
		return node_list::erase_impl(pos);
	}

	iterator erase(iterator first, iterator last)
	{
		while (first != last)
		{
			auto next = first;
			++next;

			erase(first);
			first = next;
		}
		return last;
	}

	size_type erase(const key_type key)
	{
		size_type result = 0;
		auto i = find(key);
		if (i != node_list::end())
		{
			erase(i);
			result = 1;
		}
		return result;
	}
};

// --------------------------------------------------------------------
/// element is the most important zeep::xml::node object. It encapsulates a
/// XML element as found in the XML document. It has a qname, can have children,
/// attributes and a namespace.

class element : public node
{
  public:
	template<typename,typename> friend class iterator_impl;
	template<typename> friend class basic_node_list;
	friend class node_list;
	friend class node;

	// element is a container of elements
	using value_type = element;
	using allocator_type = std::allocator<element>;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using reference = element&;
	using const_reference = const element&;
	using pointer = element*;
	using const_pointer = const element*;

	element();
	element(const std::string& qname);
	element(const std::string& qname, std::initializer_list<attribute> attributes);
	element(const element& e);
	element(element&& e);
	element& operator=(const element& e);
	element& operator=(element&& e);

	~element();

	using node::set_qname;

	virtual std::string get_qname() const { return m_qname; }
	virtual void set_qname(const std::string& qn) { m_qname = qn; }

	/// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string lang() const;

	/// content of the xml:id attribute, or the attribute that was defined to be
	/// of type ID by the DOCTYPE.
	std::string id() const;

	bool operator==(const element& e) const		{ return equals(&e); }
	bool operator!=(const element& e) const		{ return not equals(&e); }

	virtual bool equals(const node* n) const;

	void swap(element& e) noexcept;

	// --------------------------------------------------------------------
	// children

	node_list& nodes()												{ return m_nodes; }
	const node_list& nodes() const									{ return m_nodes; }

	using iterator = iterator_impl<element,node>;
	using const_iterator = iterator_impl<const element,node>;

	iterator begin()												{ return iterator(m_nodes.begin()); }
	iterator end()													{ return iterator(m_nodes.end()); }

	const_iterator begin() const									{ return const_iterator(m_nodes.begin()); }
	const_iterator end() const										{ return const_iterator(m_nodes.end()); }

	const_iterator cbegin()											{ return const_iterator(m_nodes.begin()); }
	const_iterator cend()											{ return const_iterator(m_nodes.end()); }

	element& front()												{ return *begin(); }
	const element& front() const									{ return *begin(); }

	element& back()													{ return *(end() - 1); }
	const element& back() const										{ return *(end() - 1); }

	using node_iterator = node_list::iterator;
	using const_node_iterator = node_list::const_iterator;

	// insert a copy of e
	void insert(const_iterator pos, const element& e)
	{
		emplace(pos, e);
	}

	// insert a copy of e, moving its data
	void insert(const_iterator pos, element&& e)
	{
		emplace(pos, std::forward<element>(e));
	}

	// iterator insert(const_iterator pos, size_t count, const value_type& n);

	template<typename InputIter>
	iterator insert(const_iterator pos, InputIter first, InputIter last)
	{
		difference_type offset = pos - cbegin();
		for (auto i = first; i != last; ++i, ++pos)
			insert(pos, *i);
		return begin() + offset;
	}

	iterator insert(const_iterator pos, std::initializer_list<element> nodes)
	{
		return insert(pos, nodes.begin(), nodes.end());
	}

	template<typename N, std::enable_if_t<
		std::is_same_v<std::remove_reference_t<N>, text> or
		std::is_same_v<std::remove_reference_t<N>, cdata> or
		std::is_same_v<std::remove_reference_t<N>, comment> or
		std::is_same_v<std::remove_reference_t<N>, processing_instruction>,
		int> = 0>
	iterator emplace(const_iterator pos, N&& n)
	{
		return insert_impl(pos, new N(std::forward<N>(n)));
	}

	template<typename Arg>
	inline iterator emplace(const_iterator pos, Arg&& arg)
	{
		static_assert(
			std::is_same_v<std::remove_cv_t<Arg>, element> or	
				not std::is_base_of_v<node,Arg>,
			"Use the nodes() member of element to add nodes other than element");
		return insert_impl(pos, new element(std::forward<Arg>(arg)));
	}

	template<typename... Args>
	inline iterator emplace(const_iterator pos, Args&&... args)
	{
		return insert_impl(pos, new element(std::forward<Args>(args)...));
	}

	inline iterator emplace(const_iterator pos, const std::string& name,
		std::initializer_list<attribute> attrs)
	{
		return insert_impl(pos, new element(name,
			std::forward<std::initializer_list<attribute>>(attrs)));
	}

	template<typename... Args>
	inline element& emplace_front(Args&&... args)
	{
		return *emplace(begin(), std::forward<Args>(args)...);
	}

	inline element& emplace_front(const std::string& name,
		std::initializer_list<attribute> attrs)
	{
		return *emplace(begin(), name,
			std::forward<std::initializer_list<attribute>>(attrs));
	}

	template<typename... Args>
	inline element& emplace_back(Args&&... args)
	{
		return *emplace(end(), std::forward<Args>(args)...);
	}

	inline element& emplace_back(const std::string& name,
		std::initializer_list<attribute> attrs)
	{
		return *emplace(end(), name,
			std::forward<std::initializer_list<attribute>>(attrs));
	}

	inline iterator erase(const_node_iterator pos)
	{
		return m_nodes.erase_impl(pos);
	}

	iterator erase(iterator first, iterator last)
	{
		while (first != last)
		{
			auto next = first;
			++next;

			erase(first);
			first = next;
		}
		return last;
	}

	inline void pop_front()
	{
		erase(begin());
	}

	inline void pop_back()
	{
		erase(end() - 1);
	}

	inline void push_front(element&& e)
	{
		emplace(begin(), std::forward<element>(e));
	}

	inline void push_front(const element& e)
	{
		emplace(begin(), e);
	}

	inline void push_back(element&& e)
	{
		emplace(end(), std::forward<element>(e));
	}

	inline void push_back(const element& e)
	{
		emplace(end(), e);
	}

	void clear();
	size_t size() const			{ return std::distance(begin(), end()); }
	bool empty() const			{ return size() == 0; }

	// --------------------------------------------------------------------
	// attribute support

	attribute_set& attributes()								{ return m_attributes; }
	const attribute_set& attributes() const					{ return m_attributes; }

	// --------------------------------------------------------------------

	friend std::ostream& operator<<(std::ostream& os, const element& e);
	friend class document;

	virtual std::string str() const;

	virtual std::string namespace_for_prefix(const std::string& prefix) const;
	virtual std::pair<std::string,bool> prefix_for_namespace(const std::string& uri) const;

	/// move this element and optionally everyting beneath it to the 
	/// specified namespace/prefix
	void move_to_name_space(const std::string& prefix, const std::string& uri,
		bool recursive, bool including_attributes);

	std::string get_content() const;
	void set_content(const std::string& content);

	std::string get_attribute(const std::string& qname) const;
	void set_attribute(const std::string& qname, const std::string& value);

	/// The set_text method replaces any text node with the new text
	virtual void set_text(const std::string& s);

	/// The add_text method checks if the last added child is a text node,
	/// and if so, it appends the string to this node's value. Otherwise,
	/// it adds a new text node child with the new text.
	void add_text(const std::string& s);

	/// To combine all adjecent child text nodes into one
	void flatten_text();

	// name_space_set& name_spaces()						{ return m_name_spaces; }
	// const name_space_set& name_spaces() const			{ return m_name_spaces; }

	// xpath wrappers
	// TODO: create recursive iterator and use it as return type here
	element_set			find(const std::string& path) const				{ return find(path.c_str()); }
	element*			find_first(const std::string& path) const		{ return find_first(path.c_str()); }

	element_set			find(const char* path) const;
	element*			find_first(const char* path) const;

	/// debug routine
	virtual void validate();

#ifndef LIBZEEP_DOXYGEN_INVOKED
  protected:

	friend std::ostream& operator<<(std::ostream& os, const document& doc);

	virtual node* clone() const;
	virtual node* move();
	virtual void write(std::ostream& os, format_info fmt) const;

	size_t depth() const
	{
		return m_parent != nullptr ? m_parent->depth() + 1 : 0;
	}

	// bottleneck to validate insertions (e.g. document may have only one child element)
	virtual node_iterator insert_impl(const_iterator pos, node* n)
	{
		return m_nodes.insert_impl(pos, n);
	}

  private:

	std::string			m_qname;
	node_list			m_nodes;
	attribute_set		m_attributes;
#endif
};

// --------------------------------------------------------------------

template<>
inline void iterator_impl<element,node>::skip()
{
	while (m_current != nullptr)
	{
		if (dynamic_cast<element*>(m_current) != nullptr)
			break;
		m_current = m_current->next();
	}
}

template<>
inline void iterator_impl<const element,node>::skip()
{
	while (m_current != nullptr)
	{
		if (dynamic_cast<element*>(m_current) != nullptr)
			break;
		m_current = m_current->next();
	}
}

// --------------------------------------------------------------------

/// \brief This method fixes namespace attribute when transferring an element
/// 	   from one document to another (replaces prefixes e.g.)
///
/// When moving an element from one document to another, we need to fix the
/// namespaces, make sure the destination has all the namespace specifications
/// required by the element and make sure the prefixes used are correct.
/// \param e		The element that is being transferred
/// \param source	The (usually) document element that was the source
/// \param dest		The (usually) document element that is the destination

void fix_namespaces(element& e, element& source, element& dest);


} // namespace xml
} // namespace zeep

// --------------------------------------------------------------------
// structured binding support

namespace std
{

template<> struct tuple_size<::zeep::xml::attribute>
            : public std::integral_constant<std::size_t, 2> {};

template<> struct tuple_element<0, ::zeep::xml::attribute>
{
	using type = decltype(std::declval<::zeep::xml::attribute>().name());
};

template<> struct tuple_element<1, ::zeep::xml::attribute>
{
	using type = decltype(std::declval<::zeep::xml::attribute>().value());
};

// template <std::size_t N>
// struct tuple_element<N, ::zeep::xml2::detail::attribute>
// {
//   public:
//     using type = decltype(std::declval<::zeep::xml2::detail::attribute>().get<N>());
// };

}
