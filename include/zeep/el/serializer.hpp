// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2019-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the serializer classes that help serialize data into and out of our el script objects

#include "zeep/el/object.hpp"

#include <zeem/zeem.hpp>

// --------------------------------------------------------------------

namespace zeep
{

/// \brief Alias for zeem::name_value_pair, used in conjunction with object_serializer
///        to serialize individual struct members
template <typename T>
using name_value_pair = zeem::name_value_pair<T>;

/// \brief Alias for zeem::value_serializer, used to convert between C++ types
///        and their string representation
template <typename T>
using value_serializer = zeem::value_serializer<T>;

} // namespace zeep

// --------------------------------------------------------------------

namespace zeep::el
{

/// \brief Primary template for the serializer struct
///
/// Specializations of \a serializer provide static \a serialize and \a deserialize
/// methods for converting between C++ types and \a object. The primary template
/// is left undefined; only the specializations below are valid.
/// \tparam T  The C++ type to serialize
/// \tparam enabled  Unused, allows SFINAE-based specializations
template <typename T, typename = void>
struct serializer;

/// \brief Serializer helper used by types that expose a \a serialize method
///        taking an \a object_serializer parameter
struct object_serializer;

/// \brief Deserializer helper used by types that expose a \a serialize method
///        taking an \a object_deserializer parameter
struct object_deserializer;

// --------------------------------------------------------------------
/// \brief Detects whether a value_serializer specialization exists for type \a T
///
/// Checks for both \a to_string and \a from_string member functions.

template <typename T>
using vs_to_string_function = decltype(value_serializer<T>::to_string(std::declval<T &>()));

template <typename T>
using vs_from_string_function = decltype(value_serializer<T>::from_string(std::declval<const std::string &>()));

template <typename T>
struct has_value_serializer
{
	static constexpr bool value =
		zeem::detail::is_detected_v<vs_to_string_function, T> and
		zeem::detail::is_detected_v<vs_from_string_function, T>;
};

/// \brief Convenience variable template for \a has_value_serializer
template <typename T>
inline constexpr bool has_value_serializer_v = has_value_serializer<T>::value;

// --------------------------------------------------------------------
/// \brief Detects whether type \a T has a \a serialize(Archive&, uint64_t) member

template <typename T, typename Archive>
using serialize_function = decltype(std::declval<T &>().serialize(std::declval<Archive &>(), std::declval<uint64_t>()));

template <typename T, typename Archive, typename = void>
struct has_serialize : std::false_type
{
};

template <typename T, typename Archive>
struct has_serialize<T, Archive, typename std::enable_if_t<std::is_class_v<T>>>
{
	static constexpr bool value = zeem::detail::is_detected_v<serialize_function, T, Archive>;
};

/// \brief Convenience variable template for \a has_serialize
template <typename T, typename S>
inline constexpr bool has_serialize_v = has_serialize<T, S>::value;

// --------------------------------------------------------------------
/// \brief Detects whether \a T is a map-like type (e.g. std::map<std::string, V>)
///        that can be serialized to an object value

template <typename T>
using mapped_type_t = typename T::mapped_type;

template <typename T>
using key_type_t = typename T::key_type;

template <typename T, typename = void>
struct is_serializable_map_type : std::false_type
{
};

template <typename T>
struct is_serializable_map_type<T,
	std::enable_if_t<
		zeem::detail::is_detected_v<mapped_type_t, T> and
		zeem::detail::is_detected_v<key_type_t, T> and
		zeem::detail::is_detected_v<zeem::iterator_t, T>>>
{
	static constexpr bool value =
		std::is_same_v<typename T::key_type, std::string> and
		(std::is_constructible_v<object, typename T::mapped_type> or
			has_serialize_v<typename T::mapped_type, object_serializer>);
};

/// \brief Convenience variable template for \a is_serializable_map_type
template <typename T>
inline constexpr bool is_serializable_map_type_v = is_serializable_map_type<T>::value;

// --------------------------------------------------------------------
/// \brief Detects whether \a T is an array-like type (e.g. std::vector<V>)
///        that can be serialized to an array value

template <typename T, typename = void>
struct is_serializable_array_type : std::false_type
{
};

template <typename T>
struct is_serializable_array_type<T,
	std::enable_if_t<
		not zeem::detail::is_detected_v<mapped_type_t, T> and
		not zeem::detail::is_detected_v<key_type_t, T> and
		zeem::detail::is_detected_v<zeem::value_type_t, T> and
		zeem::detail::is_detected_v<zeem::iterator_t, T> and
		not zeem::detail::is_detected_v<zeem::std_string_npos_t, T>>>
{
	static constexpr bool value = std::is_constructible_v<object, typename T::value_type> or
	                              has_serialize_v<typename T::value_type, object_serializer>;
};

/// \brief Convenience variable template for \a is_serializable_array_type
template <typename T>
inline constexpr bool is_serializable_array_type_v = is_serializable_array_type<T>::value;

// --------------------------------------------------------------------

/// \brief Helper struct for serializing C++ objects to an \a object
///
/// Iterates over the members (via name_value_pair) and stores each value
/// as a named entry in the resulting object.

struct object_serializer
{
	object_serializer() = default;

	/// \brief Accept a name-value pair and serialize the value under the given name
	/// \tparam T  The type of the value to serialize
	template <typename T>
	object_serializer &operator&(name_value_pair<T> &&nvp)
	{
		serialize(nvp.name(), nvp.value());
		return *this;
	}

	/// \brief Serialize a named value into the result object
	/// \param name  The key under which to store the value
	/// \param data  The value to serialize
	template <typename T>
	void serialize(std::string name, const T &data)
	{
		using serializer_impl = serializer<T>;

		m_elem.emplace(name, serializer_impl::serialize(data));
	}

