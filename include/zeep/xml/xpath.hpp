// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::xml::xpath class, implementing a XPath 1.0 compatible search facility

#include <zeep/config.hpp>

#include <zeep/xml/node.hpp>

namespace zeep::xml
{

class document;

// --------------------------------------------------------------------
/// XPath's can contain variables. And variables can contain all kinds of data
/// like strings, numbers and even node_sets. If you want to use variables,
/// you can define a context, add your variables to it and then pass it on
/// in the xpath::evaluate method.

class context
{
  public:
	context();
	virtual ~context();

	void set(const std::string& name, const std::string& value);
	void set(const std::string& name, double value);

	template <typename T, std::enable_if_t<std::is_same_v<T, std::string> or std::is_same_v<T, double>, int> = 0>
	T get(const std::string& name);

  private:
	context(const context &);
	context& operator=(const context &);

	friend class xpath;

	struct context_imp* m_impl;
};

// --------------------------------------------------------------------
/// The actual xpath implementation. It expects an xpath in the constructor and
/// this path _must_ be UTF-8 encoded.

class xpath
{
  public:
	xpath(const std::string& path);
	xpath(const char* path);
	xpath(const xpath& rhs);
	xpath& operator=(const xpath &);

	virtual ~xpath();

	/// evaluate returns a node_set. If you're only interested in zeep::xml::element
	/// results, you should call the evaluate<element>() instantiation.
	template <typename NODE_TYPE>
	std::list<NODE_TYPE*> evaluate(const node& root) const
	{
		context ctxt;
		return evaluate<NODE_TYPE>(root, ctxt);
	}

	/// The second evaluate method is used for xpaths that contain variables.
	template <typename NODE_TYPE>
	std::list<NODE_TYPE*> evaluate(const node& root, context& ctxt) const;

	/// Returns true if the \a n node matches the XPath
	bool matches(const node* n) const;

	/// debug routine, dumps the parse tree to stdout
	void dump();

  private:
	struct xpath_imp* m_impl;
};

} // namespace zeep::xml
