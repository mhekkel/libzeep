// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

#include <map>

#include <boost/range.hpp>
#include <boost/cstdint.hpp>

#include <zeep/http/request.hpp>
#include <zeep/exception.hpp>

#include <zeep/el/element.hpp>

namespace zeep
{
namespace el
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

/// \brief Process the text in \a text. The result is put in \a result
///
///	The expression in \a text is processed and the result is returned
/// in \a result.
/// \param scope  The scope for this el script
/// \param text   The el script
/// \param result The result of the script
void evaluate_el(const scope& scope, const std::string& text, object& result);

/// \brief Process the text in \a text and replace it with the result
///
///	The expressions found in \a text are processed and the output of
/// 				the processing is used as a replacement value for the expressions.
/// \param scope  The scope for the el scripts
/// \param text   The text optionally containing el scripts.
/// \return       Returns true if \a text was changed.
bool evaluate_el(const scope& scope, const std::string& text);

// --------------------------------------------------------------------

class scope
{
public:
	scope(const http::request& req);
	explicit scope(const scope& next);

	template <typename T>
	void put(const std::string& name, const T& value);

	template <typename ForwardIterator>
	void put(const std::string& name, ForwardIterator begin, ForwardIterator end);

	const object& lookup(const std::string& name) const;
	const object& operator[](const std::string& name) const;

	object& lookup(const std::string& name);
	object& operator[](const std::string& name);

	const http::request& get_request() const;

private:
	/// for debugging purposes
	friend std::ostream& operator<<(std::ostream& lhs, const scope& rhs);

	scope& operator=(const scope& );

	typedef std::map<std::string, object> data_map;

	data_map m_data;
	scope *m_next;
	const http::request *m_req;
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

} // namespace el
} // namespace zeep

