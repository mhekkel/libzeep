// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-License-Identifier: BSL-1.0
//
// expression language support
//

#pragma once

/// \file
/// definition of the routines that can parse and interpret el (expression language) code in a web application context

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include "zeep/el/object.hpp"

# include <zeem/zeem.hpp>
#endif

namespace zeep::http
{

ZEEP_EXPORT class scope;
ZEEP_EXPORT class basic_server;

using object = el::object;

/// \brief Process the text in \a text and return `true` if the result is
///        not empty, zero or false.
///
///	The expression in \a text is processed and if the result of this
/// expression is empty, false or zero then `false` is returned.
/// \param scope_ The scope for this el script
/// \param text   The el script
/// \return       The result of the script
ZEEP_EXPORT bool process_el(const scope &scope_, std::string &text);

/// \brief Process the text in \a text and return the result if the expression is valid,
///        the value of \a text otherwise.
///
///	If the expression in \a text is valid, it is processed and the result
/// is returned, otherwise simply returns the text.
/// \param scope_ The scope for this el script
/// \param text   The el script
/// \return       The result of the script
ZEEP_EXPORT std::string process_el_2(const scope &scope_, const std::string &text);

/// \brief Process the text in \a text. The result is put in \a result
///
///	The expression in \a text is processed and the result is returned
/// in \a result.
/// \param scope_ The scope for this el script
/// \param text   The el script
/// \result		  The result of the script
ZEEP_EXPORT object evaluate_el(const scope &scope_, const std::string &text);

/// \brief Process the text in \a text and return a list of name/value pairs
///
///	The expressions found in \a text are processed and the result is
/// 				returned as a list of name/value pairs to be used in e.g.
///                 processing a m2:attr attribute.
/// \param scope_ The scope for the el scripts
/// \param text   The text optionally containing el scripts.
/// \return       list of name/value pairs
ZEEP_EXPORT std::vector<std::pair<std::string, std::string>> evaluate_el_attr(const scope &scope_, const std::string &text);

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
ZEEP_EXPORT bool evaluate_el_assert(const scope &scope_, const std::string &text);

/// \brief Process the text in \a text and put the resulting z:with expressions in the scope
///
///	The expressions found in \a text are processed and the result is
///	returned as a list of name/value pairs to be used in e.g.
/// processing a m2:attr attribute.
/// \param scope_ The scope for the el scripts
/// \param text   The text containing el scripts in the form var=val(,var=val)*.
ZEEP_EXPORT void evaluate_el_with(scope &scope_, const std::string &text);

/// \brief Evaluate the text in \a text as a potential link template
///
///	The expression found in \a text is processed and the result is
///	returned as a link template object. This function is called from
/// el::include, el::replace and el::insert attributes.
///
/// \param scope_ The scope for the el scripts
/// \param text   The text containing the link specification
/// \result		  The resulting link
ZEEP_EXPORT object evaluate_el_link(const scope &scope_, const std::string &text);

// --------------------------------------------------------------------

/// \brief Base class for utility objects, objects that are exposed as
/// objects in the Expression Language API.

ZEEP_EXPORT class expression_utility_object_base
{
  public:
	virtual ~expression_utility_object_base() = default;

	/// \brief Evaluate a static method call on a registered utility object
	/// \param scope_      The scope for this el script
	/// \param className   The name of the utility class
	/// \param methodName  The method to invoke
	/// \param parameters  The parameters to pass to the method
	/// \return            The result of the evaluation, or a null object if not found
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
	/// \brief Evaluate a method call on this utility object
	/// \param scope_      The scope for this el script
	/// \param methodName  The method to invoke
	/// \param parameters  The parameters to pass to the method
	/// \return            The result of the evaluation
	[[nodiscard]] virtual object evaluate(const scope &scope_, const std::string &methodName,
		const std::vector<object> &parameters) const = 0;

	/// \brief Struct used to store the instances of the derived classes along with
	/// their name
	struct instance
	{
		expression_utility_object_base *m_obj = nullptr; ///< Pointer to the utility object instance
		const char *m_name{};                            ///< The registered name of the utility class
		instance *m_next = nullptr;                      ///< Pointer to the next instance in the linked list
	};

	static instance *s_head; ///< Head of the linked list of registered utility object instances
};

/// \brief The actual base class for utility objects, objects that are exposed as
/// objects in the Expression Language API.
/// Uses the https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
///
/// \tparam OBJ  The derived class type (CRTP pattern)

ZEEP_EXPORT template <typename OBJ>
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
