// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2026
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::controller class. This class takes
/// care of handling requests that are mapped to call back functions
/// and provides code to return XHTML formatted replies.

#include "zeep/exception.hpp"
#include "zeep/http/controller.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/scope.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// --------------------------------------------------------------------
//

namespace zeep::http
{

class basic_template_processor;

// --------------------------------------------------------------------

/// \brief base class for a webapp controller that uses XHTML templates
///
/// html::controller is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class html_controller : public controller
{
  public:
	html_controller(const std::string &prefix_path = "/")
		: controller(prefix_path)
	{
	}

	/// \brief return the basic_template_processor of the server
	basic_template_processor &get_template_processor();

	/// \brief return the basic_template_processor of the server
	[[nodiscard]] const basic_template_processor &get_template_processor() const;

	/// \brief default file handling
	///
	/// This method will ask the server for the default template processor
	/// to load the actual file. If there is no template processor set,
	/// it will therefore throw an exception.
	virtual reply handle_file(const scope &scope_);

	// --------------------------------------------------------------------
  public:
	/// @cond

	template <typename Callback, typename...>
	struct html_mount_point
	{
	};

	/// \brief templated base class for mount points
	template <typename ControllerType, typename... Args>
	struct html_mount_point<reply (ControllerType::*)(const scope &scope_, Args...)> : mount_point_base
	{
		using Sig = reply (ControllerType::*)(const scope &, Args...);
		using ArgsTuple = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<Args>>...>;
		using Callback = std::function<reply(const scope &, Args...)>;

		static constexpr size_t N = sizeof...(Args);

		template <typename... Names>
		html_mount_point(std::string path, std::string method, html_controller *owner, Sig sig, Names... names)
			: mount_point_base(std::move(path), std::move(method))
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");

			auto *controller = dynamic_cast<ControllerType *>(owner);
			if (controller == nullptr)
				throw exception("Invalid controller for callback");

			m_callback = [controller, sig](const scope &scope_, Args... args)
			{
				return (controller->*sig)(scope_, std::move(args)...);
			};

			set_names(names...);
		}

		reply call(const scope &scope) override
		{
			auto args = collect_arguments(scope, std::make_index_sequence<N>());
			return std::apply(m_callback, std::move(args));
		}

		template <std::size_t... I>
		auto collect_arguments(const scope &scope, std::index_sequence<I...> /*unused*/)
		{
			return std::make_tuple(scope, get_parameter(scope, m_names[I].c_str(), typename std::tuple_element_t<I, ArgsTuple>{})...);
		}

		Callback m_callback;
	};

	/// @endcond

	/// assign a handler function to a path in the server's namespace, new version
	/// Usually called like this:
	/// \code{.cpp}
	///   map("page", &my_controller::page_handler, "param");
	/// \endcode
	/// Where page_handler is defined as:
	/// \code{.cpp}
	/// zeep::http::reply my_controller::page_handler(const scope& scope, std::optional<int> param);
	/// \endcode
	/// Note, the first parameter is a glob pattern, similar to Ant matching rules.
	/// Supported operators are \*, \*\* and ?. As an addition curly bracketed optional objects are allowed
	/// as well as semi-colons that define separate paths.
	/// Also, patterns ending in / are interpreted as ending in /\*\*
	///
	/// path             | matches
	/// ---------------- | --------------------------------------------
	/// **/*.js          | matches x.js, a/b/c.js, etc
	/// {css,scripts}/   | matches e.g. css/1/first.css and scripts/index.js
	/// a;b;c            | matches either a, b or c
	///
	/// The \a mountPoint parameter is the local part of the mount point.
	/// It can contain parameters enclosed in curly brackets.
	///
	/// For example, say we need a REST call to get the status of shoppingcart
	/// where the browser will send:
	///
	///		GET /ajax/cart/1234/status
	///
	/// Our callback will look like this, for a class my_ajax_handler constructed
	/// with prefixPath `/ajax`:
	/// \code{.cpp}
	/// CartStatus my_ajax_handler::handle_get_status(int id);
	/// \endcode
	/// Then we mount this callback like this:
	/// \code{.cpp}
	/// map_get("/cart/{id}/status", &my_ajax_handler::handle_get_status, "id");
	/// \endcode
	///
	/// The number of \a names of the paramers specified should be equal to the number of
	/// actual arguments for the callback, otherwise the compiler will complain.
	///
	/// Arguments not found in the path will be fetched from the payload in case of a POST
	/// or from the URI parameters otherwise.

