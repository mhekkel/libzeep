// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the zeep::template_processor class. This class
/// handles the loading and processing of XHTML files.

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include "zeep/config.hpp"
# include "zeep/http/reply.hpp"
# include "zeep/http/tag-processor.hpp"

# include <zeem/zeem.hpp>

# include <filesystem>
# include <iosfwd>
# include <optional>
# include <set>
# include <string>
# include <system_error>
# include <utility>
#endif

// --------------------------------------------------------------------
//

namespace zeep::http
{

ZEEP_EXPORT class request;
ZEEP_EXPORT class html_controller;

// -----------------------------------------------------------------------
/// \brief abstract base class for a resource loader
///
/// A resource loader is used to fetch the resources a webapp can serve
/// This is an abstract base class, use either file_loader to load files
/// from a 'docroot' directory or rsrc_loader to load files from compiled in
/// resources. (See https://forge.hekkelman.net/maarten/mrc for more info on resources)

ZEEP_EXPORT class resource_loader
{
  public:
	virtual ~resource_loader() = default;

	resource_loader(const resource_loader &) = delete;
	resource_loader &operator=(const resource_loader &) = delete;

	/// \brief return last_write_time of \a file
	/// \param file  The path to the file to check
	/// \param ec    Error code set on failure
	/// \return      The last write time of the file
	virtual std::filesystem::file_time_type file_time(std::filesystem::path file, std::error_code &ec) noexcept = 0;

	/// \brief basic loader, returns error in ec if file was not found
	/// \param file  The path to the file to load
	/// \param ec    Error code set on failure
	/// \return      A unique pointer to an input stream, or nullptr on error
	virtual std::unique_ptr<std::istream> load_file(std::string file, std::error_code &ec) noexcept = 0;

  protected:
	resource_loader() = default;
};

// -----------------------------------------------------------------------
/// \brief actual implementation of a zeep::resource_loader that loads files from disk
///
/// Load the resources from the directory specified in the docroot constructor parameter.

ZEEP_EXPORT class file_loader : public resource_loader
{
  public:
	/// \brief constructor
	///
	/// \param docroot	Path to the directory where the 'resources' are located
	///
	/// Throws a runtime_error if the docroot does not exist
	file_loader(std::filesystem::path docroot);

	/// \brief return last_write_time of \a file
	/// \param file  The path to the file to check
	/// \param ec    Error code set on failure
	/// \return      The last write time of the file
	std::filesystem::file_time_type file_time(std::filesystem::path file, std::error_code &ec) noexcept override;

	/// \brief basic loader, returns error in ec if file was not found
	/// \param file  The path to the file to load
	/// \param ec    Error code set on failure
	/// \return      A unique pointer to an input stream, or nullptr on error
	std::unique_ptr<std::istream> load_file(std::string file, std::error_code &ec) noexcept override;

  private:
	std::filesystem::path m_docroot;
};

#if USE_RSRC
// -----------------------------------------------------------------------
/// \brief actual implementation of a zeep::resource_loader that loads resources from memory
///
/// Load the resources from resource data created with mrc (see https://forge.hekkelman.net/maarten/mrc )

ZEEP_EXPORT class rsrc_loader : public resource_loader
{
  public:
	/// \brief constructor
	///
	/// The parameter is not used
	rsrc_loader(const std::filesystem::path & /* unused */);

	/// \brief return last_write_time of \a file
	/// \param file  The path to the file to check
	/// \param ec    Error code set on failure
	/// \return      The last write time of the file
	std::filesystem::file_time_type file_time(std::filesystem::path file, std::error_code &ec) noexcept override;

	/// \brief basic loader, returns error in ec if file was not found
	/// \param file  The path to the file to load
	/// \param ec    Error code set on failure
	/// \return      A unique pointer to an input stream, or nullptr on error
	std::unique_ptr<std::istream> load_file(std::string file, std::error_code &ec) noexcept override;

  private:
	std::filesystem::file_time_type mRsrcWriteTime = {};
};
#endif

// --------------------------------------------------------------------

/// \brief base class for template processors
///
/// template_processor is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

ZEEP_EXPORT class basic_template_processor
{
  public:
	/// \brief Construct a processor with the given docroot
	/// \param docroot  Path to the document root directory
	basic_template_processor(std::filesystem::path docroot)
		: m_docroot(std::move(docroot))
	{
	}

	virtual ~basic_template_processor() = default;

	/// \brief set the docroot for this processor
	/// \param docroot  The new document root path
	virtual void set_docroot(std::filesystem::path docroot);

	/// \brief get the current docroot of this processor
	/// \return The document root path
	[[nodiscard]] std::filesystem::path get_docroot() const { return m_docroot; }

	// --------------------------------------------------------------------
	// tag processor support

	/// \brief process all the tags in this node
	/// \param node   The root XML node to process
	/// \param scope  The scope containing variables and request data
	virtual void process_tags(zeem::node *node, const scope &scope);

  protected:
	std::map<std::string, std::function<tag_processor_base *(const std::string &)>> m_tag_processor_creators;

	/// \brief process only the tags with the specified namespace prefixes
	/// \param node                  The XML element to process
	/// \param scope                 The scope containing variables and request data
	/// \param registeredNamespaces  The set of namespace prefixes to process
	virtual void process_tags(zeem::element *node, const scope &scope, std::set<std::string> registeredNamespaces);

  public:
	/// \brief Use to register a new tag_processor and couple it to a namespace
	/// \tparam TagProcessor  The tag processor type, must derive from \a tag_processor_base
	/// \param ns             The namespace to associate with the processor (defaults to TagProcessor::ns())
	template <typename TagProcessor>
	void register_tag_processor(const std::string &ns = TagProcessor::ns())
	{
		m_tag_processor_creators.emplace(ns, [](const std::string &ns)
			{ return new TagProcessor(ns.c_str()); });
	}

	/// \brief Create a tag_processor
	/// \param ns  The namespace of the processor to create
	/// \return    A pointer to the newly created \a tag_processor_base
	[[nodiscard]] tag_processor_base *create_tag_processor(const std::string &ns) const
	{
		return m_tag_processor_creators.at(ns)(ns);
	}

	// --------------------------------------------------------------------

  public:
	/// \brief return last_write_time of \a file
	/// \param file  The file path to check
	/// \param ec    Error code set on failure
	/// \return      The last write time of the file
	virtual std::filesystem::file_time_type file_time(const std::string &file, std::error_code &ec) noexcept = 0;

	/// \brief return error in ec if file was not found
	/// \param file  The file path to load
	/// \param ec    Error code set on failure
	/// \return      A unique pointer to an input stream, or nullptr on error
	virtual std::unique_ptr<std::istream> load_file(const std::string &file, std::error_code &ec) noexcept = 0;

  public:
	/// \brief Use load_template to fetch the XHTML template file
	/// \param file  The path to the template file
	/// \param doc   The document to load the template into
	virtual void load_template(const std::string &file, zeem::document &doc);

	/// \brief Check if the argument \a file contains a valid reference to an XHTML template file and return the path, if any.
	/// \param file  The file path to check
	/// \return      The valid template path, or std::nullopt if not valid
	virtual std::optional<std::filesystem::path> get_template_file(const std::string &file);

	/// \brief create a reply based on a template
	/// \param file   The template file path
	/// \param scope  The scope containing variables and request data
	/// \param reply  The reply object to populate
	virtual void create_reply_from_template(const std::string &file, const scope &scope, reply &reply);

	/// \brief create a reply based on a template, alternate version
	/// \param file   The template file path
	/// \param scope  The scope containing variables and request data
	/// \return       The populated reply object
	[[nodiscard]] reply create_reply_from_template(const std::string &file, const scope &scope)
	{
		reply result = reply::stock_reply(status_type::ok);
		create_reply_from_template(file, scope, result);
		return result;
	}

	/// \brief Default handler for serving files out of our doc root
	/// \param scope  The scope containing variables and request data
	/// \return       A reply with the file contents or a 404
	[[nodiscard]] reply create_reply_for_get_file(const scope &scope);

	/// \brief Initialize the scope object
	/// \param req    The HTTP request
	/// \param scope  The scope to initialize
	virtual void init_scope(request &req, scope &scope);

  protected:
	std::string m_ns;
	std::filesystem::path m_docroot;
};

// --------------------------------------------------------------------
/// \brief actual implementation of the abstract basic_template_processor
///
/// \tparam Loader  The resource loader type, either \a file_loader or \a rsrc_loader

ZEEP_EXPORT template <typename Loader>
class html_template_processor : public basic_template_processor
{
  public:
	/// \brief Construct an HTML template processor
	/// \param docroot                 The document root path
	/// \param addDefaultTagProcessors Whether to register default tag processors
	html_template_processor(const std::filesystem::path &docroot = {}, bool addDefaultTagProcessors = true)
		: basic_template_processor(docroot)
		, m_loader(docroot)
	{
		if (addDefaultTagProcessors)
			register_tag_processor<tag_processor>();
	}

