// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

/// \file
/// definition of the zeep::http::webapp class, a rich extension of the zeep::http::server class 
/// that allows mapping of member functions to mount points in HTTP space.

#include <map>
#include <set>
#include <string>
#include <vector>
#include <mutex>

#include <zeep/config.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/el-processing.hpp>
#include <zeep/http/tag-processor.hpp>
#include <zeep/http/authorization.hpp>

// --------------------------------------------------------------------
//

namespace zeep::http
{

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
/// \brief actual implementation of a zeep::http::resource_loader that loads files from disk
/// 
/// Load the resources from the directory specified in the docroot constructor parameter.

class file_loader : public resource_loader
{
  public:
	/// \brief constructor
	///
	/// \param docroot	Path to the directory where the 'resources' are located
	file_loader(const std::filesystem::path& docroot = ".")
		: resource_loader(), m_docroot(docroot) {}
	
	/// \brief return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept;

	/// \brief basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept;

  private:
	std::filesystem::path m_docroot;
};

// -----------------------------------------------------------------------
/// \brief actual implementation of a zeep::http::resource_loader that loads resources from memory
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

/// \brief base class for a webapp
///
/// basic_webapp is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class basic_webapp
{
  public:
	basic_webapp();

	virtual ~basic_webapp();

	/// \brief set the docroot for a webapp
	virtual void set_docroot(const std::filesystem::path& docroot);

	/// \brief get the current docroot of the webapp
	std::filesystem::path get_docroot() const { return m_docroot; }

	/// \brief Add a new authentication handler
	///
	/// basic_webapp takes ownership.
	/// \param authenticator	The object that will the authentication 
	/// \param login			If true, handlers will be added for /logout and GET and POST /login
	void add_authenticator(authentication_validation_base* authenticator, bool login = false);

	/// \brief Create an error reply for the error containing a validation header
	///
	/// When a authentication violation is encountered, this function is called to generate
	/// the appropriate reply.
	/// \param req		The request that triggered this call
	/// \param stale	For Digest authentication, indicates the authentication information is correct but out of date
	/// \param realm	The name of the protected area, might be shown to the user
	/// \param rep		Write the reply in this object
	virtual void create_unauth_reply(const request& req, bool stale, const std::string& realm, reply& rep);

	/// \brief Create an error reply for the error
	///
	/// An error should be returned with HTTP status code \a status. This method will create a default error page.
	/// \param req		The request that triggered this call
	/// \param realm	The name of the protected area, might be shown to the user
	/// \param rep		Write the reply in this object
	virtual void create_error_reply(const request& req, status_type status, reply& rep);

	/// \brief Create an error reply for the error with an additional message for the user
	///
	/// An error should be returned with HTTP status code \a status and additional information \a message.
	/// This method will create a default error page.
	/// \param req		The request that triggered this call
	/// \param status	The error that triggered this call
	/// \param message	The message describing the error
	/// \param rep		Write the reply in this object
	virtual void create_error_reply(const request& req, status_type status, const std::string& message, reply& rep);

	/// \brief Dispatch and handle the request
	virtual void handle_request(request& req, reply& rep);

	// --------------------------------------------------------------------
	// tag processor support

	/// \brief process all the tags in this node
	virtual void process_tags(xml::node* node, const scope& scope);

	/// \brief get the CSRF token in the request \a req
	std::string get_csrf_token(const request& req) const
	{
		return req.get_cookie("csrf-token");
	}

	/// \brief get the CSRF token from the request burried in \a scope
	std::string get_csrf_token(const scope& scope) const
	{
		return get_csrf_token(scope.get_request());
	}

  protected:

	std::map<std::string,std::function<tag_processor*(const std::string&)>> m_tag_processor_creators;

	/// \brief process only the tags with the specified namespace prefixes
	virtual void process_tags(xml::element* node, const scope& scope, std::set<std::string> registeredNamespaces);

  public:

	/// \brief Use to register a new tag_processor and couple it to a namespace
	template<typename TagProcessor>
	void register_tag_processor(const std::string& ns)
	{
		m_tag_processor_creators.emplace(ns, [](const std::string& ns) { return new TagProcessor(ns.c_str()); });
	}

	/// \brief Create a tag_processor
	tag_processor* create_tag_processor(const std::string& ns) const
	{
		return m_tag_processor_creators.at(ns)(ns);
	}

	// --------------------------------------------------------------------

  public:

	/// \brief webapp works with 'handlers' that are methods 'mounted' on a path in the requested URI

	using handler_type = std::function<void(const request& request, const scope& scope, reply& reply)>;

	/// assign a handler function to a path in the server's namespace
	/// Usually called like this:
	/// \code{.cpp}
	///
	///   mount("page", std::bind(&page_handler, this, _1, _2, _3));
	/// \endcode
	/// Where page_handler is defined as:
	/// \code{.cpp}
	/// void session_server::page_handler(const request& request, const scope& scope, reply& reply);
	/// \endcode
	/// Note, the first parameter is a glob pattern, similar to Ant matching rules.
	/// Supported operators are \*, \*\* and ?. As an addition curly bracketed optional elements are allowed
	/// Also, patterns ending in / are interpreted as ending in /\*\*
	/// 
	/// path             | matches                                     
	/// ---------------- | --------------------------------------------
	/// `**``/``*.js`     | matches x.js, a/b/c.js, etc                 
	/// `{css,scripts}/` | matches css/1/first.css and scripts/index.js

	/// \brief mount a callback on URI path \a path for any HTTP method
	template<class Class>
	void mount(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method_type::UNDEFINED, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP GET method
	template<class Class>
	void mount_get(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method_type::GET, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP POST method
	template<class Class>
	void mount_post(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method_type::POST, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method \a method
	template<class Class>
	void mount(const std::string& path, method_type method, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for any HTTP method, and enforce authentication specified by \a realm
	template<class Class>
	void mount(const std::string& path, const std::string& realm, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method_type::UNDEFINED, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method GET, and enforce authentication specified by \a realm
	template<class Class>
	void mount_get(const std::string& path, const std::string& realm, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method_type::GET, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method POST, and enforce authentication specified by \a realm
	template<class Class>
	void mount_post(const std::string& path, const std::string& realm, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method_type::POST, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for the HTTP method \a method, and enforce authentication specified by \a realm
	template<class Class>
	void mount(const std::string& path, const std::string& realm, method_type method, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<basic_webapp,Class>, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a handler on URI path \a path for HTTP method \a method
	void mount(const std::string& path, method_type method, handler_type handler)
	{
		mount(path, "", method, std::move(handler));
	}

	/// \brief mount a handler on URI path \a path for HTTP method \a method, and enforce authentication specified by \a realm
	void mount(const std::string& path, const std::string& realm, method_type method, handler_type handler)
	{
		auto mp = std::find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
			[path, method](auto& mp)
			{
				return mp.path == path and (mp.method == method or mp.method == method_type::UNDEFINED or method == method_type::UNDEFINED);
			});

		if (mp == m_dispatch_table.end())
			m_dispatch_table.push_back({path, realm, method, handler});
		else
		{
			if (mp->realm != realm)
				throw std::logic_error("realms not equal");

			if (mp->method != method)
				throw std::logic_error("cannot mix method_type::UNDEFINED with something else");

			mp->handler = handler;
		}
	}

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

	/// \brief default GET login handler, will simply return the login page
	///
	/// This is the default handler for `GET /login`. If you want to provide a custom login
	/// page, you have to override this method.
	virtual void handle_get_login(const request& request, const scope& scope, reply& reply);

	/// \brief default POST login handler, will process the credentials from the form in the login page
	///
	/// This is the default handler for `POST /login`. If you want to provide a custom login
	/// procedure, you have to override this method.
	virtual void handle_post_login(const request& request, const scope& scope, reply& reply);

	/// \brief default logout handler, will return a redirect to the base URL and remove the authentication Cookie
	virtual void handle_logout(const request& request, const scope& scope, reply& reply);

  private:

	struct mount_point
	{
		std::string path;
		std::string realm;
		method_type method;
		handler_type handler;
	};

	using mount_point_list = std::vector<mount_point>;

	mount_point_list m_dispatch_table;
	std::string m_ns;
	std::filesystem::path m_docroot;

	std::vector<authentication_validation_base*> m_authentication_validators;
};

// --------------------------------------------------------------------
/// webapp is derived from both zeep::http::server and basic_webapp, it is used to create
/// interactive web applications.

template<typename Loader>
class webapp_base : public http::server, public basic_webapp
{
  public:

	webapp_base(const std::string& docroot = ".")
		: m_loader(docroot)
	{
		register_tag_processor<tag_processor_v1>(tag_processor_v1::ns());
		register_tag_processor<tag_processor_v2>(tag_processor_v2::ns());
	}

	virtual ~webapp_base() {}

	void handle_request(request& req, reply& rep)
	{
		server::get_log() << req.uri;
		basic_webapp::handle_request(req, rep);
	}

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

using file_based_webapp = webapp_base<file_loader>;
using rsrc_based_webapp = webapp_base<rsrc_loader>;

#if WEBAPP_USES_RESOURCES
using webapp = rsrc_based_webapp;
#else
using webapp = file_based_webapp;
#endif

} // namespace http::zeep