	/// \brief map \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Callback, typename... ArgNames>
	void map(std::string mountPoint, std::string method, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new html_mount_point<Callback>(std::move(mountPoint), std::move(method), this, callback, names...));
	}

	/// \brief map a POST to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Callback, typename... ArgNames>
	void map_post(std::string mountPoint, Callback callback, ArgNames... names)
	{
		map(std::move(mountPoint), "POST", callback, names...);
	}

	/// \brief map a PUT to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_put(std::string mountPoint, Sig callback, ArgNames... names)
	{
		map(std::move(mountPoint), "PUT", callback, names...);
	}

	/// \brief map a GET to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_get(std::string mountPoint, Sig callback, ArgNames... names)
	{
		map(std::move(mountPoint), "GET", callback, names...);
	}

	/// \brief map a DELETE to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_delete(std::string mountPoint, Sig callback, ArgNames... names)
	{
		map(std::move(mountPoint), "DELETE", callback, names...);
	}

	/// \brief map a GET for files found in docroot
	void map_get_file(std::string mountPoint)
	{
		map(std::move(mountPoint), "GET", &html_controller::handle_file);
	}

	// --------------------------------------------------------------------
	/// assign a default handler function to a path in the server's namespace
	/// Usually called like this:
	/// \code{.cpp}
	///   map("page", "page.html");
	/// \endcode
	/// Or even more simple:
	/// \code{.cpp}
	///   map("page", "page");
	/// \endcode
	/// Where page is the name of a template file.
	///
	/// Note, the first parameter is a glob pattern, similar to Ant matching rules. Similar to the previous map calls.

	/// @cond
	struct html_mount_point_simple : public mount_point_base
	{
		html_mount_point_simple(std::string path, std::string method, std::string templateName, html_controller &controller)
			: mount_point_base(std::move(path), std::move(method))
			, m_template(std::move(templateName))
			, m_controller(controller)
		{
		}

		reply call(const scope &scope) override;

		std::string m_template;
		html_controller &m_controller;
	};
	/// @endcond

	/// \brief map a simple page to a URI.
	void map_get_simple(std::string mountPoint, std::string templateName)
	{
		m_mountpoints.emplace_back(new html_mount_point_simple(std::move(mountPoint), "GET", std::move(templateName), *this));
	}

	void map_post_simple(std::string mountPoint, std::string templateName)
	{
		m_mountpoints.emplace_back(new html_mount_point_simple(std::move(mountPoint), "POST", std::move(templateName), *this));
	}

	void map_simple(const std::string &mountPoint, const std::string &templateName)
	{
		map_get_simple(mountPoint, templateName);
		map_post_simple(mountPoint, templateName);
	}

	// --------------------------------------------------------------------

  protected:
	/// \brief Initialize the scope object
	///
	/// Initialize scope, derived classes should call this first
	void init_scope(scope & /*scope*/) override;
};

// --------------------------------------------------------------------
// legacy html_controller support

class html_controller_v1 : public html_controller
{
  public:
	[[deprecated("This old controller will be removed in the next release")]] html_controller_v1(const std::string &prefix_path = "/")
		: html_controller(prefix_path)
	{
	}

	/// \brief Dispatch and handle the request
	bool handle_request(request &req, reply &reply_) override;

  public:
	/// \brief html_controller works with 'handlers' that are methods 'mounted' on a path in the requested URI

	using handler_type = std::function<void(const request &request_, const scope &scope_, reply &reply_)>;

	/// assign a handler function to a path in the server's namespace
	/// Usually called like this:
	/// \code{.cpp}
	///   mount(std::move(")page", std::bind(&page_handler, this, _1, _2, _3));
	/// \endcode
	/// Where page_handler is defined as:
	/// \code{.cpp}
	/// void session_server::page_handler(const request& request, const scope& scope, reply& reply);
	/// \endcode
	/// Note, the first parameter is a glob pattern, similar to Ant matching rules.
	/// Supported operators are \*, \*\* and ?. As an addition curly bracketed optional objects are allowed
	/// as well as semi-colons that define separate paths.
	/// Also, patterns ending in / are interpreted as ending in /\*\*
	///
	/// path             | matches
	/// ---------------- | --------------------------------------------
	/// **/*.js          | matches x.js, a/b/c.js, etc
	/// {css,scripts}/   | matches e.g. css/1/first.css and scripts/index.js
	/// a;b;c            | matches either a, b or c

	/// \brief mount a callback on URI path \a path for any HTTP method
	template <class Class>
	void mount(std::string path, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(std::move(path), "UNDEFINED", [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a callback on URI path \a path for HTTP GET method
	template <class Class>
	void mount_get(std::string path, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(std::move(path), "GET", [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a callback on URI path \a path for HTTP POST method
	template <class Class>
	void mount_post(std::string path, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(std::move(path), "POST", [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method \a method
	template <class Class>
	void mount(std::string path, std::string method, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(std::move(path), std::move(method), [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a handler on URI path \a path for HTTP method \a method
	void mount(std::string path, std::string method, const handler_type &handler)
	{
		auto mpi = std::ranges::find_if(m_dispatch_table,
			[&path, &method](auto &mp)
			{
				return mp.path == path and (mp.method == method or mp.method == "UNDEFINED" or method == "UNDEFINED");
			});

		if (mpi == m_dispatch_table.end())
			m_dispatch_table.emplace_back(std::move(path), std::move(method), handler);
		else
		{
			if (mpi->method != method)
				throw logic_exception("cannot mix method UNDEFINED with something else");

			mpi->handler = handler;
		}
	}

  private:
	/// @cond
	struct mount_point_v1
	{
		mount_point_v1(std::string path, std::string method, handler_type handler)
			: path(std::move(path))
			, method(std::move(method))
			, handler(std::move(handler))
		{
		}

		std::string path;
		std::string method;
		handler_type handler;
	};

	using mount_point_list = std::vector<mount_point_v1>;

	mount_point_list m_dispatch_table;
	/// @endcond
};

} // namespace zeep::http
