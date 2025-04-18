//       Copyright Maarten L. Hekkelman, 2019-2025
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the serializer classes that help serialize data into and out of our el script objects

#include <zeep/config.hpp>

#include <zeep/el/object.hpp>
#include <zeep/nvp.hpp>
#include <zeep/type-traits.hpp>
#include <zeep/value-serializer.hpp>

// --------------------------------------------------------------------

namespace zeep::el
{

template <typename T, typename = void>
struct serializer;

template <typename T, typename = void>
struct deserializer;

struct object_serializer;
struct object_deserializer;

// --------------------------------------------------------------------

template <typename T, typename Archive>
using serialize_function = decltype(std::declval<T &>().serialize(std::declval<Archive &>(), std::declval<unsigned long>()));

template <typename T, typename Archive, typename = void>
struct has_serialize : std::false_type
{
};

template <typename T, typename Archive>
struct has_serialize<T, Archive, typename std::enable_if_t<std::is_class_v<T>>>
{
	static constexpr bool value = std::experimental::is_detected_v<serialize_function, T, Archive>;
};

template <typename T, typename S>
inline constexpr bool has_serialize_v = has_serialize<T, S>::value;

// --------------------------------------------------------------------

template <typename E, typename T, typename = void>
struct is_compatible_string_type : std::false_type
{
};

template <typename E, typename T>
struct is_compatible_string_type<E, T,
	std::enable_if_t<
		std::experimental::is_detected_exact_v<typename E::string_type::value_type, value_type_t, T>>>
{
	static constexpr bool value =
		std::is_constructible_v<typename E::string_type, T>;
};

template <typename E, typename T>
inline constexpr bool is_compatible_string_type_v = is_compatible_string_type<E, T>::value;

// --------------------------------------------------------------------

template <typename T, typename = void>
struct is_serializable_array_type : std::false_type
{
};

template <typename T>
struct is_serializable_array_type<T,
	std::enable_if_t<
		not std::experimental::is_detected_v<mapped_type_t, T> and
		not std::experimental::is_detected_v<key_type_t, T> and
		std::experimental::is_detected_v<value_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		not std::experimental::is_detected_v<std_string_npos_t, T>>>
{
	static constexpr bool value = std::is_constructible_v<object, typename T::mapped_type> or
	                              has_serialize_v<typename T::mapped_type, object_serializer>;
};

template <typename T>
inline constexpr bool is_serializable_array_type_v = is_serializable_array_type<T>::value;

// --------------------------------------------------------------------

template <typename T, typename = void>
struct is_serializable_map_type : std::false_type
{
};

template <typename T>
struct is_serializable_map_type<T,
	std::enable_if_t<
		std::experimental::is_detected_v<mapped_type_t, T> and
		std::experimental::is_detected_v<key_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		not is_compatible_string_type_v<object, T>>>
{
	static constexpr bool value =
		std::is_same_v<typename T::key_type, std::string> and
		(std::is_constructible_v<object, typename T::mapped_type> or
			has_serialize_v<typename T::mapped_type, object_serializer>);
};

template <typename T>
inline constexpr bool is_serializable_map_type_v = is_serializable_map_type<T>::value;

// --------------------------------------------------------------------

struct object_serializer
{
	object_serializer() {}

	template <typename T>
	object_serializer &operator&(name_value_pair<T> &&nvp)
	{
		serialize(nvp.name(), nvp.value());
		return *this;
	}

	template <typename T>
	void serialize(const char *name, const T &data)
	{
		using serializer_impl = serializer<T>;

		m_elem.emplace(name, serializer_impl::serialize(data));
	}

	template <typename T>
	static void serialize(object &o, const T &v)
	{
		using serializer_impl = serializer<T>;

		o = serializer_impl::serialize(v);
	}

	object m_elem;
};

struct object_deserializer
{
	object_deserializer(const object &o)
		: m_elem(o)
	{
	}

