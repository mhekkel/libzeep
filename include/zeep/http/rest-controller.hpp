// Copyright Maarten L. Hekkelman, Radboud University 2008-2019.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <boost/filesystem/path.hpp>

#include <utility>
#include <experimental/tuple>

#include <zeep/http/controller.hpp>
#include <zeep/el/parser.hpp>

namespace zeep
{

namespace http
{

class rest_controller : public controller
{
  public:
	rest_controller(const std::string& prefixPath)
		: controller(prefixPath)
	{
	}
    ~rest_controller();

	virtual bool handle_request(const request& req, reply& rep);

  protected:

	struct mount_point_base
	{
		mount_point_base(const char* path, method_type method)
			: m_path(path), m_method(method) {}

        virtual ~mount_point_base() {}

		virtual void call(const request& req, reply& reply) = 0;

		std::string m_path;
		method_type m_method;
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
		using ArgsTuple = std::tuple<typename std::remove_const<typename std::remove_reference<Args>::type>::type...>;
		using Callback = std::function<Result(Args...)>;
        using json = el::element;

		static constexpr size_t N = sizeof...(Args);

		mount_point(const char* path, method_type method, rest_controller* owner, Sig sig)
			: mount_point_base(path, method)
		{
			ControllerType* controller = dynamic_cast<ControllerType*>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](Args... args) {
				return (controller->*sig)(args...);
			};
		}

		template<typename... Names>
		mount_point(const char* path, method_type method, rest_controller* owner, Sig sig, Names... names)
			: mount_point(path, method, owner, sig)
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");
			
			// for (auto name: {...names })
			size_t i = 0;
			for (auto name: {names...})
				m_names[i++] = name;

			// construct a regex for matching paths
			namespace fs = boost::filesystem;

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

		virtual void call(const request& req, reply& reply)
		{
			try
			{
				json message("ok");
				reply.set_content(message);
				reply.set_status(ok);

				ArgsTuple args = collect_arguments(req, std::make_index_sequence<N>());
				invoke<Result>(std::move(args), reply);
			}
			catch (const std::exception& e)
			{
				json message({"error", e.what()});

				reply.set_content(message);
				reply.set_status(internal_server_error);
			}
		}

		template<typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_void<ResultType>::value, int> = 0>
		void invoke(ArgsTuple&& args, reply& reply)
		{
			std::experimental::apply(m_callback, std::forward<ArgsTuple>(args));
		}

		template<typename ResultType, typename ArgsTuple, std::enable_if_t<not std::is_void<ResultType>::value, int> = 0>
		void invoke(ArgsTuple&& args, reply& reply)
		{
			set_reply(reply, std::experimental::apply(m_callback, std::forward<ArgsTuple>(args)));
		}

		template<typename T, std::enable_if_t<
			zeep::has_serialize<T, zeep::serializer<json>>::value or
			zeep::is_serializable_array_type<T, zeep::serializer<json>>::value, int> = 0>
		void set_reply(reply& rep, T&& v)
		{
			json j_result;

			zeep::to_element(j_result, v);

			rep.set_content(j_result);
		}

		template<typename T, std::enable_if_t<
			not (zeep::has_serialize<T, zeep::serializer<json>>::value or
				zeep::is_serializable_array_type<T, zeep::serializer<json>>::value), int> = 0>
		void set_reply(reply& rep, T&& v)
		{
			rep.set_content(boost::lexical_cast<std::string>(std::forward<T>(v)), "text/plain");
		}

		template<std::size_t... I>
		ArgsTuple collect_arguments(const request& req, std::index_sequence<I...>)
		{
			// return std::make_tuple(req.get_parameter(m_names[I])...);
			return std::make_tuple(get_parameter<typename std::tuple_element<I, ArgsTuple>::type>(req, m_names[I])...);
		}

		template<typename T, std::enable_if_t<
			not (zeep::has_serialize<T, zeep::deserializer<json>>::value or std::is_enum<T>::value), int> = 0>
		T get_parameter(const request& req, const char* name)
		{
			return boost::lexical_cast<T>(req.get_parameter(name));
		}

		template<typename T, std::enable_if_t<zeep::el::detail::has_from_element<T>::value and std::is_enum<T>::value, int> = 0>
		T get_parameter(const request& req, const char* name)
		{
			json v = req.get_parameter(name);

			T tv;
			from_element(v, tv);
			return tv;
		}

		template<typename T, std::enable_if_t<zeep::has_serialize<T, zeep::deserializer<json>>::value, int> = 0>
		T get_parameter(const request& req, const char* name)
		{
			json v;

			// sigh... is this correct?
			if (req.get_header("content-type") == "application/json")
				zeep::el::parse_json(req.payload, v);
			else
				zeep::el::parse_json(req.get_parameter(name), v);

			T tv;
			from_element(v, tv);
			return tv;
		}

		Callback m_callback;
		std::array<const char*, N>	m_names;
	};

	template<typename Callback, typename... ArgNames>
	void map_request(const char* mountPoint, method_type method, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(mountPoint, method, this, callback, names...));
	}

	template<typename Callback, typename... ArgNames>
	void map_post_request(const char* mountPoint, Callback callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::POST, callback, names...);
	}

	template<typename Sig, typename... ArgNames>
	void map_put_request(const char* mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::PUT, callback, names...);
	}

	template<typename Sig, typename... ArgNames>
	void map_get_request(const char* mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::GET, callback, names...);
	}

	template<typename Sig, typename... ArgNames>
	void map_delete_request(const char* mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, method_type::DELETE, callback, names...);
	}

	std::list<mount_point_base*> m_mountpoints;
};


} // namespace http

} // namespace zeep
