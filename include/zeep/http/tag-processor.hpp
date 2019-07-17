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

class template_loader
{
  public:
	virtual ~template_loader() = default;
	virtual void load_template(const std::string& file, xml::document& doc) = 0;
	void load_template(const boost::filesystem::path& file, xml::document& doc)
	{
		load_template(file.string(), doc);
	}
};

class tag_processor
{
  public:
	tag_processor(template_loader& tr, const std::string& ns = "http://www.cmbi.ru.nl/libzeep/ml");

	tag_processor(const tag_processor&) = delete;
	tag_processor& operator=(const tag_processor&) = delete;

	virtual ~tag_processor();

	/// process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir);

	/// The type of the callback for processing tags
	typedef std::function<void(xml::element* node, const el::scope& scope, boost::filesystem::path dir)> processor_type;

	/// To add additional processors
	virtual void add_processor(const std::string& name, processor_type processor);

	/// default handler for mrs:include tags
	virtual void process_include(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:if tags
	virtual void process_if(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:iterate tags
	virtual void process_iterate(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:for tags
	virtual void process_for(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:number tags
	virtual void process_number(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:options tags
	virtual void process_options(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:option tags
	virtual void process_option(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:checkbox tags
	virtual void process_checkbox(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:url tags
	virtual void process_url(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:param tags
	virtual void process_param(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:embed tags
	virtual void process_embed(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

  protected:
	using processor_map = std::map<std::string, processor_type>;

	template_loader& m_template_loader;
	std::string m_ns;
	processor_map m_processor_table;
};

}
}