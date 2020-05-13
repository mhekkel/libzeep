// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::rest_controller class.
/// Instances of this class take care of mapping member functions to
/// REST calls automatically converting in- and output data

#include <zeep/config.hpp>

#include <filesystem>
#include <fstream>
#include <utility>
#include <experimental/tuple>

#include <zeep/http/controller.hpp>
#include <zeep/http/authorization.hpp>
#include <zeep/el/parser.hpp>

namespace zeep::http
{

/// \brief class that helps with handling REST requests
///
/// This controller will handle REST requests. (See https://restfulapi.net/ for more info on REST)
///
/// To use this, create a subclass and add some methods that should be exposed.
/// Then _map_ these methods on a path that optionally contains parameter values.
///
/// See the chapter on REST controllers in the documention for more information.

class rest_controller : public controller
{
  public:

	/// \brief constructor
	///
	/// \param prefix_path	This is the leading part of the request URI for each mount point
	/// \param auth			Optionally protect these REST calls with a authentication validator.
	///						This validator should also be added to the web_app that will contain
	///						this controller.
	rest_controller(const std::string& prefix_path, authentication_validation_base* auth = nullptr)
		: controller(prefix_path), m_auth(auth)
	{
	}

    ~rest_controller();

	/// \brief will do the hard work
	virtual bool handle_request(request& req, reply& rep);

  protected:

	/// \brief validate \a req in combination with \a realm and create a JSON error message in \a rep in case of failure
	virtual bool validate_request(request& req, reply& rep, const std::string& realm);

	/// \brief Return the credentials for the current call, is valid only when inside a `handle_request`
	virtual el::element& get_credentials()
	{
		return s_credentials;
	}

	using param = header;

	/// \brief helper class for pulling parameter values out of the request
	struct parameter_pack
	{
		parameter_pack(const request& req) : m_req(req) {}

		std::string get_parameter(const char* name) const
		{
			auto p = std::find_if(m_path_parameters.begin(), m_path_parameters.end(),
				[name](auto& pp) { return pp.name == name; });
			if (p != m_path_parameters.end())
				return p->value;
			else
				return m_req.get_parameter(name);
		}

		file_param get_file_parameter(const char* name) const
		{
			return m_req.get_file_parameter(name);
		}

		const request& m_req;
		std::vector<param> m_path_parameters;
	};

	/// \brief abstract base class for mount points
	struct mount_point_base
	{
		mount_point_base(const char* path, method_type method, const std::string& realm)
			: m_path(path), m_method(method), m_realm(realm) {}

        virtual ~mount_point_base() {}

		virtual void call(const parameter_pack& params, reply& reply) = 0;

		std::string m_path;
		method_type m_method;
		std::string m_realm;
		std::regex m_rx;
		std::vector<std::string> m_path_params;
	};

	template<typename Callback, typename...>
	struct mount_point {};

	template<typename ControllerType, typename Result, typename... Args>
	struct mount_point<Result(ControllerType::*)(Args...)> : mount_point_base
	{
		using Sig = Result(ControllerType::*)(Args...);
		// using ArgsTuple = std::tuple<Args...>;
		using ArgsTuple = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<Args>>...>;
		using Callback = std::function<Result(Args...)>;

		static constexpr size_t N = sizeof...(Args);

		mount_point(const char* path, method_type method, const std::string& realm, rest_controller* owner, Sig sig)
			: mount_point_base(path, method, realm)
		{
			ControllerType* controller = dynamic_cast<ControllerType*>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](Args... args) {
				return (controller->*sig)(args...);
			};
		}

		template<typename... Names>
		mount_point(const char* path, method_type method, const std::string& realm, rest_controller* owner, Sig sig, Names... names)
			: mount_point(path, method, realm, owner, sig)
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");
			
			// for (auto name: {...names })
			size_t i = 0;
			for (auto name: {names...})
				m_names[i++] = name;

			// construct a regex for matching paths
			namespace fs = std::filesystem;

			fs::path p = path;
			std::string ps;

			for (auto pp: p)
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

		virtual void call(const parameter_pack& params, reply& reply)
		{
			try
			{
				el::element message("ok");
				reply.set_content(message);
				reply.set_status(ok);

				ArgsTuple args = collect_arguments(params, std::make_index_sequence<N>());
				invoke<Result>(std::move(args), reply);
			}
			catch (const std::exception& e)
			{
				el::element message;
				message["error"] = e.what();

				reply.set_content(message);
				reply.set_status(internal_server_error);
			}
		}

		template<typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_void_v<ResultType>, int> = 0>
		void invoke(ArgsTuple&& args, reply& reply)
		{
			std::experimental::apply(m_callback, std::forward<ArgsTuple>(args));
		}

		template<typename ResultType, typename ArgsTuple, std::enable_if_t<not std::is_void_v<ResultType>, int> = 0>
		void invoke(ArgsTuple&& args, reply& reply)
		{
			set_reply(reply, std::experimental::apply(m_callback, std::forward<ArgsTuple>(args)));
		}

		void set_reply(reply& rep, std::filesystem::path v)
		{
			rep.set_content(new std::ifstream(v, std::ios::binary), "application/octet-stream");
		}

