//        Copyright Maarten L. Hekkelman, 2014-2025
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the base class zeep::http::controller, used by e.g. controller and soap_controller

#include <zeep/config.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/scope.hpp>

#include <fstream>

namespace zeep::http
{

/// \brief A base class for controllers, classes that handle a request
///
/// This concept is inspired by the Spring way of delegating the work to
/// controller classes. In libzeep there are two major implementations of
/// controllers: zeep::http::controller and zeep::http::soap_controller
///
/// There can be multiple controllers in a web application, each is connected
/// to a certain prefix-path. This is the leading part of the request URI.

class controller
{
  public:
	/// \brief constructor
	///
	/// \param prefix_path  The prefix path this controller is bound to

	controller(std::string prefix_path);

	virtual ~controller();

	/// \brief Calls handle_request but stores a pointer to the request first
	virtual bool dispatch_request(asio_ns::ip::tcp::socket &socket, request &req, reply &rep);

	/// \brief The virtual method that actually handles the request
	virtual bool handle_request(request &req, reply &rep);

	/// \brief returns the defined prefix path
	uri get_prefix() const { return m_prefix_path; }

	/// \brief return whether this uri request path matches our prefix
	bool path_matches_prefix(const uri &path) const;

	/// \brief return the path with the prefix path stripped off
	uri get_prefixless_path(const request &req) const;

	/// \brief bind this controller to \a server
	virtual void set_server(basic_server *server)
	{
		m_server = server;
	}

	/// \brief return the server object we're bound to
	const basic_server *get_server() const { return m_server; }
	basic_server *get_server() { return m_server; }

	/// \brief return the context name, if specified. Empty string otherwise
	std::string get_context_name() const
	{
		return m_server ? m_server->get_context_name() : "";
	}

	/// \brief Fill in the OPTIONS in reply \a rep for a request \a req
	virtual void get_options(const request &req, reply &rep);

  protected:
	controller(const controller &) = delete;
	controller &operator=(const controller &) = delete;

	uri m_prefix_path;
	basic_server *m_server = nullptr;

	/// @cond

	/// \brief abstract base class for mount points, derived classes should
	/// derive from mount_point instead of this class
	struct mount_point_base
	{
		mount_point_base(std::string path, std::string method)
			: m_path(std::move(path))
			, m_method(std::move(method))
		{
		}

		virtual ~mount_point_base() {}

		virtual reply call(const scope &scope) = 0;

		template <typename... Names>
		void set_names(Names... names)
		{
			if constexpr (sizeof...(Names) > 0)
			{
				std::filesystem::path p = m_path;

				for (auto name : { names... })
					m_names.emplace_back(name);

				// construct a regex for matching paths
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
						ps += "([^/]*)";
					}
					else
						ps += pp.string();
				}

				m_rx.assign(ps);
			}
		}

