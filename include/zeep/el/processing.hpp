//        Copyright Maarten L. Hekkelman, 2014-2026
// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

/// \file
/// definition of the routines that can parse and interpret el (expression language) code in a web application context

#include "zeep/el/object.hpp"

#include <zeem/zeem.hpp>

namespace zeep::http
{

class scope;
class basic_server;

using object = el::object;

/// \brief Process the text in \a text and return `true` if the result is
///        not empty, zero or false.
///
///	The expression in \a text is processed and if the result of this
/// expression is empty, false or zero then `false` is returned.
/// \param scope_ The scope for this el script
/// \param text   The el script
/// \return       The result of the script
bool process_el(const scope &scope_, std::string &text);

/// \brief Process the text in \a text and return the result if the expression is valid,
///        the value of \a text otherwise.
///
///	If the expression in \a text is valid, it is processed and the result
/// is returned, otherwise simply returns the text.
/// \param scope_ The scope for this el script
/// \param text   The el script
/// \return       The result of the script
std::string process_el_2(const scope &scope_, const std::string &text);

/// \brief Process the text in \a text. The result is put in \a result
///
///	The expression in \a text is processed and the result is returned
/// in \a result.
/// \param scope_ The scope for this el script
/// \param text   The el script
/// \result		  The result of the script
object evaluate_el(const scope &scope_, const std::string &text);

/// \brief Process the text in \a text and return a list of name/value pairs
///
///	The expressions found in \a text are processed and the result is
/// 				returned as a list of name/value pairs to be used in e.g.
///                 processing a m2:attr attribute.
/// \param scope_ The scope for the el scripts
/// \param text   The text optionally containing el scripts.
/// \return       list of name/value pairs
std::vector<std::pair<std::string, std::string>> evaluate_el_attr(const scope &scope_, const std::string &text);

/// \brief Process the text in \a text. This should be a comma separated list
/// of expressions that each should evaluate to true.
///
///	The expression in \a text is processed and the result is false if
/// one of the expressions in the comma separated list evaluates to false.
///
/// in \a result.
/// \param scope_ The scope for this el script
/// \param text   The el script
/// \return       True in case all the expressions evaluate to true
bool evaluate_el_assert(const scope &scope_, const std::string &text);

/// \brief Process the text in \a text and put the resulting z:with expressions in the scope
///
///	The expressions found in \a text are processed and the result is
///	returned as a list of name/value pairs to be used in e.g.
/// processing a m2:attr attribute.
/// \param scope_ The scope for the el scripts
/// \param text   The text containing el scripts in the form var=val(,var=val)*.
void evaluate_el_with(scope &scope_, const std::string &text);

/// \brief Evaluate the text in \a text as a potential link template
///
///	The expression found in \a text is processed and the result is
///	returned as a link template object. This function is called from
/// el::include, el::replace and el::insert attributes.
///
/// \param scope_ The scope for the el scripts
/// \param text   The text containing the link specification
/// \result		  The resulting link
object evaluate_el_link(const scope &scope_, const std::string &text);

// --------------------------------------------------------------------

/// \brief Base class for utility objects, objects that are exposed as
/// objects in the Expression Language API.

class expression_utility_object_base
{
  public:
	virtual ~expression_utility_object_base() = default;

	static object evaluate(const scope &scope_,
		const std::string &className, const std::string &methodName,
		const std::vector<object> &parameters)
	{
		for (auto inst = s_head; inst != nullptr; inst = inst->m_next)
		{
			if (className == inst->m_name)
				return inst->m_obj->evaluate(scope_, methodName, parameters);
		}

		return {};
	}

  protected:
	[[nodiscard]] virtual object evaluate(const scope &scope_, const std::string &methodName,
		const std::vector<object> &parameters) const = 0;

	/// Struct used to store the instances of the derived classes along with
	/// their name
	struct instance
	{
		expression_utility_object_base *m_obj = nullptr;
		const char *m_name{};
		instance *m_next = nullptr;
	};

	static instance *s_head;
};

/// \brief The actual base class for utility objects, objects that are exposed as
/// objects in the Expression Language API.
/// Uses the https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

template <typename OBJ>
class expression_utility_object : public expression_utility_object_base
{
  public:
	using implementation_type = OBJ;

  protected:
	expression_utility_object() noexcept // NOLINT(bugprone-crtp-constructor-accessibility)
	{
		static instance s_next{ this, implementation_type::name(), s_head };
		s_head = &s_next;
	}
};

} // namespace zeep::http
