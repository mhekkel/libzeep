//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// various templated classes that help selecting the right conversion routines when (de-)serializing zeep::json::element (JSON) objects

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>

#include <zeep/type-traits.hpp>
#include <zeep/json/element_fwd.hpp>

namespace zeep::json::detail
{

template<typename> struct is_element : std::false_type {};
template<> struct is_element<element> : std::true_type {};

template<typename T>
inline constexpr bool is_element_v = is_element<T>::value;

template <typename T>
using mapped_type_t = typename T::mapped_type;

template <typename T>
using key_type_t = typename T::key_type;

template <typename T>
using value_type_t = typename T::value_type;

template <typename T>
using iterator_t = typename T::iterator;

template <typename T>
using iterator_category_t = typename T::iterator_category;

template <typename T>
using difference_type_t = typename T::difference_type;

template <typename T>
using reference_t = typename T::reference;

template <typename T>
using pointer_t = typename T::pointer;

template <typename T, typename = void>
struct is_iterator_traits : std::false_type {};

template <typename T>
struct is_iterator_traits<std::iterator_traits<T>>
{
  private:
    using traits = std::iterator_traits<T>;

  public:
    static constexpr auto value =
        std::experimental::is_detected_v<value_type_t, traits> and
        std::experimental::is_detected_v<difference_type_t, traits> and
        std::experimental::is_detected_v<pointer_t, traits> and
        std::experimental::is_detected_v<iterator_category_t, traits> and
        std::experimental::is_detected_v<reference_t, traits>;
};

template<typename T>
inline constexpr bool is_iterator_traits_v = is_iterator_traits<T>::value;

template <typename T, typename... Args>
using to_element_function = decltype(T::to_element(std::declval<Args>()...));

template <typename T, typename... Args>
using from_element_function = decltype(T::from_element(std::declval<Args>()...));

template<typename T, typename = void>
struct has_to_element : std::false_type {};

template<typename T>
struct has_to_element<T, std::enable_if_t<not is_element_v<T>>>
{
	using serializer = element_serializer<T, void>;
	static constexpr bool value =
		std::experimental::is_detected_exact_v<void, to_element_function, serializer, element&, T>;
};

template<typename T>
inline constexpr bool has_to_element_v = has_to_element<T>::value;

template<typename T, typename = void>
struct has_from_element : std::false_type {};

template<typename T>
struct has_from_element<T, std::enable_if_t<not is_element_v<T>>>
{
	using serializer = element_serializer<T, void>;
	static constexpr bool value =
		std::experimental::is_detected_exact_v<void, from_element_function, serializer, const element&, T&>;
};

template<typename T>
inline constexpr bool has_from_element_v = has_from_element<T>::value;

template<typename T, typename = void>
struct is_compatible_array_type : std::false_type {};

template<typename T>
struct is_compatible_array_type<T,
	std::enable_if_t<
		std::experimental::is_detected_v<value_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		std::is_constructible_v<element, typename T::value_type>>> : std::true_type {};

template<typename T>
inline constexpr bool is_compatible_array_type_v = is_compatible_array_type<T>::value;

template<typename E, typename T, typename = void>
struct is_constructible_array_type : std::false_type {};

template<typename E, typename T>
struct is_constructible_array_type<E, T,
	std::enable_if_t<
		std::experimental::is_detected_v<value_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		is_complete_type_v<std::experimental::detected_t<value_type_t, T>> >>
{
    static constexpr bool value =
		not is_iterator_traits_v<std::iterator_traits<T>> and
		(
			std::is_same_v<typename T::value_type, typename E::array_type::value_type> or
			has_from_element_v<typename T::value_type>
		);
};

template<typename E, typename T>
inline constexpr bool is_constructible_array_type_v = is_constructible_array_type<E,T>::value;

template<typename J, typename T, typename = void>
struct is_object_type : std::false_type {};

template<typename J, typename T>
struct is_object_type<J, T, std::enable_if_t<
	std::experimental::is_detected_v<mapped_type_t, T> and
	std::experimental::is_detected_v<key_type_t, T>>>
{
	using map_t = typename J::object_type;
	static constexpr bool value = 
		std::is_constructible_v<std::string,typename map_t::key_type> and
		std::is_constructible_v<element,typename map_t::mapped_type>; 
};

template<typename J, typename T>
inline constexpr bool is_object_type_v = is_object_type<J,T>::value;

// any compatible type

template<typename T, typename = void>
struct is_compatible_type : std::false_type {};

template<typename T>
struct is_compatible_type<T, std::enable_if_t<is_complete_type_v<T>>>
{
	static constexpr bool value = has_to_element_v<T>;
};

template<typename T>
inline constexpr bool is_compatible_type_v = is_compatible_type<T>::value;

// compatible string

template<typename E, typename T, typename = void>
struct is_compatible_string_type : std::false_type {};

template<typename E, typename T>
struct is_compatible_string_type<E, T,
	std::enable_if_t<
		std::experimental::is_detected_exact_v<typename E::string_type::value_type, value_type_t, T>>>
{
	static constexpr bool value =
		std::is_constructible_v<typename E::string_type, T>;
};

template<typename E, typename T>
inline constexpr bool is_compatible_string_type_v = is_compatible_string_type<E, T>::value;

// compatible object

template<typename J, typename T, typename = void>
struct is_compatible_object_type : std::false_type {};

template<typename J, typename T>
struct is_compatible_object_type<J, T, std::enable_if_t<
	std::experimental::is_detected_v<mapped_type_t, T> and
	std::experimental::is_detected_v<key_type_t, T>>>
{
	using map_t = typename J::object_type;
	static constexpr bool value = 
		std::is_constructible_v<typename J::object_type::key_type,typename map_t::key_type> and
		(std::is_same_v<typename J::value_type,typename map_t::mapped_type> or
			has_from_element_v<typename T::mapped_type>); 
};

template<typename J, typename T>
inline constexpr bool is_compatible_object_type_v = is_compatible_object_type<J,T>::value;


template<typename T, typename Archive, typename = void>
struct is_serializable_map_type : std::false_type {};

template<typename T, typename Archive>
struct is_serializable_map_type<T, Archive,
	std::enable_if_t<
		std::experimental::is_detected_v<detail::mapped_type_t, T> and
		std::experimental::is_detected_v<detail::key_type_t, T> and
		std::experimental::is_detected_v<detail::iterator_t, T> and
		not detail::is_compatible_string_type_v<typename Archive::element_type,T>>>
{
	static constexpr bool value =
		std::is_same_v<typename T::key_type, std::string> and
		(
			detail::is_compatible_type_v<typename T::mapped_type> or
			has_serialize_v<typename T::mapped_type, Archive>
		);
};

template<typename T, typename Archive>
inline constexpr bool is_serializable_map_type_v = is_serializable_map_type<T, Archive>::value;

template<typename T>
using has_value_or_result = decltype(std::declval<T>().value_or(std::declval<typename T::value_type&&>()));

template<typename T, typename Archive, typename = void>
struct is_serializable_optional_type : std::false_type {};

template<typename T, typename Archive>
struct is_serializable_optional_type<T, Archive,
	std::enable_if_t<
		std::experimental::is_detected_v<detail::value_type_t, T> and
		std::is_same_v<has_value_or_result<T>,typename T::value_type> and
		not detail::is_compatible_string_type_v<typename Archive::element_type,T>>>
{
	static constexpr bool value =
		detail::is_compatible_type_v<typename T::value_type> or
		has_serialize_v<typename T::value_type, Archive>;
};

template<typename T, typename Archive>
inline constexpr bool is_serializable_optional_type_v = is_serializable_optional_type<T, Archive>::value;

}
