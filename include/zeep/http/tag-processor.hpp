//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::tag_processor classes. These classes take care of processing HTML templates

#include <zeep/config.hpp>

#include <filesystem>

#include <zeep/el/processing.hpp>
#include <mxml.hpp>

namespace zeep::http
{

class html_controller;
class basic_template_processor;

// --------------------------------------------------------------------
//

/// \brief Abstract base class for tag_processor.
///
/// Note that this class should be light in construction, we create it every time a page is rendered.

class tag_processor
{
  public:
	tag_processor(const tag_processor &) = delete;
	tag_processor &operator=(const tag_processor &) = delete;

	virtual ~tag_processor() = default;

	/// \brief process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	///
	/// This function is called to modify the xml tree in \a node
	///
	/// \param node		The XML mxml::node (element) to manipulate
	/// \param scope	The zeep::http::scope containing the variables and request
	/// \param dir		The path to the docroot, the directory containing the XHTML templates
	/// \param loader	The template processor to use to load resources
	virtual void process_xml(mxml::node *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader) = 0;

  protected:
	/// \brief constructor
	///
	/// \param ns	Then XML namespace for the tags and attributes that are processed by this tag_processor
	tag_processor(const char *ns)
		: m_ns(ns)
	{
	}

	std::string m_ns;
};

// --------------------------------------------------------------------

#if ZEEP_SUPPORT_TAG_PROCESSOR_V1

/// \brief A tag_processor compatible with the old version of libzeep. Works
/// on tags only, not on attributes. Also parses any occurrence of ${}.
/// For newer code, please consider using the v2 version only.

class tag_processor_v1 : public tag_processor
{
  public:
	/// \brief default namespace for this processor
	static constexpr const char *ns() { return "http://www.hekkelman.com/libzeep/m1"; }

	/// \brief constructor
	///
	/// By default the namespace for the v1 processor is the one in ns()
	tag_processor_v1(const char *ns = tag_processor_v1::ns());

	/// \brief actual implementation of the tag processing.
	virtual void process_xml(mxml::node *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);

  protected:
	virtual void process_tag(const std::string &tag, mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);

  private:
	/// handler for mrs:include tags
	void process_include(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_if(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_iterate(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_for(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_number(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_options(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_option(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_checkbox(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	// void process_url(mxml::element* node, const scope& scope, std::filesystem::path dir, basic_template_processor& loader);
	void process_param(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_embed(mxml::element *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);

	bool process_el(const scope &scope, std::string &s);
};

#endif

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

class tag_processor_v2 : public tag_processor
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

	using attr_handler = std::function<AttributeAction(mxml::element *, mxml::attribute &, scope &, std::filesystem::path, basic_template_processor &loader)>;

	/// \brief constructor with default namespace
	tag_processor_v2(const char *ns = tag_processor_v2::ns());

	/// \brief process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	void process_xml(mxml::node *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader) override;

	/// \brief It is possible to extend this processor with custom handlers
	void register_attr_handler(const std::string &attr, attr_handler &&handler)
	{
		m_attr_handlers.emplace(attr, std::move(handler));
	}

  protected:
	void process_node(mxml::node *node, const scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	void process_text(mxml::node_with_text &t, const scope &scope);
	void post_process(mxml::element *e, const scope &parentScope, std::filesystem::path dir, basic_template_processor &loader);

	// mxml::element resolve_fragment_spec(mxml::element* node, std::filesystem::path dir, basic_html_controller& controller, const std::string& spec, const scope& scope);
	mxml::element resolve_fragment_spec(mxml::element *node, std::filesystem::path dir, basic_template_processor &loader, const object &spec, const scope &scope);
	mxml::element resolve_fragment_spec(mxml::element *node, std::filesystem::path dir, basic_template_processor &loader, const std::string &file, const std::string &selector, bool byID);

	// virtual void process_node_attr(mxml::node* node, const scope& scope, std::filesystem::path dir);
	AttributeAction process_attr_if(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, bool unless);
	AttributeAction process_attr_assert(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_text(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, bool escaped);
	AttributeAction process_attr_switch(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_each(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_attr(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_with(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_generic(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_boolean_value(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_inline(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_append(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, std::string dest, bool prepend);
	AttributeAction process_attr_classappend(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_styleappend(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);
	AttributeAction process_attr_remove(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader);

	enum class TemplateIncludeAction
	{
		include,
		insert,
		replace
	};

	AttributeAction process_attr_include(mxml::element *node, mxml::attribute &attr, scope &scope, std::filesystem::path dir, basic_template_processor &loader, TemplateIncludeAction tia);

	std::map<std::string, attr_handler> m_attr_handlers;
	mxml::document m_template; // copy of the entire document...
};

} // namespace zeep::http
