//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <zeep/xml/document.hpp>

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
	tag_processor(const std::string& ns)
		: m_ns(ns) {}

	tag_processor(const tag_processor&) = delete;
	tag_processor& operator=(const tag_processor&) = delete;

	virtual ~tag_processor() = default;

	/// process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp) = 0;

  protected:
	std::string	m_ns;
};

// --------------------------------------------------------------------

/// At tag_processor compatible with the old version of libzeep. Works
/// on tags only, not on attributes. Also parses any occurrence of ${}.

class tag_processor_v1 : public tag_processor
{
  public:
	tag_processor_v1(const std::string& ns = "http://www.hekkelman.com/libzeep/m1");

	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

  protected:

	virtual void process_tag(const std::string& tag, xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

  private:

	/// handler for mrs:include tags
	void process_include(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:if tags
	void process_if(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:iterate tags
	void process_iterate(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:for tags
	void process_for(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:number tags
	void process_number(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:options tags
	void process_options(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:option tags
	void process_option(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:checkbox tags
	void process_checkbox(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:url tags
	void process_url(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:param tags
	void process_param(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	/// handler for mrs:embed tags
	void process_embed(xml::element* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
};

}
}