	template <typename T>
	object_deserializer &operator&(name_value_pair<T> &&nvp)
	{
		deserialize(nvp.name(), nvp.value());
		return *this;
	}

	template <typename T>
	void deserialize(const char *name, T &data)
	{
		if (not m_elem.is_object() or m_elem.empty())
			return;

		using serializer_impl = serializer<T>;

		auto value = m_elem[name];

		if (value.is_null())
			return;

		data = serializer_impl::deserialize(value);
	}

	const object &m_elem;
};

// --------------------------------------------------------------------

template <typename T>
	requires(
		//
		std::is_constructible_v<object, T> and
		not std::is_same_v<T, object> and
		not is_serializable_map_type_v<T> and
		not is_serializable_array_type_v<T>
		//
		)
struct serializer<T>
{
	static object serialize(const T &v)
	{
		return object{ v };
	}

	static object serialize(T &&v)
	{
		return object{ std::forward<T>(v) };
	}

	static T deserialize(const object &o)
	{
		return o.get<T>();
	}
};

template <typename T>
	requires has_serialize_v<T, object_serializer>
struct serializer<T>
{
	static object serialize(const T &v)
	{
		object_serializer s;
		const_cast<T &>(v).serialize(s, 0);
		return s.m_elem;
	}

	static object serialize(T &&v)
	{
		object_serializer s;
		const_cast<T &>(v).serialize(s, 0);
		return s.m_elem;
	}

	static T deserialize(const object &o)
	{
		object_deserializer s(o);
		T result{};
		const_cast<T &>(result).serialize(s, 0);
		return result;
	}
};

template <typename T>
	requires is_serializable_map_type_v<T>
struct serializer<T>
{
	static object serialize(const T &v)
	{
		using value_serializer_impl = serializer<typename T::mapped_type>;

		object e = object::value_type::object;

		for (auto &i : v)
			e.emplace(i.first, value_serializer_impl::serialize(i.second));

		return e;
	}

	static T deserialize(const object &o)
	{
		using value_deserializer_impl = serializer<typename T::mapped_type>;

		T result{};

		for (auto i = o.begin(); i != o.end(); ++i)
			result[i.key()] = value_deserializer_impl::deserialize(i.value());

		return result;
	}
};

template <typename T>
	requires is_serializable_array_type_v<T>
struct serializer<T>
{
	static object serialize(const T &v)
	{
		using value_serializer_impl = serializer<typename T::mapped_type>;

		object o = object::value_type::array;

		for (auto &i : v)
			o.push_back(value_serializer_impl::serialize(v));

		return o;
	}

