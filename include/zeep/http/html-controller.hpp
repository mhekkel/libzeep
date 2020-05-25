// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::controller class. This class takes
/// care of handling requests that are mapped to call back functions
/// and provides code to return XHTML formatted replies.

#include <zeep/config.hpp>

#include <zeep/http/controller.hpp>
#include <zeep/http/el-processing.hpp>
#include <zeep/http/template-processor.hpp>

// --------------------------------------------------------------------
//

namespace zeep::http
{

// --------------------------------------------------------------------

/// \brief base class for a webapp controller that uses XHTML templates
///
/// html::controller is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class html_controller_base : public controller
{
  public:
	html_controller_base(const std::string& prefix_path = "/")
		: controller(prefix_path)
	{
	}

	/// \brief Dispatch and handle the request
	virtual bool handle_request(request& req, reply& reply);

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
		static_assert(std::is_base_of_v<html_controller_base,Class>, "This call can only be used for methods in classes derived from html_controller_base");
		mount(path, method_type::UNDEFINED, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP GET method
	template<class Class>
	void mount_get(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<html_controller_base,Class>, "This call can only be used for methods in classes derived from html_controller_base");
		mount(path, method_type::GET, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP POST method
	template<class Class>
	void mount_post(const std::string& path, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<html_controller_base,Class>, "This call can only be used for methods in classes derived from html_controller_base");
		mount(path, method_type::POST, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method \a method
	template<class Class>
	void mount(const std::string& path, method_type method, void(Class::*callback)(const request& request, const scope& scope, reply& reply))
	{
		static_assert(std::is_base_of_v<html_controller_base,Class>, "This call can only be used for methods in classes derived from html_controller_base");
		mount(path, method, [server = static_cast<Class*>(this), callback](const request& request, const scope& scope, reply& reply)
			{ (server->*callback)(request, scope, reply); });
	}

	/// \brief mount a handler on URI path \a path for HTTP method \a method
	void mount(const std::string& path, method_type method, handler_type handler)
	{
		auto mpi = std::find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
			[path, method](auto& mp)
			{
				return mp.path == path and (mp.method == method or mp.method == method_type::UNDEFINED or method == method_type::UNDEFINED);
			});

		if (mpi == m_dispatch_table.end())
			m_dispatch_table.emplace_back(path, method, handler);
		else
		{
			if (mpi->method != method)
				throw std::logic_error("cannot mix method_type::UNDEFINED with something else");

			mpi->handler = handler;
		}
	}

	/// \brief Initialize the scope object
	///
	/// The default implementation does nothing, derived implementations may
	/// want to add some default data to the scope.
	virtual void init_scope(scope& scope) {}

  private:

	struct mount_point
	{
		mount_point(const std::string& path, method_type method, handler_type handler)
			: path(path), method(method), handler(handler) {}

		std::string path;
		method_type method;
		handler_type handler;
	};

	using mount_point_list = std::vector<mount_point>;

	mount_point_list m_dispatch_table;
};

// --------------------------------------------------------------------

template<typename PROCESSOR>
class html_controller_template_base : public html_controller_base, public PROCESSOR
{
  public:
	using template_processor = PROCESSOR;

	html_controller_template_base(const std::string& prefix_path, const std::string& docroot)
		: html_controller_base(prefix_path), template_processor(docroot)
	{
	}

	/// \brief default file handling
	virtual void handle_file(const request& request, const scope& scope, reply& reply)
	{
		template_processor::handle_file(request, scope, reply);
	}
};

// --------------------------------------------------------------------

#if WEBAPP_USES_RESOURCES
using html_controller = html_controller_template_base<rsrc_based_html_template_processor>;
#else
using html_controller = html_controller_template_base<file_based_html_template_processor>;
#endif

} // namespace zeep::http
