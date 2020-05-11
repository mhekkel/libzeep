// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

/// \file definition of the routines that can parse and interpret el (expression language) code in a web application context

#include <map>

#include <zeep/http/request.hpp>
#include <zeep/exception.hpp>

#include <zeep/el/element.hpp>

#include <zeep/xml/node.hpp>

namespace zeep::http
{

using object = ::zeep::el::element;

class scope;
/// This zeep::http::el::object class is a bridge to the `el` expression language.

/// \brief Process the text in \a text and return `true` if the result is
///        not empty, zero or false.
///
///	The expression in \a text is processed and if the result of this
/// expression is empty, false or zero then `false` is returned.
/// \param scope  The scope for this el script
/// \param text   The el script
/// \return       The result of the script
bool process_el(const scope& scope, std::string& text);

/// \brief Process the text in \a text and return the result if the expression is valid,
///        the value of \a text otherwise.
///
///	If the expression in \a text is valid, it is processed and the result
/// is returned, otherwise simply returns the text.
/// \param scope  The scope for this el script
/// \param text   The el script
/// \return       The result of the script
std::string process_el_2(const scope& scope, const std::string& text);

/// \brief Process the text in \a text. The result is put in \a result
///
///	The expression in \a text is processed and the result is returned
/// in \a result.
/// \param scope  The scope for this el script
/// \param text   The el script
/// \param result The result of the script
object evaluate_el(const scope& scope, const std::string& text);

// /// \brief Process the text in \a text and replace it with the result
// ///
// ///	The expressions found in \a text are processed and the output of
// /// 				the processing is used as a replacement value for the expressions.
// /// \param scope  The scope for the el scripts
// /// \param text   The text optionally containing el scripts.
// /// \return       Returns true if \a text was changed.
// bool evaluate_el(const scope& scope, const std::string& text);

/// \brief Process the text in \a text and return a list of name/value pairs
///
///	The expressions found in \a text are processed and the result is
/// 				returned as a list of name/value pairs to be used in e.g.
///                 processing a m2:attr attribute.
/// \param scope  The scope for the el scripts
/// \param text   The text optionally containing el scripts.
/// \return       list of name/value pairs
std::vector<std::pair<std::string,std::string>> evaluate_el_attr(const scope& scope, const std::string& text);

/// \brief Process the text in \a text. This should be a comma separated list
/// of expressions that each should evaluate to true.
///
///	The expression in \a text is processed and the result is false if
/// one of the expressions in the comma separated list evaluates to false.
///
/// in \a result.
/// \param scope  The scope for this el script
/// \param text   The el script
/// \return       True in case all the expressions evaluate to true
bool evaluate_el_assert(const scope& scope, const std::string& text);

// --------------------------------------------------------------------

class scope
{
  public:
	scope();
	scope(const http::request& req);
	explicit scope(const scope& next);

	template <typename T>
	void put(const std::string& name, const T& value);

	template <typename ForwardIterator>
	void put(const std::string& name, ForwardIterator begin, ForwardIterator end);

	const object& lookup(const std::string& name, bool includeSelected = false) const;
	const object& operator[](const std::string& name) const;

	object& lookup(const std::string& name);
	object& operator[](const std::string& name);

	const http::request& get_request() const;

    // current selected object
    void select_object(const object& o);

	// a nodeset for a selector, cached to avoid recusive expansion
	using node_set_type = std::vector<std::unique_ptr<xml::node>>;

	node_set_type get_nodeset(const std::string& name) const;
	void set_nodeset(const std::string& name, node_set_type&& nodes);
	bool has_nodeset(const std::string& name) const
	{
		return m_nodesets.count(name) or (m_next != nullptr and m_next->has_nodeset(name));
	}

  private:
	/// for debugging purposes
	friend std::ostream& operator<<(std::ostream& lhs, const scope& rhs);

	scope& operator=(const scope& );

	using data_map = std::map<std::string, object>;

	data_map m_data;
	scope *m_next;
	unsigned m_depth;
	const http::request *m_req;
    object m_selected;

	using nodeset_map = std::map<std::string,node_set_type>;

	nodeset_map m_nodesets;
};

template <typename T>
inline void scope::put(const std::string& name, const T& value)
{
	m_data[name] = value;
}

template <>
inline void scope::put(const std::string& name, const object& value)
{
	m_data[name] = value;
}

template <typename ForwardIterator>
inline void scope::put(const std::string& name, ForwardIterator begin, ForwardIterator end)
{
	std::vector<object> elements;
	while (begin != end)
		elements.push_back(object(*begin++));
	m_data[name] = elements;
}

// --------------------------------------------------------------------

} // namespace zeep::http