		template<typename T>
		void set_reply(reply& rep, T&& v)
		{
			el::element e;
			to_element(e, v);
			rep.set_content(e);
		}

		template<std::size_t... I>
		ArgsTuple collect_arguments(const parameter_pack& params, std::index_sequence<I...>)
		{
			// return std::make_tuple(params.get_parameter(m_names[I])...);
			return std::make_tuple(get_parameter<typename std::tuple_element_t<I, ArgsTuple>>(params, m_names[I])...);
		}

		template<typename T, std::enable_if_t<std::is_same_v<T,bool>, int> = 0>
		bool get_parameter(const parameter_pack& params, const char* name)
		{
			bool result = false;

			try
			{
				auto v = params.get_parameter(name);
				result = v == "true" or v == "1" or v == "on";
			}
			catch (const std::exception& e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template<typename T, std::enable_if_t<std::is_same_v<T,file_param>, int> = 0>
		file_param get_parameter(const parameter_pack& params, const char* name)
		{
			file_param result;

			try
			{
				result = params.get_file_parameter(name);
			}
			catch (const std::exception& e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template<typename T, std::enable_if_t<std::is_same_v<T,el::element>, int> = 0>
		el::element get_parameter(const parameter_pack& params, const char* name)
		{
			el::element result;

			try
			{
				el::parse_json(params.get_parameter(name), result);
			}
			catch (const std::exception& e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template<typename T, std::enable_if_t<
			not (zeep::has_serialize_v<T, zeep::el::deserializer<el::element>> or
				 std::is_enum_v<T> or
				 std::is_same_v<T,bool> or
				 std::is_same_v<T,file_param> or
				 std::is_same_v<T,el::element>), int> = 0>
		T get_parameter(const parameter_pack& params, const char* name)
		{
			try
			{
				T v = {};
				auto p = params.get_parameter(name);
				if (not p.empty())
					v = boost::lexical_cast<T>(p);
				return v;
			}
			catch(const std::exception& e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}
		}

		template<typename T, std::enable_if_t<zeep::el::detail::has_from_element_v<T> and std::is_enum_v<T>, int> = 0>
		T get_parameter(const parameter_pack& params, const char* name)
		{
			el::element v = params.get_parameter(name);

			T tv;
			from_element(v, tv);
			return tv;
		}

		template<typename T, std::enable_if_t<zeep::has_serialize_v<T, zeep::el::deserializer<el::element>>, int> = 0>
		T get_parameter(const parameter_pack& params, const char* name)
		{
			el::element v;

			if (params.m_req.get_header("content-type") == "application/json")
				zeep::el::parse_json(params.m_req.payload, v);
			else
				zeep::el::parse_json(params.get_parameter(name), v);

			T tv;
			from_element(v, tv);
			return tv;
		}

		Callback m_callback;
		std::array<const char*, N>	m_names;
	};

	/// The \a mountPoint parameter is the local part of the mount point.
	/// It can contain parameters enclosed in curly brackets.
	///
	/// For example, say we need a REST call to get the status of shoppingcart
	/// where the browser will send:
	/// 
	///		`GET /ajax/cart/1234/status`
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
	template<typename Callback, typename... ArgNames>
	void map_request(const char* mountPoint, method_type method, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(mountPoint, method, "", this, callback, names...));
	}

	/// \brief map \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names This version requires authentication.
	template<typename Callback, typename... ArgNames>
	void map_request(const char* mountPoint, method_type method, const std::string& realm, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(mountPoint, method, realm, this, callback, names...));
	}

	/// \brief map a POST to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template<typename Callback, typename... ArgNames>
	void map_post_request(const char* mountPoint, Callback callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::POST, callback, names...);
	}

	/// \brief map a POST to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names This version requires authentication.
	template<typename Callback, typename... ArgNames>
	void map_post_request(const char* mountPoint, const std::string& realm, Callback callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::POST, realm, callback, names...);
	}

	/// \brief map a PUT to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template<typename Sig, typename... ArgNames>
	void map_put_request(const char* mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::PUT, callback, names...);
	}

	/// \brief map a PUT to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names This version requires authentication.
	template<typename Sig, typename... ArgNames>
	void map_put_request(const char* mountPoint, const std::string& realm, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::PUT, realm, callback, names...);
	}

	/// \brief map a GET to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template<typename Sig, typename... ArgNames>
	void map_get_request(const char* mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::GET, callback, names...);
	}

	/// \brief map a GET to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names This version requires authentication.
	template<typename Sig, typename... ArgNames>
	void map_get_request(const char* mountPoint, const std::string& realm, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::GET, realm, callback, names...);
	}

	/// \brief map a DELETE to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template<typename Sig, typename... ArgNames>
	void map_delete_request(const char* mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::DELETE, callback, names...);
	}

	/// \brief map a DELETE to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names This version requires authentication.
	template<typename Sig, typename... ArgNames>
	void map_delete_request(const char* mountPoint, const std::string& realm, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::DELETE, realm, callback, names...);
	}

  private:

	std::list<mount_point_base*> m_mountpoints;
	authentication_validation_base* m_auth = nullptr;

	// keep track of current user accessing this API
	static thread_local el::element s_credentials;
};

} // namespace zeep::http
