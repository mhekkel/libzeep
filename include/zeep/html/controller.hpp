// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::html::controller class. This class takes
/// care of handling requests that are mapped to call back functions
/// and provides code to return XHTML formatted replies.

#include <map>
#include <set>
#include <string>
#include <vector>
#include <mutex>

#include <zeep/config.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/controller.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/authorization.hpp>
#include <zeep/html/el-processing.hpp>
#include <zeep/html/tag-processor.hpp>
#include <zeep/html/template-processor.hpp>

// --------------------------------------------------------------------
//

namespace zeep::html
{

// --------------------------------------------------------------------

/// \brief base class for a webapp controller
///
/// html::controller is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class controller : public http::controller, public template_processor
{
  public:
	controller(const std::string& prefix_path = "/", const std::string& docroot = ".");

	virtual ~controller();

	/// \brief Dispatch and handle the request
	virtual bool handle_request(http::request& req, http::reply& reply);

	// --------------------------------------------------------------------

  public:

	/// \brief webapp works with 'handlers' that are methods 'mounted' on a path in the requested URI

	using handler_type = std::function<void(const http::request& request, const scope& scope, http::reply& reply)>;

	/// assign a handler function to a path in the server's namespace
	/// Usually called like this:
	/// \code{.cpp}
	///
	///   mount("page", std::bind(&page_handler, this, _1, _2, _3));
	/// \endcode
	/// Where page_handler is defined as:
	/// \code{.cpp}
	/// void session_server::page_handler(const http::request& request, const scope& scope, http::reply& reply);
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
	void mount(const std::string& path, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, http::method_type::UNDEFINED, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP GET method
	template<class Class>
	void mount_get(const std::string& path, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, http::method_type::GET, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP POST method
	template<class Class>
	void mount_post(const std::string& path, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, http::method_type::POST, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method \a method
	template<class Class>
	void mount(const std::string& path, http::method_type method, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, method, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for any HTTP method, and enforce authentication specified by \a realm
	template<class Class>
	void mount(const std::string& path, const std::string& realm, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, realm, http::method_type::UNDEFINED, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method GET, and enforce authentication specified by \a realm
	template<class Class>
	void mount_get(const std::string& path, const std::string& realm, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, realm, http::method_type::GET, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method POST, and enforce authentication specified by \a realm
	template<class Class>
	void mount_post(const std::string& path, const std::string& realm, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, realm, http::method_type::POST, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for the HTTP method \a method, and enforce authentication specified by \a realm
	template<class Class>
	void mount(const std::string& path, const std::string& realm, http::method_type method, void(Class::*callback)(const http::request& request, const scope& scope, http::reply& reply))
	{
		static_assert(std::is_base_of_v<controller,Class>, "This call can only be used for methods in classes derived from basic_html_controller");
		mount(path, realm, method, [server = static_cast<Class*>(this), callback](const http::request& request, const scope& scope, http::reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a handler on URI path \a path for HTTP method \a method
	void mount(const std::string& path, http::method_type method, handler_type handler)
	{
		mount(path, "", method, std::move(handler));
	}

	/// \brief mount a handler on URI path \a path for HTTP method \a method, and enforce authentication specified by \a realm
	void mount(const std::string& path, const std::string& realm, http::method_type method, handler_type handler)
	{
		auto mp = std::find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
			[path, method](auto& mp)
			{
				return mp.path == path and (mp.method == method or mp.method == http::method_type::UNDEFINED or method == http::method_type::UNDEFINED);
			});

		if (mp == m_dispatch_table.end())
			m_dispatch_table.push_back({path, realm, method, handler});
		else
		{
			if (mp->realm != realm)
				throw std::logic_error("realms not equal");

			if (mp->method != method)
				throw std::logic_error("cannot mix http::method_type::UNDEFINED with something else");

			mp->handler = handler;
		}
	}

	/// \brief Initialize the scope object
	///
	/// The default implementation does nothing, derived implementations may
	/// want to add some default data to the scope.
	virtual void init_scope(scope& scope);

  private:

	struct mount_point
	{
		std::string path;
		std::string realm;
		http::method_type method;
		handler_type handler;
	};

	using mount_point_list = std::vector<mount_point>;

	mount_point_list m_dispatch_table;
	std::vector<http::authentication_validation_base*> m_authentication_validators;
};

} // namespace zeep::html
