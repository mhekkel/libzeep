//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::template_processor class. This class
/// handles the loading and processing of XHTML files.

#include <zeep/config.hpp>

#include <set>
#include <filesystem>

#include <zeep/http/el-processing.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/http/tag-processor.hpp>

// --------------------------------------------------------------------
//

namespace zeep::http
{

class html_controller;

// -----------------------------------------------------------------------
/// \brief abstract base class for a resource loader
///
/// A resource loader is used to fetch the resources a webapp can serve
/// This is an abstract base class, use either file_loader to load files
/// from a 'docroot' directory or rsrc_loader to load files from compiled in
/// resources. (See https://github.com/mhekkel/mrc for more info on resources)

class resource_loader
{
  public:
	virtual ~resource_loader() {}

	resource_loader(const resource_loader&) = delete;
	resource_loader& operator=(const resource_loader&) = delete;

	/// \brief return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept = 0;

	/// \brief basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept = 0;

  protected:
	resource_loader() {}
};

// -----------------------------------------------------------------------
/// \brief actual implementation of a zeep::resource_loader that loads files from disk
/// 
/// Load the resources from the directory specified in the docroot constructor parameter.

class file_loader : public resource_loader
{
  public:
	/// \brief constructor
	///
	/// \param docroot	Path to the directory where the 'resources' are located
	///
	/// Throws a runtime_error if the docroot does not exist
	file_loader(const std::filesystem::path& docroot = ".");
	
	/// \brief return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept;

	/// \brief basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept;

  private:
	std::filesystem::path m_docroot;
};

// -----------------------------------------------------------------------
/// \brief actual implementation of a zeep::resource_loader that loads resources from memory
/// 
/// Load the resources from resource data created with mrc (see https://github.com/mhekkel/mrc )

class rsrc_loader : public resource_loader
{
  public:
	/// \brief constructor
	/// 
	/// The parameter is not used
	rsrc_loader(const std::string&);
	
	/// \brief return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept;

	/// \brief basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept;

  private:
	std::filesystem::file_time_type mRsrcWriteTime = {};
};

// --------------------------------------------------------------------

/// \brief base class for template processors
///
/// template_processor is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class basic_template_processor
{
  public:
	basic_template_processor(const std::string& docroot)
		: m_docroot(docroot) {}

	virtual ~basic_template_processor() {}

	/// \brief set the docroot for this processor
	virtual void set_docroot(const std::filesystem::path& docroot);

	/// \brief get the current docroot of this processor
	std::filesystem::path get_docroot() const { return m_docroot; }

	// --------------------------------------------------------------------
	// tag processor support

	/// \brief process all the tags in this node
	virtual void process_tags(xml::node* node, const scope& scope);

  protected:

	std::map<std::string,std::function<tag_processor*(const std::string&)>> m_tag_processor_creators;

	/// \brief process only the tags with the specified namespace prefixes
	virtual void process_tags(xml::element* node, const scope& scope, std::set<std::string> registeredNamespaces);

  public:

	/// \brief Use to register a new tag_processor and couple it to a namespace
	template<typename TagProcessor>
	void register_tag_processor(const std::string& ns = TagProcessor::ns())
	{
		m_tag_processor_creators.emplace(ns, [](const std::string& ns) { return new TagProcessor(ns.c_str()); });
	}

	/// \brief Create a tag_processor
	tag_processor* create_tag_processor(const std::string& ns) const
	{
		return m_tag_processor_creators.at(ns)(ns);
	}

	// --------------------------------------------------------------------
	
	/// \brief Default handler for serving files out of our doc root
	virtual void handle_file(const request& request, const scope& scope, reply& reply);

  public:

	/// \brief return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept = 0;

	/// \brief return error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept = 0;

  public:

	/// \brief Use load_template to fetch the XHTML template file
	virtual void load_template(const std::string& file, xml::document& doc);

	/// \brief create a reply based on a template
	virtual void create_reply_from_template(const std::string& file, const scope& scope, reply& reply);

	/// \brief Initialize the scope object
	virtual void init_scope(scope& scope);

  protected:

	std::string m_ns;
	std::filesystem::path m_docroot;
};

// --------------------------------------------------------------------
/// \brief actual implementation of the abstract basic_template_processor

template<typename Loader>
class html_template_processor : public basic_template_processor
{
  public:

	html_template_processor(const std::string& docroot = "", bool addDefaultTagProcessors = true)
		: basic_template_processor(docroot), m_loader(docroot)
	{
		if (addDefaultTagProcessors)
		{
			register_tag_processor<tag_processor_v1>();
			register_tag_processor<tag_processor_v2>();
		}
	}

	virtual ~html_template_processor() {}

	/// return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept
	{
		return m_loader.file_time(file, ec);
	}

	// basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept
	{
		return m_loader.load_file(file, ec);
	}

  protected:
	Loader m_loader;
};

using file_based_html_template_processor = html_template_processor<file_loader>;
using rsrc_based_html_template_processor = html_template_processor<rsrc_loader>;

/// \brief the actual definition of zeep::template_processor

#if WEBAPP_USES_RESOURCES
using template_processor = rsrc_based_html_template_processor;
#else
using template_processor = file_based_html_template_processor;
#endif

}