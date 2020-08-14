// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

/// \file
/// definition of the routines that can parse and interpret el (expression language) code in a web application context

#include <zeep/config.hpp>

#include <map>

#include <zeep/exception.hpp>
#include <zeep/json/element.hpp>
#include <zeep/http/request.hpp>
#include <zeep/xml/node.hpp>

namespace zeep::http
{

using object = ::zeep::json::element;

class scope;
class server;

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
/// \result		  The result of the script
object evaluate_el(const scope& scope, const std::string& text);

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

/// \brief Process the text in \a text and put the resulting z:with expressions in the scope
///
///	The expressions found in \a text are processed and the result is
///	returned as a list of name/value pairs to be used in e.g.
/// processing a m2:attr attribute.
/// \param scope  The scope for the el scripts
/// \param text   The text containing el scripts in the form var=val(,var=val)*.
void evaluate_el_with(scope& scope, const std::string& text);

// --------------------------------------------------------------------

class expression_utility_object_base
{
  public:

	virtual ~expression_utility_object_base() = default;

	static object evaluate(const scope& scope,
		const std::string& className, const std::string& methodName,
		const std::vector<object>& parameters)
	{
		for (auto inst = s_head; inst != nullptr; inst = inst->m_next)
		{
			if (className == inst->m_name)
				return inst->m_obj->evaluate(scope, methodName, parameters);
		}

		return {};
	}

  protected:

	virtual object evaluate(const scope& scope, const std::string& methodName,
		const std::vector<object>& parameters) const = 0;

	struct instance
	{
		expression_utility_object_base*	m_obj = nullptr;
		const char*						m_name;
		instance*						m_next = nullptr;
	};

	static instance* s_head;
};

template<typename OBJ>
class expression_utility_object : public expression_utility_object_base
{
  public:
	using implementation_type = OBJ;

  protected:

	expression_utility_object()
	{
		static instance s_next{ this, implementation_type::name(), s_head };
		s_head = &s_next;
	}
};

// --------------------------------------------------------------------

/// \brief The class that stores variables for the current scope
///
/// When processing tags and in expression language constructs we use
/// variables. These are stored in scope instances.

class scope
{
  public:

	/// \brief simple constructor, used where there's no request available
	scope();

	/// \brief constructor to be used only in debugging 
	///
	/// \param req		The incomming HTTP request
	scope(const request& req);

	/// \brief constructor used in a HTTP request context
	///
	/// \param server	The server that handles the incomming request
	/// \param req		The incomming HTTP request
	scope(const server& server, const request& req);

	/// \brief chaining constructor
	///
	/// Scopes can be nested, introducing new namespaces
	/// \param next	The next scope up the chain.
	explicit scope(const scope& next);

	/// \brief put variable in the scope with \a name and \a value
	template <typename T>
	void put(const std::string& name, const T& value);

	/// \brief put variable of type array in the scope with \a name and values from \a begin to \a end
	template <typename ForwardIterator>
	void put(const std::string& name, ForwardIterator begin, ForwardIterator end);

	/// \brief return variable with \a name 
	///
	/// \param name				The name of the variable to return
	/// \param includeSelected	If this is true, and the variable was not found as a regular variable
	///							in the current scope, the selected objects will be search for members
	///							with \a name This is used by the tag processing lib v2 in _z2:object_
	/// \return					The value found or null if there was no such variable.
	const object& lookup(const std::string& name, bool includeSelected = false) const;

	/// \brief return variable with \a name 
	const object& operator[](const std::string& name) const;

	/// \brief return variable with \a name 
	///
	/// \param name				The name of the variable to return
	/// \param includeSelected	If this is true, and the variable was not found as a regular variable
	///							in the current scope, the selected objects will be search for members
	///							with \a name This is used by the tag processing lib v2 in _z2:object_
	/// \return					The value found or null if there was no such variable.
	object& lookup(const std::string& name);

	/// \brief return variable with \a name 
	object& operator[](const std::string& name);

	/// \brief return the HTTP request, will throw if the scope chain was not created with a request
	const request& get_request() const;

	/// \brief return the context_name of the server
	std::string get_context_name() const;

	/// \brief return the credentials of the current user
	json::element get_credentials() const;

    /// \brief select object \a o , used in z2:object constructs
    void select_object(const object& o);

	/// \brief a nodeset for a selector, cached to avoid recusive expansion
	///
	/// In tag processors it is sometimes needed to take a selection of zeep::xml::nodes
	/// and reuse these, as a copy when inserting templates e.g.
	using node_set_type = std::vector<std::unique_ptr<xml::node>>;

	/// \brief return the node_set_type with name \a name
	node_set_type get_nodeset(const std::string& name) const;

	/// \brief store node_set_type \a nodes with name \a name
	void set_nodeset(const std::string& name, node_set_type&& nodes);

	/// \brief return whether a node_set with name \a name is stored
	bool has_nodeset(const std::string& name) const
	{
		return m_nodesets.count(name) or (m_next != nullptr and m_next->has_nodeset(name));
	}

	/// \brief get the CSRF token from the request burried in \a scope
	std::string get_csrf_token() const;

  private:
	/// for debugging purposes
	friend std::ostream& operator<<(std::ostream& lhs, const scope& rhs);

	scope& operator=(const scope& );

	using data_map = std::map<std::string, object>;

	data_map m_data;
	scope *m_next;
	unsigned m_depth;
	const request *m_req;
	const server* m_server;
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

