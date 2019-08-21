//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/filesystem/path.hpp>

#include <zeep/xml/document.hpp>
#include <zeep/el/process.hpp>

namespace zeep
{

namespace http
{

class basic_webapp;

// --------------------------------------------------------------------
//

/// Base class for tag_processor. Note that this class should be light
/// in construction, we create it every time a page is rendered.

class tag_processor
{
  public:
	tag_processor(const tag_processor&) = delete;
	tag_processor& operator=(const tag_processor&) = delete;

	virtual ~tag_processor() = default;

	/// process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp) = 0;

  protected:

	tag_processor(const char* ns)
		: m_ns(ns) {}

	std::string	m_ns;
};

// --------------------------------------------------------------------

/// At tag_processor compatible with the old version of libzeep. Works
/// on tags only, not on attributes. Also parses any occurrence of ${}.
/// For newer code, please consider using the v2 version only.

class tag_processor_v1 : public tag_processor
{
  public:
	static constexpr const char* ns() { return "http://www.hekkelman.com/libzeep/m1"; }

	tag_processor_v1(const char* ns = tag_processor_v1::ns());

	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

  protected:

	virtual void process_tag(const std::string& tag, xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

  private:

	/// handler for mrs:include tags
	void process_include(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_if(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_iterate(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_for(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_number(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_options(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_option(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_checkbox(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_url(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_param(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	void process_embed(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	bool process_el(const el::scope& scope, std::string& s);
};

// --------------------------------------------------------------------

/// This is the new tag_processor. It is designed to look a bit like
/// Thymeleaf (https://www.thymeleaf.org/)
/// This processor works on attributes mostly, but can process inline
/// el constructs as well.

class tag_processor_v2 : public tag_processor
{
  public:
	static constexpr const char* ns() { return "http://www.hekkelman.com/libzeep/m2"; }

	enum class AttributeAction
	{
		none, remove, replace
	};

	using attr_handler = std::function<AttributeAction(xml::element*, xml::attribute*, el::scope&, boost::filesystem::path, basic_webapp& webapp)>;

	tag_processor_v2(const char* ns = tag_processor_v2::ns());

	/// process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	void register_attr_handler(const std::string& attr, attr_handler&& handler)
	{
		m_attr_handlers.emplace(attr, std::forward<attr_handler>(handler));
	}

  protected:

	// virtual void process_node_attr(xml::node* node, const el::scope& scope, boost::filesystem::path dir);
	AttributeAction process_attr_if(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp, bool unless);
	AttributeAction process_attr_text(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp, bool escaped);
	AttributeAction process_attr_switch(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_each(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_attr(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_with(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_generic(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_boolean_value(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_inline(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	std::map<std::string, attr_handler> m_attr_handlers;
};

}
}