	~html_template_processor() override = default;

	/// \brief return last_write_time of \a file
	/// \param file  The file path to check
	/// \param ec    Error code set on failure
	/// \return      The last write time of the file
	[[nodiscard]] std::filesystem::file_time_type file_time(const std::string &file, std::error_code &ec) noexcept override
	{
		return m_loader.file_time(file, ec);
	}

	/// \brief basic loader, returns error in ec if file was not found
	/// \param file  The file path to load
	/// \param ec    Error code set on failure
	/// \return      A unique pointer to an input stream, or nullptr on error
	[[nodiscard]] std::unique_ptr<std::istream> load_file(const std::string &file, std::error_code &ec) noexcept override
	{
		return m_loader.load_file(file, ec);
	}

  protected:
	Loader m_loader; ///< The resource loader instance
};

/// \brief HTML template processor using \a file_loader
ZEEP_EXPORT using file_based_html_template_processor = html_template_processor<file_loader>;

#if USE_RSRC
/// \brief HTML template processor using \a rsrc_loader
ZEEP_EXPORT using rsrc_based_html_template_processor = html_template_processor<rsrc_loader>;
#endif

/// \brief the actual definition of zeep::template_processor

#if WEBAPP_USES_RESOURCES and USE_RSRC
ZEEP_EXPORT using template_processor = rsrc_based_html_template_processor;
#else
ZEEP_EXPORT using template_processor = file_based_html_template_processor;
#endif

} // namespace zeep::http
