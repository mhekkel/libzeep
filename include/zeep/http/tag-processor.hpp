//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::tag_processor classes. These classes take care of processing HTML templates

#include "zeep/config.hpp"

#include <filesystem>

#include "zeep/el/processing.hpp"
#include <zeem.hpp>

namespace zeep::http
{

class html_controller;
class basic_template_processor;

// --------------------------------------------------------------------
//

/// \brief Abstract base class for tag_processor.
///
/// Note that this class should be light in construction, we create it every time a page is rendered.

class tag_processor_base
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
	virtual void process_xml(zeem::node *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader) = 0;

  protected:
	/// \brief constructor
	///
	/// \param ns	Then XML namespace for the tags and attributes that are processed by this tag_processor
	tag_processor_base(std::string ns)
		: m_ns(std::move(ns))
	{
	}

	std::string m_ns;
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

class tag_processor : public tag_processor_base
{
  public:
	/// \brief default namespace for this processor
	static constexpr const char *ns() { return "http://www.hekkelman.com/libzeep/m2"; }

	/// \brief each handler returns a code telling the processor what to do with the node
	enum class AttributeAction
	{
		none,
		remove,
		replace
	};

	using attr_handler = std::function<AttributeAction(zeem::element *, zeem::attribute &, scope &, std::filesystem::path, basic_template_processor &loader)>;

	/// \brief constructor with default namespace
	tag_processor(const char *ns = tag_processor::ns());

	/// \brief process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	void process_xml(zeem::node *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader) override;

	/// \brief It is possible to extend this processor with custom handlers
	void register_attr_handler(std::string attr, attr_handler &&handler)
	{
		m_attr_handlers.emplace(std::move(attr), std::move(handler));
	}

  protected:
	void process_node(zeem::node *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_text(zeem::node_with_text &t, const scope &scope);
	void post_process(zeem::element *e, const scope &parentScope, std::filesystem::path dir, basic_template_processor &loader);

	// zeem::element resolve_fragment_spec(zeem::element* node, std::filesystem::path dir, basic_html_controller& controller, const std::string& spec, const scope& scope);
	zeem::element resolve_fragment_spec(zeem::element *node, std::filesystem::path dir, basic_template_processor &loader, const object &spec, const scope &scope);
	zeem::element resolve_fragment_spec(zeem::element *node, std::filesystem::path dir, basic_template_processor &loader, const std::string &file, std::string_view selector, bool byID);

	// virtual void process_node_attr(zeem::node* node, const scope& scope, std::filesystem::path dir);
	AttributeAction process_attr_if(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, bool unless);
	AttributeAction process_attr_assert(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_text(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, bool escaped);
	AttributeAction process_attr_switch(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_each(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_attr(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_with(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_generic(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_boolean_value(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_inline(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_append(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, std::string dest, bool prepend);
	AttributeAction process_attr_classappend(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_styleappend(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_remove(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);

	enum class TemplateIncludeAction
	{
		include,
		insert,
		replace
	};

	AttributeAction process_attr_include(zeem::element *node, zeem::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, TemplateIncludeAction tia);

	std::map<std::string, attr_handler> m_attr_handlers;
	zeem::document m_template; // copy of the entire document...
};

} // namespace zeep::http
