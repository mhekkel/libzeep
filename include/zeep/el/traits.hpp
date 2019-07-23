//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <experimental/type_traits>

#include <zeep/el/element_fwd.hpp>

namespace zeep
{
namespace el
{
namespace detail
{

static_assert(__cpp_lib_experimental_detect, "This one is needed");

template<typename> struct is_element : std::false_type {};
template<> struct is_element<element> : std::true_type {};

template <typename T, typename = void>
struct is_iterator_traits : std::false_type {};

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

template <typename T, typename... Args>
using to_element_function = decltype(T::to_element(std::declval<Args>()...));

template <typename T, typename... Args>
using from_element_function = decltype(T::from_element(std::declval<Args>()...));

template <typename T>
struct is_iterator_traits<std::iterator_traits<T>>
{
  private:
    using traits = std::iterator_traits<T>;

  public:
    static constexpr auto value =
        std::experimental::is_detected<value_type_t, traits>::value &&
        std::experimental::is_detected<difference_type_t, traits>::value &&
        std::experimental::is_detected<pointer_t, traits>::value &&
        std::experimental::is_detected<iterator_category_t, traits>::value &&
        std::experimental::is_detected<reference_t, traits>::value;
};

template<typename T, typename = void>
struct has_to_element : std::false_type {};

template<typename T>
struct has_to_element<T, std::enable_if_t<not is_element<T>::value>>
{
	using serializer = element_serializer<T, void>;
	static constexpr bool value =
		std::experimental::is_detected_exact<void, to_element_function, serializer, element&, T>::value;
};

template<typename T, typename = void>
struct has_from_element : std::false_type {};

template<typename T>
struct has_from_element<T, std::enable_if_t<not is_element<T>::value>>
{
	using serializer = element_serializer<T, void>;
	static constexpr bool value =
		std::experimental::is_detected_exact<void, from_element_function, serializer, const element&, T&>::value;
};

template<typename T, typename = void>
struct is_complete_type : std::false_type {};

template<typename T>
struct is_complete_type<T, decltype(void(sizeof(T)))> : std::true_type {};

template<typename T, typename = void>
struct is_compatible_array_type : std::false_type {};

template<typename T>
struct is_compatible_array_type<T,
	std::enable_if_t<
		std::experimental::is_detected<value_type_t, T>::value and
		std::experimental::is_detected<iterator_t, T>::value>>
{
    static constexpr bool value =
        std::is_constructible<element, typename T::value_type>::value;
};

template<typename E, typename T, typename = void>
struct is_constructible_array_type : std::false_type {};

template<typename E, typename T>
struct is_constructible_array_type<E, T,
	std::enable_if_t<
		std::experimental::is_detected<value_type_t, T>::value and
		std::experimental::is_detected<iterator_t, T>::value and
		is_complete_type<std::experimental::detected_t<value_type_t, T>>::value >>
{
    static constexpr bool value =
		not is_iterator_traits<std::iterator_traits<T>>::value and
		(
			std::is_same<typename T::value_type, typename E::array_type::value_type>::value or
			has_from_element<typename T::value_type>::value
		);
};

template<typename J, typename T, typename = void>
struct is_object_type : std::false_type {};

template<typename J, typename T>
struct is_object_type<J, T, std::enable_if_t<
	std::experimental::is_detected<mapped_type_t, T>::value and
	std::experimental::is_detected<key_type_t, T>::value>>
{
	using map_t = typename J::object_type;
	static constexpr bool value = 
		std::is_constructible<std::string,typename map_t::key_type>::value and
		std::is_constructible<element,typename map_t::mapped_type>::value; 
};

// any compatible type

template<typename T, typename = void>
struct is_compatible_type : std::false_type {};

template<typename T>
struct is_compatible_type<T, std::enable_if_t<is_complete_type<T>::value>>
{
	static constexpr bool value = has_to_element<T>::value;
};

// compatible string

template<typename E, typename T, typename = void>
struct is_compatible_string_type : std::false_type {};

template<typename E, typename T>
struct is_compatible_string_type<E, T,
	std::enable_if_t<
		std::experimental::is_detected_exact<typename E::string_type::value_type, value_type_t, T>::value>>
{
	static constexpr bool value =
		std::is_constructible<typename E::string_type, T>::value;
};

// compatible object

template<typename J, typename T, typename = void>
struct is_compatible_object_type : std::false_type {};

template<typename J, typename T>
struct is_compatible_object_type<J, T, std::enable_if_t<
	std::experimental::is_detected<mapped_type_t, T>::value and
	std::experimental::is_detected<key_type_t, T>::value>>
{
	using map_t = typename J::object_type;
	static constexpr bool value = 
		std::is_constructible<typename J::object_type::key_type,typename map_t::key_type>::value and
		(std::is_same<typename J::value_type,typename map_t::mapped_type>::value or
			has_from_element<typename T::mapped_type>::value); 
};

}
}
}