	/// \brief Static convenience: serialize \a v directly into \a o
	template <typename T>
	static void serialize(object &o, const T &v)
	{
		using serializer_impl = serializer<T>;

		o = serializer_impl::serialize(v);
	}

	/// \brief The accumulated serialized object
	object m_elem;
};

/// \brief Helper struct for deserializing C++ objects from an \a object
///
/// Reads named members from an \a object and assigns them to the corresponding
/// fields via name_value_pair.

struct object_deserializer
{
	/// \brief Construct a deserializer from a source object
	explicit object_deserializer(const object &o)
		: m_elem(o)
	{
	}

	/// \brief Accept a name-value pair and deserialize the value from the named member
	/// \tparam T  The type to deserialize into
	template <typename T>
	object_deserializer &operator&(name_value_pair<T> &&nvp)
	{
		deserialize(nvp.name(), nvp.value());
		return *this;
	}

	/// \brief Deserialize a named member from the source object
	/// \param name  The key to look up
	/// \param data  Reference to store the deserialized value into
	template <typename T>
	void deserialize(const std::string &name, T &data)
	{
		if (not m_elem.is_object() or m_elem.empty())
			return;

		using serializer_impl = serializer<T>;

		auto value = m_elem[name];

		if (value.is_null())
			return;

		data = serializer_impl::deserialize(value);
	}

	/// \brief The source object being deserialized from
	const object &m_elem;
};

// --------------------------------------------------------------------
/// \name Serializer specializations

///@{

/// \brief Specialization for types that have a \a value_serializer but are not
///        constructible as an \a object directly.
///
/// Converts to/from a string representation using \a value_serializer::to_string
/// and \a value_serializer::from_string. Excludes enum types (handled separately).

template <typename T>
	requires(
		not std::is_constructible_v<object, T> and
		has_value_serializer_v<T> and
		not std::is_enum_v<T>)
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

/// \brief Specialization for types directly constructible from (or convertible to)
///        \a object, such as built-in arithmetic types, std::string, bool, etc.
///
/// Excludes \a object itself, \a std::initializer_list, map-like types,
/// array-like types, and enums.

template <typename T>
	requires(
		std::is_constructible_v<object, T> and
		not std::is_same_v<T, std::initializer_list<object>> and
		not std::is_same_v<T, object> and
		not is_serializable_map_type_v<T> and
		not is_serializable_array_type_v<T> and
		not std::is_enum_v<T>)
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

/// \brief Specialization for types that expose a \a serialize(Archive&, uint64_t) member
///
/// The member function writes into an \a object_serializer during serialization
/// and reads from an \a object_deserializer during deserialization.

template <typename T>
	requires zeem::has_serialize_v<T, object_serializer>
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

/// \brief Specialization for map-like types (e.g. std::map<std::string, V>)
///
/// Serializes to an \a object with keys matching the map keys and values
/// recursively serialized.

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

/// \brief Specialization for array-like types (e.g. std::vector<V>)
///
/// Serializes to an \a object array with each element recursively serialized.

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

/// \brief Specialization for std::optional<T>
///
/// Serializes a disengaged optional to null, an engaged optional to the
/// serialized value of \a T. Deserializes null to disengaged, any other
/// value to the deserialized \a T wrapped in optional.

template <typename T>
struct serializer<std::optional<T>>
{
	static object serialize(const std::optional<T> &v)
	{
		using value_serializer_impl = serializer<T>;

		object result;
		if (v)
			result = value_serializer_impl::serialize(*v);
		return result;
	}

	static std::optional<T> deserialize(const object &o)
	{
		using value_serializer_impl = serializer<T>;

		std::optional<T> result;
		if (not o.is_null())
			result = value_serializer_impl::deserialize(o);
		return result;
	}
};

/// \brief Specialization for enum types that have a \a value_serializer
///
/// Converts the enum to/from its string representation.

template <typename T>
	requires(std::is_enum_v<T> and has_value_serializer_v<T>)
struct serializer<T>
{
	static object serialize(T v)
	{
		return object(value_serializer<T>::to_string(v));
	}

	static T deserialize(const object &o)
	{
		return value_serializer<T>::from_string(o.get<std::string>());
	}
};

///@}

// --------------------------------------------------------------------
/// \brief Detects whether a type \a T has a valid \a serializer specialization

template <typename T>
using serialize_to_object_function = decltype(zeep::el::serializer<T>::serialize(std::declval<T &>()));

template <typename T>
struct is_serializable_to_object
{
	static constexpr bool value =
		zeem::detail::is_detected_v<serialize_to_object_function, T>;
};

/// \brief Convenience variable template for \a is_serializable_to_object
template <typename T>
inline constexpr bool is_serializable_to_object_v = is_serializable_to_object<T>::value;

// --------------------------------------------------------------------

/// \brief Convenience function to serialize a value to an \a object
/// \tparam T  The type to serialize (must be serializable)
/// \param v   The value to serialize
/// \return    The resulting object

template <typename T>
object to_object(const T &v)
	requires(is_serializable_to_object_v<T>)
{
	using value_serializer_impl = serializer<T>;
	return value_serializer_impl::serialize(v);
}

/// \brief Convenience function to deserialize a value from an \a object
/// \tparam T  The target type (must be deserializable)
/// \param o   The object to deserialize from
/// \return    The deserialized value

template <typename T>
T from_object(const object &o)
	requires(is_serializable_to_object_v<T>)
{
	using value_serializer_impl = serializer<T>;
	return value_serializer_impl::deserialize(o);
}

} // namespace zeep::el
