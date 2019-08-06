//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <zeep/http/webapp.hpp>
#include <zeep/http/tag-processor.hpp>

namespace zeep
{

namespace http
{

class tag_processor_v2 : public tag_processor
{
  public:
	enum class AttributeAction
	{
		none, remove, replace
	};

	using attr_handler = std::function<AttributeAction(xml::element*, xml::attribute*, const el::scope&, boost::filesystem::path, basic_webapp& webapp)>;

	tag_processor_v2(const std::string& ns = "http://www.hekkelman.com/libzeep/m2");

	/// process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	void register_attr_handler(const std::string& attr, attr_handler&& handler)
	{
		m_attr_handlers.emplace(attr, std::forward<attr_handler>(handler));
	}

  protected:

	// virtual void process_node_attr(xml::node* node, const el::scope& scope, boost::filesystem::path dir);
	AttributeAction process_attr_if(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_text(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp, bool escaped);
	AttributeAction process_attr_each(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);
	AttributeAction process_attr_attr(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	AttributeAction process_attr_generic(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp);

	std::map<std::string, attr_handler> m_attr_handlers;
};

}
}