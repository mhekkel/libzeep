//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::tag_processor classes. These classes take care of processing HTML templates

#include <filesystem>

#include <zeep/xml/document.hpp>
#include <zeep/http/el-processing.hpp>

namespace zeep::http
{

class basic_html_controller;

// --------------------------------------------------------------------
//

/// \brief Abstract base class for tag_processor. 
///
/// Note that this class should be light in construction, we create it every time a page is rendered.

class tag_processor
{
  public:
	tag_processor(const tag_processor&) = delete;
	tag_processor& operator=(const tag_processor&) = delete;

	virtual ~tag_processor() = default;

	/// \brief process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	///
	/// This function is called to modify the xml tree in \a node
	///
	/// \param node		The XML zeep::xml::node (element) to manipulate
	/// \param scope	The zeep::http::scope containing the variables and request
	/// \param dir		The path to the docroot, the directory containing the XHTML templates
	/// \param webapp	The instance of webapp that called this function
	virtual void process_xml(xml::node* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp) = 0;

  protected:

	/// \brief constructor
	///
	/// \param ns	Then XML namespace for the tags and attributes that are processed by this tag_processor 
	tag_processor(const char* ns)
		: m_ns(ns) {}

	std::string	m_ns;
};

// --------------------------------------------------------------------

/// \brief A tag_processor compatible with the old version of libzeep. Works
/// on tags only, not on attributes. Also parses any occurrence of ${}.
/// For newer code, please consider using the v2 version only.

class tag_processor_v1 : public tag_processor
{
  public:
	/// \brief default namespace for this processor
	static constexpr const char* ns() { return "http://www.hekkelman.com/libzeep/m1"; }

	/// \brief constructor
	///
	/// By default the namespace for the v1 processor is the one in ns()
	tag_processor_v1(const char* ns = tag_processor_v1::ns());

	/// \brief actual implementation of the tag processing.
	virtual void process_xml(xml::node* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);

  protected:

	virtual void process_tag(const std::string& tag, xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);

  private:

	/// handler for mrs:include tags
	void process_include(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_if(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_iterate(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_for(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_number(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_options(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_option(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_checkbox(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_url(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_param(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_embed(xml::element* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);

	bool process_el(const scope& scope, std::string& s);
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

class tag_processor_v2 : public tag_processor
{
  public:
	/// \brief default namespace for this processor
	static constexpr const char* ns() { return "http://www.hekkelman.com/libzeep/m2"; }

	/// \brief each handler returns a code telling the processor what to do with the node
	enum class AttributeAction
	{
		none, remove, replace
	};

	using attr_handler = std::function<AttributeAction(xml::element*, xml::attribute*, scope&, std::filesystem::path, basic_html_controller& webapp)>;

	/// \brief constructor with default namespace
	tag_processor_v2(const char* ns = tag_processor_v2::ns());

	/// \brief process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void process_xml(xml::node* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);

	/// \brief It is possible to extend this processor with custom handlers
	void register_attr_handler(const std::string& attr, attr_handler&& handler)
	{
		m_attr_handlers.emplace(attr, std::forward<attr_handler>(handler));
	}

  protected:

	void process_node(xml::node* node, const scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	void process_text(xml::text& t, const scope& scope);
	void post_process(xml::element* e, const scope& parentScope, std::filesystem::path dir, basic_html_controller& webapp);

	// std::vector<std::unique_ptr<xml::node>> resolve_fragment_spec(xml::element* node, std::filesystem::path dir, basic_html_controller& webapp, const std::string& spec, const scope& scope);
	std::vector<std::unique_ptr<xml::node>> resolve_fragment_spec(xml::element* node, std::filesystem::path dir, basic_html_controller& webapp, const json::element& spec, const scope& scope);
	std::vector<std::unique_ptr<xml::node>> resolve_fragment_spec(xml::element* node, std::filesystem::path dir, basic_html_controller& webapp, const std::string& file, const std::string& selector, bool byID);

	// virtual void process_node_attr(xml::node* node, const scope& scope, std::filesystem::path dir);
	AttributeAction process_attr_if(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp, bool unless);
	AttributeAction process_attr_assert(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_text(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp, bool escaped);
	AttributeAction process_attr_switch(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_each(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_attr(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_with(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_generic(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_boolean_value(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_inline(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_append(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp, std::string dest, bool prepend);
	AttributeAction process_attr_classappend(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_styleappend(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);
	AttributeAction process_attr_remove(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp);

	enum class TemplateIncludeAction { include, insert, replace };

	AttributeAction process_attr_include(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_html_controller& webapp, TemplateIncludeAction tia);

	std::map<std::string, attr_handler> m_attr_handlers;
};

}
