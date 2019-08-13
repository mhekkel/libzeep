// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

#include <map>
#include <string>
#include <vector>
#include <mutex>

#include <boost/filesystem/path.hpp>

#include <zeep/exception.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/server.hpp>
#include <zeep/el/process.hpp>
#include <zeep/http/tag-processor.hpp>

// --------------------------------------------------------------------
//

namespace zeep
{
namespace http
{

/// webapps can use authentication, this exception is thrown for unauthorized access

struct unauthorized_exception : public std::exception
{
	unauthorized_exception(bool stale, const std::string& realm)
		: m_stale(stale)
	{
		std::string::size_type n = realm.length();
		if (n >= sizeof(m_realm))
			n = sizeof(m_realm) - 1;
		realm.copy(m_realm, n);
		m_realm[n] = 0;
	}
	bool m_stale;	  ///< Is true when the authorization information is valid but stale (too old)
	char m_realm[256]; ///< Realm for which the authorization failed
};

/// basic_webapp is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

/// class to use in authentication
struct auth_info;
typedef std::list<auth_info> auth_info_list;

class basic_webapp
{
  public:
	/// first parameter to constructor is the
	/// namespace to use in template XHTML files.
	basic_webapp(const boost::filesystem::path& docroot = ".");

	virtual ~basic_webapp();

	virtual void set_docroot(const boost::filesystem::path& docroot);
	boost::filesystem::path get_docroot() const { return m_docroot; }

	/// Support for HTTP Authentication, sets the username field in the request
	virtual void validate_authentication(request& request, const std::string& realm)
	{
		request.username = validate_authentication(request.get_header("Authorization"), request.method, request.uri, realm);
	}

	/// Support for HTTP Authentication, returns the validated user name
	virtual std::string validate_authentication(const std::string& authorization,
												method_type method, const std::string& uri, const std::string& realm);

	/// Subclasses should implement this to return the password for the user,
	/// result should be the MD5 hash of the string username + ':' + realm + ':' + password
	virtual std::string get_hashed_password(const std::string& username, const std::string& realm);

	/// Create an error reply for the error containing a validation header
	virtual void create_unauth_reply(const request& req, bool stale, const std::string& realm,
									 const std::string& authentication, reply& rep);

	/// Create an error reply for the error
	virtual void create_error_reply(const request& req, status_type status, reply& rep);

	/// Create an error reply for the error with an additional message for the user
	virtual void create_error_reply(const request& req, status_type status, const std::string& message, reply& rep);

	/// Dispatch and handle the request
	virtual void handle_request(const request& req, reply& rep);

	// --------------------------------------------------------------------
	// tag processor support

  protected:

	std::map<std::string,std::function<tag_processor*(const std::string&)>> m_tag_processor_creators;

	virtual void process_tags(xml::element* node, const el::scope& scope, std::set<std::string> registeredNamespaces);

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

  protected:

	virtual void create_unauth_reply(const request& req, bool stale, const std::string& realm,
									 reply& rep)
	{
		create_unauth_reply(req, stale, realm, "WWW-Authenticate", rep);
	}

	// webapp works with 'handlers' that are methods 'mounted' on a path in the requested URI

	typedef std::function<void(const request& request, const el::scope& scope, reply& reply)> handler_type;

	/// assign a handler function to a path in the server's namespace
	/// Usually called like this:
	///
	///   mount("page", boost::bind(&page_handler, this, _1, _2, _3));
	///
	/// Where page_handler is defined as:
	///
	/// void my_server::page_handler(const request& request, const el::scope& scope, reply& reply);
	///

	template<class Class>
	void mount(const std::string& path, void(Class::*callback)(const request& request, const el::scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, [server = static_cast<Class*>(this), callback](const request& request, const el::scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	template<class Class>
	void mount(const std::string& path, const std::string& realm, void(Class::*callback)(const request& request, const el::scope& scope, reply& reply))
	{
		static_assert(std::is_base_of<basic_webapp,Class>::value, "This call can only be used for methods in classes derived from basic_webapp");
		mount(path, realm, [server = static_cast<Class*>(this), callback](const request& request, const el::scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	void mount(const std::string& path, handler_type handler)
	{
		mount(path, "", std::move(handler));
	}

	/// version of mount that requires authentication
	void mount(const std::string& path, const std::string& realm, handler_type handler)
	{
		auto mp = std::find_if(m_dispatch_table.begin(), m_dispatch_table.end(), [path](auto& mp) -> bool { return mp.path == path; });
		if (mp == m_dispatch_table.end())
			m_dispatch_table.push_back({path, realm, handler});
		else
		{
			if (mp->realm != realm)
				throw std::logic_error("realms not equal");

			mp->handler = handler;
		}
	}

	/// Default handler for serving files out of our doc root
	virtual void handle_file(const request& request, const el::scope& scope, reply& reply);

  public:
	/// Use load_template to fetch the XHTML template file
	virtual void load_template(const std::string& file, xml::document& doc);

	/// create a reply based on a template
	virtual void create_reply_from_template(const std::string& file, const el::scope& scope, reply& reply);

	/// Initialize the el::scope object
	virtual void init_scope(el::scope& scope);

  private:

	struct mount_point
	{
		std::string path;
		std::string realm;
		handler_type handler;
	};

	typedef std::vector<mount_point> mount_point_list;

	mount_point_list m_dispatch_table;
	std::string m_ns;
	boost::filesystem::path m_docroot;
	auth_info_list m_auth_info;
	std::mutex m_auth_mutex;
};

// --------------------------------------------------------------------

/// webapp is derived from both zeep::http::server and basic_webapp, it is used to create
/// interactive web applications.

class webapp : public http::server, public basic_webapp
{
public:
	webapp(const boost::filesystem::path& docroot = ".");

	virtual void handle_request(const request& req, reply& rep);
};

} // namespace http
} // namespace zeep
