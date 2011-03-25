//  Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include <boost/static_assert.hpp>
#include <zeep/xml/node.hpp>

namespace zeep { namespace xml {

class document;

// --------------------------------------------------------------------
// XPath's can contain variables. And variables can contain all kinds of data
// like strings, numbers and even node_sets. If you want to use variables,
// you can define a context, add your variables to it and then pass it on
// in the xpath::evaluate method.

class context
{
  public:
							context();
	virtual					~context();

	template<typename T>
	void					set(const std::string& name, const T& value);

	template<typename T>
	T						get(const std::string& name);

  private:

							context(const context&);
	context&				operator=(const context&);

	friend class xpath;

	struct context_imp*		m_impl;
};

// --------------------------------------------------------------------
// The actual xpath implementation. It expects an xpath in the constructor and
// this path _should_ be UTF-8 encoded.

class xpath
{
  public:
							xpath(const std::string& path);
	virtual					~xpath();

	// evaluate returns a node_set. If you're only interested in xml::element
	// results, you should call the evaluate<element>() instantiation.
	template<typename NODE_TYPE>
	std::list<NODE_TYPE*>	evaluate(const node& root) const;

	// The second evaluate method is used for xpaths that contain variables.
	template<typename NODE_TYPE>
	std::list<NODE_TYPE*>	evaluate(const node& root, context& ctxt) const;

  private:
	struct xpath_imp*		m_impl;
};

// --------------------------------------------------------------------
// template specialisations. 

template<typename T>
void context::set(const std::string& name, const T& value)
{
	// only implemented for the cases specialised below
	assert(false);
}

template<>
void context::set<double>(const std::string& name, const double& value);

template<>
void context::set<std::string>(const std::string& name, const std::string& value);

template<typename T>
T context::get(const std::string& name)
{
	// only implemented for the cases specialised below
	assert(false);
}

template<>
double context::get<double>(const std::string& name);

template<>
std::string context::get<std::string>(const std::string& name);

// --------------------------------------------------------------------

template<>
node_set xpath::evaluate<node>(const node& root) const;

template<>
element_set xpath::evaluate<element>(const node& root) const;
	
template<>
node_set xpath::evaluate<node>(const node& root, context& ctxt) const;

template<>
element_set xpath::evaluate<element>(const node& root, context& ctxt) const;
	
}
}
