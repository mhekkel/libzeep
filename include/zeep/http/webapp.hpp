// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

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
/// A resource loader is used to fetch the resources a webapp can serve
/// This is an abstract base class, use either file_loader to load files
/// from a 'docroot' directory or rsrc_loader to load files from compiled in
/// resources.

class resource_loader
{
  public:
	virtual ~resource_loader() {}

	resource_loader(const resource_loader&) = delete;
	resource_loader& operator=(const resource_loader&) = delete;

	/// return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept = 0;

	// basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept = 0;

  protected:
	resource_loader() {}
};

// -----------------------------------------------------------------------
/// Load the resources from disk

class file_loader : public resource_loader
{
  public:
	file_loader(const std::filesystem::path& docroot = ".")
		: resource_loader(), m_docroot(docroot) {}
	
	/// return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept;

	// basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept;

  private:
	std::filesystem::path m_docroot;
};

// -----------------------------------------------------------------------
/// Load the resources from rsrc created with mrc (see https://github.com/mhekkel/mrc )

class rsrc_loader : public resource_loader
{
  public:
	rsrc_loader(const std::string& docroot);
	
	/// return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept;

	// basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept;

  private:
	std::filesystem::file_time_type mRsrcWriteTime = {};
};

// --------------------------------------------------------------------

/// basic_webapp is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class basic_webapp
{
  public:
	basic_webapp();

	virtual ~basic_webapp();

	virtual void set_docroot(const std::filesystem::path& docroot);
	std::filesystem::path get_docroot() const { return m_docroot; }

	/// Set the authentication handler, basic_webapp takes ownership.
	/// \param login If true, this class will handle a POST to /login with username/password
	void set_authenticator(authentication_validation_base* authenticator, bool login = false);

	/// Create an error reply for the error containing a validation header
	virtual void create_unauth_reply(const request& req, bool stale, const std::string& realm, reply& rep);

	/// Create an error reply for the error
	virtual void create_error_reply(const request& req, status_type status, reply& rep);

	/// Create an error reply for the error with an additional message for the user
	virtual void create_error_reply(const request& req, status_type status, const std::string& message, reply& rep);

	/// Dispatch and handle the request
	virtual void handle_request(request& req, reply& rep);

	// --------------------------------------------------------------------
	// tag processor support

	/// process all the tags in this node
	virtual void process_tags(xml::node* node, const scope& scope);

	/// CSRF token
	std::string get_csrf_token(const request& req) const
	{
		return req.get_cookie("csrf-token");
	}

	std::string get_csrf_token(const scope& scope) const
	{
		return get_csrf_token(scope.get_request());
	}

  protected:

	std::map<std::string,std::function<tag_processor*(const std::string&)>> m_tag_processor_creators;

	/// process only the tags with the specified namespace prefixes
	virtual void process_tags(xml::element* node, const scope& scope, std::set<std::string> registeredNamespaces);

  public:

	/// Use to register a new tag_processor and couple it to a namespace
	template<typename TagProcessor>
	void register_tag_processor(const std::string& ns)
	{
		m_tag_processor_creators.emplace(ns, [](const std::string& ns) { return new TagProcessor(ns.c_str()); });
	}

	/// Create a tag_processor
	tag_processor* create_tag_processor(const std::string& ns) const
	{
		return m_tag_processor_creators.at(ns)(ns);
	}

	// --------------------------------------------------------------------

  public:

	// webapp works with 'handlers' that are methods 'mounted' on a path in the requested URI

	typedef std::function<void(const request& request, const scope& scope, reply& reply)> handler_type;

	/// assign a handler function to a path in the server's namespace
	/// Usually called like this:
	///
	///   mount("page", boost::bind(&page_handler, this, _1, _2, _3));
	///
	/// Where page_handler is defined as:
	///
	/// void session_server::page_handler(const request& request, const scope& scope, reply& reply);
	///
	/// Note, the first parameter is a glob pattern, similar to Ant matching rules.
	/// Supported operators are *, ** and ?. As an addition curly bracketed optional elements are allowed
	/// Also, patterns ending in / are interpreted as ending in /**
	/// 
	/// e.g.
	/// **/*.js           matches x.js, a/b/c.js, etc
	/// {css,scripts}/    matches css/1/first.css and scripts/index.js


	template<class Class>
	void mount(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method_type::UNDEFINED, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount_get(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method_type::GET, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount_post(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method_type::POST, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount(const std::string& path, method_type method, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, method, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount(const std::string& path, const std::string& realm, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method_type::UNDEFINED, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount_get(const std::string& path, const std::string& realm, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method_type::GET, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount_post(const std::string& path, const std::string& realm, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method_type::POST, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount(const std::string& path, const std::string& realm, method_type method, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, method, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	void mount(const std::string& path, method_type method, handler_type handler)
	{
		mount(path, "", method, std::move(handler));
	}

	/// version of mount that requires authentication
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

	/// Default handlers for serving files out of our doc root
	virtual void handle_file(const request& request, const scope& scope, reply& reply);

  public:

	/// return last_write_time of \a file
	virtual std::filesystem::file_time_type file_time(const std::string& file, std::error_code& ec) noexcept = 0;

	// basic loader, returns error in ec if file was not found
	virtual std::istream* load_file(const std::string& file, std::error_code& ec) noexcept = 0;

  public:

	/// Use load_template to fetch the XHTML template file
	virtual void load_template(const std::string& file, xml::document& doc);

	/// create a reply based on a template
	virtual void create_reply_from_template(const std::string& file, const scope& scope, reply& reply);

	/// Initialize the scope object
	virtual void init_scope(scope& scope);

  protected:

	virtual void handle_get_login(const request& request, const scope& scope, reply& reply);
	virtual void handle_post_login(const request& request, const scope& scope, reply& reply);
	virtual void handle_logout(const request& request, const scope& scope, reply& reply);

  private:

	struct mount_point
	{
		std::string path;
		std::string realm;
		method_type method;
		handler_type handler;
	};

	typedef std::vector<mount_point> mount_point_list;

	mount_point_list m_dispatch_table;
	std::string m_ns;
	std::filesystem::path m_docroot;

	std::unique_ptr<authentication_validation_base> m_authenticator;
};

// --------------------------------------------------------------------
/// webapp is derived from both zeep::http::server and basic_webapp, it is used to create
/// interactive web applications.

template<typename Loader>
class webapp_base : public http::server, public basic_webapp
{
  public:

	// template<typename... Args>
	// webapp_base(Args... args)
	// 	: m_loader(std::forward<Args>(args)...)
	// {
	// 	register_tag_processor<tag_processor_v1>(tag_processor_v1::ns());
	// 	register_tag_processor<tag_processor_v2>(tag_processor_v2::ns());
	// }

	webapp_base(const std::string& docroot = ".")
		: m_loader(docroot)
	{
		register_tag_processor<tag_processor_v1>(tag_processor_v1::ns());
		register_tag_processor<tag_processor_v2>(tag_processor_v2::ns());
	}

	virtual ~webapp_base() {}

	void handle_request(request& req, reply& rep)
	{
		server::log() << req.uri;
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