		bool get_parameter(const scope &scope, const std::string &name, bool result)
		{
			try
			{
				auto v = scope.get_parameter(name).value_or("false");
				result = v == "true" or v == "1" or v == "on";
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		std::string get_parameter(const scope &scope, const std::string &name, std::string result)
		{
			try
			{
				result = scope.get_parameter(name).value_or("");
			}
			catch (const std::exception &)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		file_param get_parameter(const scope &scope, const std::string &name, file_param result)
		{
			try
			{
				result = scope.get_file_parameter(name);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		std::vector<file_param> get_parameter(const scope &scope, const std::string &name, std::vector<file_param> result)
		{
			try
			{
				result = scope.get_file_parameters(name);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		object get_parameter(const scope &scope, const std::string &name, object result)
		{
			try
			{
				auto p = scope.get_parameter(name);
				if (p.has_value())
					result = object::parse_JSON(*p);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template <typename T>
		std::optional<T> get_parameter(const scope &scope, const std::string &name, std::optional<T> result)
		{
			try
			{
				auto v = scope.get_parameter(name);
				if (v.has_value())
					result = value_serializer<T>::from_string(*v);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		std::optional<std::string> get_parameter(const scope &scope, const std::string &name, std::optional<std::string> result)
		{
			try
			{
				result = scope.get_parameter(name);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template <typename T>
			requires el::has_value_serializer_v<T>
		T get_parameter(const scope &scope, const std::string &name, T result)
		{
			try
			{
				auto p = scope.get_parameter(name);
				if (p.has_value())
					result = value_serializer<T>::from_string(*p);
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template <typename T>
			requires mxml::has_serialize_v<T, el::deserializer<object>> or
		             mxml::is_serializable_array_type_v<T, el::deserializer<object>>
		T get_parameter(const scope &scope, const std::string &name, T result)
		{
			object v;

			if (scope.get_header("content-type") == "application/json")
				v = object::parse_JSON(scope.get_payload());
			else
			{
				auto p = scope.get_parameter(name);
				if (p.has_value())
					v = object::parse_JSON(*p);
			}

			return el::serializer<T>::deserialize(v);
		}

		std::string m_path;
		std::string m_method;
		std::regex m_rx;
		std::vector<std::string> m_path_params;
		std::vector<std::string> m_names;
	};

	template <typename Callback, typename...>
	struct mount_point
	{
	};

	/// \brief templated abstract base class for mount points
	template <typename ControllerType, typename Result, typename... Args>
	struct mount_point<Result (ControllerType::*)(Args...)> : mount_point_base
	{
		using Sig = Result (ControllerType::*)(Args...);
		using ArgsTuple = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<Args>>...>;
		using ResultType = typename std::remove_const_t<typename std::remove_reference_t<Result>>;
		using Callback = std::function<ResultType(Args...)>;

		static constexpr size_t N = sizeof...(Args);

		template <typename... Names>
		mount_point(std::string path, std::string method, controller *owner, Sig sig, Names... names)
			: mount_point_base(std::move(path), std::move(method))
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");

			ControllerType *controller = dynamic_cast<ControllerType *>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](Args... args)
			{
				return (controller->*sig)(args...);
			};

			set_names(names...);
		}

		reply call(const scope &scope) override
		{
			ArgsTuple args = collect_arguments(scope, std::make_index_sequence<N>());
			return invoke<Result>(std::move(args));
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_void_v<ResultType>, int> = 0>
		reply invoke(ArgsTuple &&args)
		{
			std::apply(m_callback, std::forward<ArgsTuple>(args));
			return reply::stock_reply(ok);
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<not(std::is_void_v<ResultType> or std::is_same_v<ResultType, reply>), int> = 0>
		reply invoke(ArgsTuple &&args)
		{
			return set_reply(std::apply(m_callback, std::forward<ArgsTuple>(args)));
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_same_v<ResultType, reply>, int> = 0>
		reply invoke(ArgsTuple &&args)
		{
			return std::apply(m_callback, std::forward<ArgsTuple>(args));
		}

		reply set_reply(std::filesystem::path v)
		{
			reply rep(ok);
			rep.set_content(new std::ifstream(v, std::ios::binary), "application/octet-stream");
			return rep;
		}

		reply set_reply(object &&v)
		{
			reply rep(ok);
			rep.set_content(std::move(v));
			return rep;
		}

		template <typename T>
		reply set_reply(T &&v)
		{
			reply rep(ok);
			rep.set_content(el::serializer<T>::serialize(std::forward<T>(v)));
			return rep;
		}

		template <std::size_t... I>
		ArgsTuple collect_arguments(const scope &scope, std::index_sequence<I...>)
		{
			// return std::make_tuple(params.get_parameter(m_names[I])...);
			return std::make_tuple(get_parameter(scope, m_names[I], typename std::tuple_element_t<I, ArgsTuple>{})...);
		}

		Callback m_callback;
	};

	/// @endcond

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
	/// map_get_request("/cart/{id}/status", &my_ajax_handler::handle_get_status, "id");
	/// \endcode
	///
	/// The number of \a names of the paramers specified should be equal to the number of
	/// actual arguments for the callback, otherwise the compiler will complain.
	///
	/// Arguments not found in the path will be fetched from the payload in case of a POST
	/// or from the URI parameters otherwise.

	/// \brief map \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Callback, typename... ArgNames>
	void map_request(std::string mountPoint, std::string method, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(std::move(mountPoint), std::move(method), this, callback, names...));
	}

	/// \brief map a POST to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Callback, typename... ArgNames>
	void map_post_request(std::string mountPoint, Callback callback, ArgNames... names)
	{
		map_request(std::move(mountPoint), "POST", callback, names...);
	}

	/// \brief map a PUT to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_put_request(std::string mountPoint, Sig callback, ArgNames... names)
	{
		map_request(std::move(mountPoint), "PUT", callback, names...);
	}

	/// \brief map a GET to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_get_request(std::string mountPoint, Sig callback, ArgNames... names)
	{
		map_request(std::move(mountPoint), "GET", callback, names...);
	}

	/// \brief map a DELETE to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_delete_request(std::string mountPoint, Sig callback, ArgNames... names)
	{
		map_request(std::move(mountPoint), "DELETE", callback, names...);
	}

	// --------------------------------------------------------------------

	virtual reply call_mount_point(mount_point_base *mp, const scope &scope);
	virtual void init_scope(scope &scope);

  protected:
	std::list<mount_point_base *> m_mountpoints;
};

} // namespace zeep::http