	static T deserialize(const object &o)
	{
		using value_deserializer_impl = serializer<typename T::mapped_type>;

		T result{};

		for (auto &i : o)
			result.emplace_back(value_deserializer_impl::deserialize(i));

		return result;
	}
};

// // --------------------------------------------------------------------

// template<typename T>
// struct serializer
// {
// 	template <typename T, typename = void>
// 	struct serializer_impl
// 	{
// 	};

// 	template <typename T>
// 		requires has_serialize_v<T, serializer>
// 	struct serializer_impl<T>
// 	{
// 		static void serialize(const T &data, object &e)
// 		{
// 			serializer sr;
// 			const_cast<T &>(data).serialize(sr, 0);
// 			std::swap(e, sr.m_elem);
// 		}
// 	};

// 	template <typename T>
// 		requires (not has_serialize_v<T> and has_value_serializer_v<T>)
// 	struct serializer_impl<T>
// 	{
// 		using value_serializer = zeep::value_serializer<T>;

// 		static void serialize(const T &data, object &e)
// 		{
// 			e = value_serializer::to_string(data);
// 		}
// 	};

// 	// template <typename T>
// 	// 	requires is_
// 	// struct serializer_impl<T, std::enable_if_t<
// 	// 							  is_compatible_type_v<T> and
// 	// 							  not is_serializable_array_type_v<T, serializer> and
// 	// 							  not is_serializable_map_type_v<T, serializer>>>
// 	// {
// 	// 	static void serialize(const T &data, object &e)
// 	// 	{
// 	// 		e = data;
// 	// 	}
// 	// };

// 	template <typename T>
// 		requires is_serializable_array_type_v<T, serializer>
// 	struct serializer_impl<T>
// 	{
// 		static void serialize(const T &data, object &e)
// 		{
// 			e = object::value_type::array;

// 			for (auto &i : data)
// 			{
// 				object ei;
// 				zeep::el::serializer<typename T::value_type>::serialize(i, ei);
// 				e.push_back(ei);
// 			}
// 		}
// 	};

// 	template <typename T>
// 		requires is_serializable_map_type_v<T, serializer>
// 	struct serializer_impl<T>
// 	{
// 		static void serialize(const T &data, object &e)
// 		{
// 			e = object::value_type::object;

// 			for (auto &i : data)
// 			{
// 				object ei;
// 				zeep::el::serializer<object>::serialize(i.second, ei);
// 				e.emplace(i.first, ei);
// 			}
// 		}
// 	};

// 	template <typename T>
// 		requires is_serializable_optional_type_v<T, serializer>
// 	struct serializer_impl<T>
// 	{
// 		static void serialize(const T &data, object &e)
// 		{
// 			using value_serializer_impl = serializer_impl<typename T::value_type>;

// 			if (data)
// 				value_serializer_impl::serialize(*data, e);
// 			else
// 				e = {};
// 		}
// 	};

// 	serializer() {}

// 	template <typename T>
// 	serializer &operator&(name_value_pair<T> &&nvp)
// 	{
// 		serialize(nvp.name(), nvp.value());
// 		return *this;
// 	}

// 	template <typename T>
// 	void serialize(const char *name, const T &data)
// 	{
// 		using value_serializer_impl = serializer_impl<T>;

// 		object e;
// 		value_serializer_impl::serialize(data, e);
// 		m_elem.emplace(std::make_pair(name, e));
// 	}

// 	template <typename T>
// 	static void serialize(E &e, const T &v)
// 	{
// 		using value_serializer_impl = serializer_impl<T>;
// 		value_serializer_impl::serialize(v, e);
// 	}

// 	object m_elem;
// };

// template <typename E>
// struct deserializer
// {
// 	using object_type = E;

// 	template <typename T, typename = void>
// 	struct deserializer_impl
// 	{
// 	};

// 	template <typename T>
// 		requires has_serialize_v<T, serializer>
// 	struct deserializer_impl<T>
// 	{
// 		static void deserialize(T &data, const object &e)
// 		{
// 			deserializer sr(e);
// 			data.serialize(sr, 0);
// 		}
// 	};

// 	template <typename T>
// 		requires (not has_serialize_v<T> and has_value_serializer_v<T>)
// 	struct deserializer_impl<T>
// 	{
// 		using value_serializer = zeep::value_serializer<T>;

// 		static void deserialize(T &data, const object &e)
// 		{
// 			data = value_serializer::from_string(e.template get<std::string>());
// 		}
// 	};

// 	// template <typename T>
// 	// struct deserializer_impl<T, std::enable_if_t<
// 	// 								is_compatible_type_v<T> and
// 	// 								not is_serializable_array_type_v<T, deserializer> and
// 	// 								not is_serializable_map_type_v<T, deserializer>>>
// 	// {
// 	// 	static void deserialize(T &data, const object &e)
// 	// 	{
// 	// 		data = e.template get<T>();
// 	// 	}
// 	// };

// 	template <typename T>
// 		requires is_serializable_array_type_v<T, serializer>
// 	struct deserializer_impl<T>
// 	{
// 		static void deserialize(T &data, const object &e)
// 		{
// 			using value_deserializer_impl = deserializer_impl<typename T::value_type>;

// 			data.clear();

// 			for (auto &i : e)
// 			{
// 				typename T::value_type v;

// 				value_deserializer_impl::deserialize(v, i);

// 				data.push_back(v);
// 			}
// 		}
// 	};

// 	template <typename T>
// 		requires is_serializable_map_type_v<T, serializer>
// 	struct deserializer_impl<T>
// 	{
// 		static void deserialize(T &data, const object &e)
// 		{
// 			data.clear();

// 			for (auto i = e.begin(); i != e.end(); ++i)
// 			{
// 				typename T::mapped_type v;

// 				zeep::el::deserializer<object>::deserialize(v, i.value());

// 				data[i.key()] = v;
// 			}
// 		}
// 	};

// 	template <typename T>
// 		requires is_serializable_optional_type_v<T, serializer>
// 	struct deserializer_impl<T>
// 	{
// 		static void deserialize(T &data, const object &e)
// 		{
// 			typename T::value_type v;
// 			zeep::el::deserializer<object>::deserialize(v, e);
// 			data.emplace(std::move(v));
// 		}
// 	};

// 	deserializer(const object &elem)
// 		: m_elem(elem)
// 	{
// 	}

// 	template <typename T>
// 	deserializer &operator&(name_value_pair<T> nvp)
// 	{
// 		deserialize(nvp.name(), nvp.value());
// 		return *this;
// 	}

// 	template <typename T>
// 	void deserialize(const char *name, T &data)
// 	{
// 		if (not m_elem.is_object() or m_elem.empty())
// 			return;

// 		using value_deserializer_impl = deserializer_impl<T>;

// 		auto value = m_elem[name];

// 		if (value.is_null())
// 			return;

// 		value_deserializer_impl::deserialize(data, value);
// 	}

// 	template <typename T>
// 	static void deserialize(const E &e, T &v)
// 	{
// 		using value_deserializer_impl = deserializer_impl<T>;
// 		value_deserializer_impl::deserialize(v, e);
// 	}

// 	const object &m_elem;
// };

// // template<typename J, typename T>
// // void to_object(J& e, T& v)
// // {
// // 	serializer<J>::serialize(e, v);
// // }

// // template<typename J, typename T>
// // void from_object(const J& e, T& v)
// // {
// // 	deserializer<typename std::remove_cv_t<J>>::deserialize(e, v);
// // }

// // namespace detail
// // {

// // struct to_object_fn
// // {
// // 	template<typename T>
// // 	auto operator()(el::object& j, T&& val) const noexcept(noexcept(to_object(j, std::forward<T>(val))))
// // 	-> decltype(to_object(j, std::forward<T>(val)), void())
// // 	{
// // 		return to_object(j, std::forward<T>(val));
// // 	}
// // };

// // namespace
// // {
// // 	constexpr const auto& to_object = typename ::zeep::el::to_object_fn{};
// // }

// // }

// // template<typename,typename = void>
// // struct object_serializer
// // {
// // 	template<typename T>
// // 	static auto to_object(object& j, T&& v)
// // 		noexcept(noexcept(::zeep::el::to_object(j, std::forward<T>(v))))
// // 		-> decltype(::zeep::el::to_object(j, std::forward<T>(v)))
// // 	{
// // 		::zeep::el::to_object(j, std::forward<T>(v));
// // 	}

// // 	template<typename T>
// // 	static auto from_object(const object& j, T& v)
// // 		noexcept(noexcept(::zeep::el::from_object(j, v)))
// // 		-> decltype(::zeep::el::from_object(j, v))
// // 	{
// // 		::zeep::el::from_object(j, v);
// // 	}
// // };

// // // template<typename T>
// // // struct object_serializer<T, std::enable_if_t<std::is_enum_v<T>>>
// // // {
// // // 	static void to_object(object& j, T v)
// // // 	{
// // // 		j = zeep::value_serializer<T>::instance().to_string(v);
// // // 	}

// // // 	template<typename J>
// // // 	static void from_object(const J& j, T& v)
// // // 	{
// // // 		v = zeep::value_serializer<T>::instance().from_string(j.template get<std::string>());
// // // 	}
// // // };

} // namespace zeep::el
