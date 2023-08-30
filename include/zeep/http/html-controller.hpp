// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
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
#include <zeep/json/parser.hpp>

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
	const basic_template_processor &get_template_processor() const;

	/// \brief Dispatch and handle the request
	virtual bool handle_request(request &req, reply &reply_);

	/// \brief default file handling
	///
	/// This method will ask the server for the default template processor
	/// to load the actual file. If there is no template processor set,
	/// it will therefore throw an exception.
	virtual void handle_file(const request &request_, const scope &scope_, reply &reply_);

	// --------------------------------------------------------------------

  public:
	/// \brief html_controller works with 'handlers' that are methods 'mounted' on a path in the requested URI

	using handler_type = std::function<void(const request &request_, const scope &scope_, reply &reply_)>;

	/// assign a handler function to a path in the server's namespace
	/// Usually called like this:
	/// \code{.cpp}
	///   mount("page", std::bind(&page_handler, this, _1, _2, _3));
	/// \endcode
	/// Where page_handler is defined as:
	/// \code{.cpp}
	/// void session_server::page_handler(const request& request, const scope& scope, reply& reply);
	/// \endcode
	/// Note, the first parameter is a glob pattern, similar to Ant matching rules.
	/// Supported operators are \*, \*\* and ?. As an addition curly bracketed optional elements are allowed
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
	void mount(const std::string &path, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(path, "UNDEFINED", [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a callback on URI path \a path for HTTP GET method
	template <class Class>
	void mount_get(const std::string &path, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(path, "GET", [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a callback on URI path \a path for HTTP POST method
	template <class Class>
	void mount_post(const std::string &path, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(path, "POST", [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a callback on URI path \a path for HTTP method \a method
	template <class Class>
	void mount(const std::string &path, const std::string &method, void (Class::*callback)(const request &req, const scope &sc, reply &rep))
	{
		static_assert(std::is_base_of_v<html_controller, Class>, "This call can only be used for methods in classes derived from html_controller");
		mount(path, method, [server = static_cast<Class *>(this), callback](const request &req, const scope &sc, reply &rep)
			{ (server->*callback)(req, sc, rep); });
	}

	/// \brief mount a handler on URI path \a path for HTTP method \a method
	void mount(const std::string &path, const std::string &method, handler_type handler)
	{
		auto mpi = std::find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
			[path, method](auto &mp)
			{
				return mp.path == path and (mp.method == method or mp.method == "UNDEFINED" or method == "UNDEFINED");
			});

		if (mpi == m_dispatch_table.end())
			m_dispatch_table.emplace_back(path, method, handler);
		else
		{
			if (mpi->method != method)
				throw std::logic_error("cannot mix method UNDEFINED with something else");

			mpi->handler = handler;
		}
	}

	// --------------------------------------------------------------------
  public:
	using param = header;

	/// @cond
	struct parameter_pack
	{
		parameter_pack(const request &req)
			: m_req(req)
		{
		}

		std::string get_parameter(const char *name) const
		{
			auto p = std::find_if(m_path_parameters.begin(), m_path_parameters.end(),
				[name](auto &pp)
				{ return pp.name == name; });
			if (p != m_path_parameters.end())
				return p->value;
			else
				return m_req.get_parameter(name);
		}

		std::tuple<std::string, bool> get_parameter_ex(const char *name) const
		{
			auto p = std::find_if(m_path_parameters.begin(), m_path_parameters.end(),
				[name](auto &pp)
				{ return pp.name == name; });
			if (p != m_path_parameters.end())
				return { p->value, true };
			else
				return m_req.get_parameter_ex(name);
		}

		std::vector<std::string> get_parameters(const char *name) const
		{
			auto p = std::find_if(m_path_parameters.begin(), m_path_parameters.end(),
				[name](auto &pp)
				{ return pp.name == name; });
			if (p != m_path_parameters.end())
				return { p->value };
			else
			{
				std::vector<std::string> result;

				for (const auto &[p_name, p_value] : m_req.get_parameters())
				{
					if (p_name != name)
						continue;

					result.push_back(p_value);
				}

				return result;
			}
		}

		file_param get_file_parameter(const char *name) const
		{
			return m_req.get_file_parameter(name);
		}

		std::vector<file_param> get_file_parameters(const char *name) const
		{
			return m_req.get_file_parameters(name);
		}

		const request &m_req;
		std::vector<param> m_path_parameters;
	};

	struct mount_point_v2_base
	{
		mount_point_v2_base(const char *path, const std::string &method)
			: m_path(path)
			, m_method(method)
		{
		}

		virtual ~mount_point_v2_base() {}

		virtual void call(const scope &scope_, const parameter_pack &params, reply &rep) = 0;

		std::string m_path;
		std::string m_method;
		std::regex m_rx;
		std::vector<std::string> m_path_params;
	};

	template <typename Callback, typename...>
	struct mount_point_v2
	{
	};

	/// \brief templated base class for mount points
	template <typename ControllerType, typename... Args>
	struct mount_point_v2<reply (ControllerType::*)(const scope &scope_, Args...)> : mount_point_v2_base
	{
		using Sig = reply (ControllerType::*)(const scope &, Args...);
		using ArgsTuple = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<Args>>...>;
		using Callback = std::function<reply(const scope &, Args...)>;

		static constexpr size_t N = sizeof...(Args);

		template <typename... Names>
		mount_point_v2(const char *path, const std::string &method, html_controller *owner, Sig sig, Names... names)
			: mount_point_v2_base(path, method)
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");

			ControllerType *controller = dynamic_cast<ControllerType *>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](const scope &scope_, Args... args)
			{
				return (controller->*sig)(scope_, args...);
			};

			if constexpr (sizeof...(Names) > 0)
			{

				// for (auto name: {...names })
				size_t ix = 0;
				for (auto name : { names... })
					m_names[ix++] = name;

				// construct a regex for matching paths
				namespace fs = std::filesystem;

				fs::path p = path;
				std::string ps;

				for (auto pp : p)
				{
					if (pp.empty())
						continue;

					if (not ps.empty())
						ps += '/';

					if (pp.string().front() == '{' and pp.string().back() == '}')
					{
						auto param = pp.string().substr(1, pp.string().length() - 2);

						auto i = std::find(m_names.begin(), m_names.end(), param);
						if (i == m_names.end())
						{
							assert(false);
							throw std::runtime_error("Invalid path for mount point, a parameter was not found in the list of parameter names");
						}

						size_t ni = i - m_names.begin();
						m_path_params.emplace_back(m_names[ni]);
						ps += "([^/]+)";
					}
					else
						ps += pp.string();
				}

				m_rx.assign(ps);
			}
		}

		virtual void call(const scope &scope_, const parameter_pack &params, reply &rep)
		{
			auto args = collect_arguments(scope_, params, std::make_index_sequence<N>());
			rep = std::apply(m_callback, std::move(args));
		}

		template <std::size_t... I>
		auto collect_arguments(const scope &scope_, const parameter_pack &params, std::index_sequence<I...>)
		{
			// return std::make_tuple(params.get_parameter(m_names[I])...);
			return std::make_tuple(scope_, get_parameter(params, m_names[I], typename std::tuple_element_t<I, ArgsTuple>{})...);
		}

		bool get_parameter(const parameter_pack &params, const char *name, bool result)
		{
			try
			{
				auto v = params.get_parameter(name);
				result = v == "true" or v == "1" or v == "on";
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		std::string get_parameter(const parameter_pack &params, const char *name, std::string result)
		{
			try
			{
				result = params.get_parameter(name);
			}
			catch (const std::exception &)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		file_param get_parameter(const parameter_pack &params, const char *name, file_param result)
		{
			try
			{
				result = params.get_file_parameter(name);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		std::vector<file_param> get_parameter(const parameter_pack &params, const char *name, std::vector<file_param> result)
		{
			try
			{
				result = params.get_file_parameters(name);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		json::element get_parameter(const parameter_pack &params, const char *name, json::element result)
		{
			try
			{
				json::parse_json(params.get_parameter(name), result);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template <typename T>
		std::optional<T> get_parameter(const parameter_pack &params, const char *name, std::optional<T> result)
		{
			try
			{
				const auto &[s, available] = params.get_parameter_ex(name);
				if (available)
					result = value_serializer<T>::from_string(s);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		std::optional<std::string> get_parameter(const parameter_pack &params, const char *name, std::optional<std::string> result)
		{
			try
			{
				const auto &[s, available] = params.get_parameter_ex(name);
				if (available)
					result = s;
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template <typename T, std::enable_if_t<not(
												   zeep::has_serialize_v<T, zeep::json::deserializer<json::element>> or std::is_enum_v<T> or
												   zeep::is_serializable_array_type_v<T, zeep::json::deserializer<json::element>>),
								  int> = 0>
		T get_parameter(const parameter_pack &params, const char *name, T result)
		{
			try
			{
				auto p = params.get_parameter(name);
				if (not p.empty())
					result = value_serializer<T>::from_string(p);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template <typename T, std::enable_if_t<zeep::json::detail::has_from_element_v<T> and std::is_enum_v<T>, int> = 0>
		T get_parameter(const parameter_pack &params, const char *name, T result)
		{
			json::element v = params.get_parameter(name);

			from_element(v, result);
			return result;
		}

		template <typename T, std::enable_if_t<zeep::has_serialize_v<T, zeep::json::deserializer<json::element>> or
												   zeep::is_serializable_array_type_v<T, zeep::json::deserializer<json::element>>,
								  int> = 0>
		T get_parameter(const parameter_pack &params, const char *name, T result)
		{
			json::element v;

			if (params.m_req.get_header("content-type") == "application/json")
				zeep::json::parse_json(params.m_req.get_payload(), v);
			else
				zeep::json::parse_json(params.get_parameter(name), v);

			from_element(v, result);

			return result;
		}

		Callback m_callback;
		std::array<const char *, N> m_names;
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
	/// Supported operators are \*, \*\* and ?. As an addition curly bracketed optional elements are allowed
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
	void map(const char *mountPoint, const std::string &method, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point_v2<Callback>(mountPoint, method, this, callback, names...));
	}

	/// \brief map a POST to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Callback, typename... ArgNames>
	void map_post(const char *mountPoint, Callback callback, ArgNames... names)
	{
		map(mountPoint, "POST", callback, names...);
	}

	/// \brief map a PUT to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_put(const char *mountPoint, Sig callback, ArgNames... names)
	{
		map(mountPoint, "PUT", callback, names...);
	}

	/// \brief map a GET to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_get(const char *mountPoint, Sig callback, ArgNames... names)
	{
		map(mountPoint, "GET", callback, names...);
	}

	/// \brief map a DELETE to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_delete(const char *mountPoint, Sig callback, ArgNames... names)
	{
		map(mountPoint, "DELETE", callback, names...);
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
	struct mount_point_v2_simple : public mount_point_v2_base
	{
		mount_point_v2_simple(const char *path, const std::string &method, const char *templateName, html_controller &controller)
			: mount_point_v2_base(path, method)
			, m_template(templateName)
			, m_controller(controller)
		{
		}

		virtual void call(const scope &scope_, const parameter_pack &params, reply &rep);

		std::string m_template;
		html_controller &m_controller;
	};
	/// @endcond

	/// \brief map a simple page to a URI.
	void map_get(const char *mountPoint, const char *templateName)
	{
		m_mountpoints.emplace_back(new mount_point_v2_simple(mountPoint, "GET", templateName, *this));
	}

	void map_post(const char *mountPoint, const char *templateName)
	{
		m_mountpoints.emplace_back(new mount_point_v2_simple(mountPoint, "POST", templateName, *this));
	}

	void map(const char *mountPoint, const char *templateName)
	{
		map_get(mountPoint, templateName);
		map_post(mountPoint, templateName);
	}

	// --------------------------------------------------------------------

	/// \brief Initialize the scope object
	///
	/// The default implementation does nothing, derived implementations may
	/// want to add some default data to the scope.
	virtual void init_scope(scope & /*scope*/) {}

  private:

	/// @cond
	struct mount_point
	{
		mount_point(const std::string &path, const std::string &method, handler_type handler)
			: path(path)
			, method(method)
			, handler(handler)
		{
		}

		std::string path;
		std::string method;
		handler_type handler;
	};

	using mount_point_list = std::vector<mount_point>;

	mount_point_list m_dispatch_table;
	std::list<mount_point_v2_base *> m_mountpoints;
	/// @endcond
};

} // namespace zeep::http
