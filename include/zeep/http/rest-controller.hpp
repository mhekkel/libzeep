// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::rest_controller class.
/// Instances of this class take care of mapping member functions to
/// REST calls automatically converting in- and output data

#include <zeep/config.hpp>

#include <zeep/el/serializer.hpp>
#include <zeep/http/controller.hpp>
#include <zeep/streambuf.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <tuple>
#include <utility>

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
	rest_controller(const std::string &prefix_path)
		: controller(prefix_path)
	{
	}

	~rest_controller();

  protected:

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
		mount_point(const char *path, const std::string &method, rest_controller *owner, Sig sig, Names... names)
			: mount_point_base(path, method)
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");

			ControllerType *controller = dynamic_cast<ControllerType *>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](Args... args)
			{
				return (controller->*sig)(args...);
			};

			set_names(path, names...);
		}

		virtual void call(const parameter_pack &params, reply &rep)
		{
			ArgsTuple args = collect_arguments(params, std::make_index_sequence<N>());
			invoke<Result>(std::move(args), rep);
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_void_v<ResultType>, int> = 0>
		void invoke(ArgsTuple &&args, reply & /*reply*/)
		{
			std::apply(m_callback, std::forward<ArgsTuple>(args));
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<not(std::is_void_v<ResultType> or std::is_same_v<ResultType, reply>), int> = 0>
		void invoke(ArgsTuple &&args, reply &rep)
		{
			set_reply(rep, std::apply(m_callback, std::forward<ArgsTuple>(args)));
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_same_v<ResultType, reply>, int> = 0>
		void invoke(ArgsTuple &&args, reply &rep)
		{
			rep = std::apply(m_callback, std::forward<ArgsTuple>(args));
		}

		void set_reply(reply &rep, std::filesystem::path v)
		{
			rep.set_content(new std::ifstream(v, std::ios::binary), "application/octet-stream");
		}

		void set_reply(reply &rep, object &&v)
		{
			rep.set_content(std::move(v));
		}

		template <typename T>
		void set_reply(reply &rep, T &&v)
		{
			rep.set_content(el::serializer<T>::serialize(std::forward<T>(v)));
		}

		template <std::size_t... I>
		ArgsTuple collect_arguments(const parameter_pack &params, std::index_sequence<I...>)
		{
			// return std::make_tuple(params.get_parameter(m_names[I])...);
			return std::make_tuple(get_parameter(params, m_names[I].c_str(), typename std::tuple_element_t<I, ArgsTuple>{})...);
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

		object get_parameter(const parameter_pack &params, const char *name, object result)
		{
			try
			{
				result = object::parse_JSON(params.get_parameter(name));
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

		template <typename T>
			requires has_value_serializer_v<T>
		T get_parameter(const parameter_pack &params, const char *name, T result)
		{
			try
			{
				auto p = params.get_parameter(name);
				if (not p.empty())
				{
					using U = std::remove_cvref_t<T>;

					if constexpr (has_value_serializer_v<U>)
						result = value_serializer<U>::from_string(p);
					else // TODO: remove? Check?
						result = value_serializer<T>::from_string(p);
				}
			}
			catch (const std::exception &e)
			{
				using namespace std::literals::string_literals;
				throw std::runtime_error("Invalid value passed for parameter "s + name);
			}

			return result;
		}

		template <typename T>
			requires zeep::has_serialize_v<T, el::deserializer<object>> or
		             zeep::is_serializable_array_type_v<T, el::deserializer<object>>
		T get_parameter(const parameter_pack &params, const char *name, T result)
		{
			object v = params.m_req.get_header("content-type") == "application/json"
			               ? object::parse_JSON(params.m_req.get_payload())
			               : object::parse_JSON(params.get_parameter(name));

			return el::serializer<T>::deserialize(v);
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
	void map_request(const char *mountPoint, const std::string &method, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(mountPoint, method, this, callback, names...));
	}

	/// \brief map a POST to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Callback, typename... ArgNames>
	void map_post_request(const char *mountPoint, Callback callback, ArgNames... names)
	{
		map_request(mountPoint, "POST", callback, names...);
	}

	/// \brief map a PUT to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_put_request(const char *mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, "PUT", callback, names...);
	}

	/// \brief map a GET to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_get_request(const char *mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, "GET", callback, names...);
	}

	/// \brief map a DELETE to \a mountPoint in URI space to \a callback and map the arguments in this callback to parameters passed with \a names
	template <typename Sig, typename... ArgNames>
	void map_delete_request(const char *mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, "DELETE", callback, names...);
	}
};

} // namespace zeep::http
