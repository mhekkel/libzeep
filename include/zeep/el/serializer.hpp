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
	static constexpr bool value = std::is_constructible_v<object, typename T::value_type> or
	                              has_serialize_v<typename T::value_type, object_serializer>;
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

template<typename T>
using has_value_or_result_t = decltype(std::declval<T>().value_or(std::declval<typename T::value_type&&>()));

template <typename T, typename = void>
struct is_serializable_optional_type : std::false_type
{
};

template <typename T>
struct is_serializable_optional_type<T,
	std::enable_if_t<
		std::experimental::is_detected_v<value_type_t, T> and
		std::is_same_v<has_value_or_result_t<T>, typename T::value_type> and
		not is_compatible_string_type_v<object, T>>>
{
	static constexpr bool value =
		std::is_constructible_v<object, typename T::value_type> or
		has_serialize_v<typename T::value_type, object_serializer> or
		has_value_serializer_v<typename T::value_type>;
};

template <typename T>
inline constexpr bool is_serializable_optional_type_v = is_serializable_optional_type<T>::value;

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
		not std::is_constructible_v<object, T> and
		has_value_serializer_v<T>)
struct serializer<T>
{
	static object serialize(const T &v)
	{
		return object(value_serializer<T>::to_string(v));
	}

	static object serialize(T &&v)
	{
		return object(value_serializer<T>::to_string(std::forward<T>(v)));
	}

	static T deserialize(const object &o)
	{
		return value_serializer<T>::from_string(o.get<std::string>());
	}
};

template <typename T>
	requires(
		std::is_constructible_v<object, T> and
		not std::is_same_v<T, std::initializer_list<object>> and
		not std::is_same_v<T, object> and
		not is_serializable_map_type_v<T> and
		not is_serializable_array_type_v<T>)
struct serializer<T>
{
	static object serialize(const T &v)
	{
		return object(v);
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
		using value_serializer_impl = serializer<typename T::value_type>;

		object o = object::value_type::array;

		for (auto &i : v)
			o.push_back(value_serializer_impl::serialize(i));

		return o;
	}

	static T deserialize(const object &o)
	{
		using value_deserializer_impl = serializer<typename T::value_type>;

		T result{};

		for (auto &i : o)
			result.emplace_back(value_deserializer_impl::deserialize(i));

		return result;
	}
};

template <typename T>
	requires is_serializable_optional_type_v<T>
struct serializer<T>
{
	static object serialize(const T &v)
	{
		using value_serializer_impl = serializer<typename T::value_type>;

		object result;
		if (v)
			result = value_serializer_impl::serialize(*v);
		return result;
	}

	static T deserialize(const object &o)
	{
		using value_serializer_impl = serializer<typename T::value_type>;

		T result{};
		if (not o.is_null())
			result = value_serializer_impl::deserialize(o);
		return result;
	}
};

// --------------------------------------------------------------------

template<typename T>
using serialize_to_object_function = decltype(zeep::el::serializer<T>::serialize(std::declval<const typename T::value_type&>()));

template <typename T>
struct is_serializable_to_object
{
	static constexpr bool value =
		std::experimental::is_detected_v<serialize_to_object_function, T>;
};

template <typename T>
inline constexpr bool is_serializable_to_object_v = is_serializable_to_object<T>::value;

} // namespace zeep::el
