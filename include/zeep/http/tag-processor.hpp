// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2019-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the zeep::http::tag_processor classes. These classes take care of processing HTML templates

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include "zeep/el/processing.hpp"
# include "zeep/http/scope.hpp"

# include <zeem/zeem.hpp>

# include <filesystem>
# include <functional>
# include <map>
# include <string>
# include <string_view>
# include <unordered_set>
# include <utility>
#endif

namespace zeep::http
{

ZEEP_EXPORT class basic_template_processor;

// --------------------------------------------------------------------
//

/// \brief Abstract base class for tag_processor.
///
/// Note that this class should be light in construction, we create it every time a page is rendered.

ZEEP_EXPORT class tag_processor_base
{
  public:
	tag_processor_base(const tag_processor_base &) = delete;
	tag_processor_base &operator=(const tag_processor_base &) = delete;

	virtual ~tag_processor_base() = default;

	/// \brief process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	///
	/// This function is called to modify the xml tree in \a node
	///
	/// \param node		The XML zeem::node (element) to manipulate
	/// \param scope	The zeep::http::scope containing the variables and request
	/// \param dir		The path to the docroot, the directory containing the XHTML templates
	/// \param loader	The template processor to use to load resources
	virtual void process_xml(zeem::node *node, const scope &scope, const std::filesystem::path &dir, basic_template_processor &loader) = 0;

  protected:
	/// \brief constructor
	///
	/// \param ns	Then XML namespace for the tags and attributes that are processed by this tag_processor
	tag_processor_base(std::string ns)
		: m_ns(std::move(ns))
	{
	}

	std::string m_ns; ///< The XML namespace associated with this processor
};

// --------------------------------------------------------------------

/// \brief version two of the tag_processor in libzeep
///
/// This is the new tag_processor. It is designed to look a bit like
/// Thymeleaf (https://www.thymeleaf.org/)
/// This processor works on attributes mostly, but can process inline
/// el constructs as well.
///
/// The documentention contains a section describing all the
/// xml tags and attributes this processor handles.

ZEEP_EXPORT class tag_processor : public tag_processor_base
{
  public:
	/// \brief default namespace for this processor
	static constexpr const char *ns() { return "http://www.hekkelman.com/libzeep/m2"; }

	/// \brief each handler returns a code telling the processor what to do with the node
	enum class AttributeAction
	{
		none,   ///< No action required
		remove, ///< Remove the attribute from the element
		replace ///< Replace the element with the processed content
	};

	/// \brief Signature for custom attribute handler functions
	using attr_handler = std::function<AttributeAction(zeem::element *, zeem::attribute &, scope &, const std::filesystem::path &, basic_template_processor &loader)>;

	/// \brief constructor with default namespace
	tag_processor(const char *ns = tag_processor::ns());

	/// \brief process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	/// \param node    The XML node to process
	/// \param scope   The scope containing variables and request data
	/// \param dir     The docroot directory path
	/// \param loader  The template processor for loading resources
	void process_xml(zeem::node *node, const scope &scope, const std::filesystem::path &dir, basic_template_processor &loader) override;

	/// \brief It is possible to extend this processor with custom handlers
	/// \param attr     The attribute name to handle
	/// \param handler  The handler function to invoke
	void register_attr_handler(std::string attr, attr_handler &&handler)
	{
		m_attr_handlers.emplace(std::move(attr), std::move(handler));
	}

  protected:
	/// \name Node processing
	///@{

	/// \brief Recursively process a node and its children
	/// \param node    The XML node to process
	/// \param scope   The scope containing variables and request data
	/// \param dir     The docroot directory path
	/// \param loader  The template processor for loading resources
	void process_node(zeem::node *node, const scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process text nodes for inline el expressions
	/// \param t      The text node to process
	/// \param scope  The scope containing variables and request data
	void process_text(zeem::node_with_text &t, const scope &scope);

	/// \brief Post-process an element after all children have been processed
	/// \param e             The element to post-process
	/// \param parentScope   The parent scope
	/// \param dir           The docroot directory path
	/// \param loader        The template processor for loading resources
	void post_process(zeem::element *e, const scope &parentScope, const std::filesystem::path &dir, basic_template_processor &loader);

	///@}

	/// \name Fragment resolution
	///@{

	/// \brief Resolve a fragment specification object into a DOM element
	/// \param node    The context node
	/// \param dir     The docroot directory path
	/// \param loader  The template processor for loading resources
	/// \param spec    The fragment specification as an \a el::object
	/// \param scope   The scope containing variables and request data
	/// \return        The resolved element, or nullptr if not found
	zeem::element resolve_fragment_spec(zeem::element *node, const std::filesystem::path &dir, basic_template_processor &loader, const el::object &spec, const scope &scope);

	/// \brief Resolve a fragment specification from a file, selector and id flag
	/// \param node     The context node
	/// \param dir      The docroot directory path
	/// \param loader   The template processor for loading resources
	/// \param file     The file path
	/// \param selector The CSS-like selector
	/// \param byID     Whether the selector is an ID selector
	/// \return         The resolved element, or nullptr if not found
	zeem::element resolve_fragment_spec(zeem::element *node, const std::filesystem::path &dir, basic_template_processor &loader, const std::string &file, std::string_view selector, bool byID);

	///@}

	/// \name Attribute processing
	///@{

	/// \brief Process a conditional attribute (<tt>m2:if</tt> / <tt>m2:unless</tt>)
	AttributeAction process_attr_if(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader, bool unless);

	/// \brief Process an assertion attribute (<tt>m2:assert</tt>)
	AttributeAction process_attr_assert(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process a text attribute (<tt>m2:text</tt> / <tt>m2:utext</tt>)
	AttributeAction process_attr_text(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader, bool escaped);

	/// \brief Process a switch attribute (<tt>m2:switch</tt>)
	AttributeAction process_attr_switch(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process an iteration attribute (<tt>m2:each</tt>)
	AttributeAction process_attr_each(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process a generic attribute replacement (<tt>m2:attr</tt>)
	AttributeAction process_attr_attr(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process a with variable attribute (<tt>m2:with</tt>)
	AttributeAction process_attr_with(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process a generic value attribute
	AttributeAction process_attr_generic(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process a boolean value attribute
	AttributeAction process_attr_boolean_value(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process an inline attribute
	AttributeAction process_attr_inline(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process an append/prepend attribute
	AttributeAction process_attr_append(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader, std::string dest, bool prepend);

	/// \brief Process a classappend attribute (<tt>m2:classappend</tt>)
	AttributeAction process_attr_classappend(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process a styleappend attribute (<tt>m2:styleappend</tt>)
	AttributeAction process_attr_styleappend(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Process a remove attribute (<tt>m2:remove</tt>)
	AttributeAction process_attr_remove(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader);

	/// \brief Action to take when including template fragments
	enum class TemplateIncludeAction
	{
		include, ///< Include the fragment content
		insert,  ///< Insert the fragment as a child
		replace  ///< Replace the current element with the fragment
	};

	/// \brief Process an include/insert/replace attribute
	AttributeAction process_attr_include(zeem::element *node, zeem::attribute &attr, scope &scope, const std::filesystem::path &dir, basic_template_processor &loader, TemplateIncludeAction tia);

	///@}

	/// \brief Mark a node inserted as already-rendered output so it is not re-processed.
	/// \param node  The node that must not be processed as template content again
	void mark_rendered(zeem::node *node) { m_rendered.insert(node); }

	std::map<std::string, attr_handler> m_attr_handlers; ///< Registered custom attribute handlers
	zeem::document m_template;                           ///< Cached copy of template documents

	/// Nodes inserted as already-rendered output (via m:utext or inline [(...)]).
	/// These are treated as final markup and skipped during recursive template
	/// processing, preventing untrusted data from being re-evaluated as EL.
	std::unordered_set<const zeem::node *> m_rendered;
};

} // namespace zeep::http
