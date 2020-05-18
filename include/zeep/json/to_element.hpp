//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// various implementations of the to_element function that intializes a zeep::json::element object with some value

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <optional>

#include <zeep/json/element_fwd.hpp>
#include <zeep/json/factory.hpp>
#include <zeep/json/type_traits.hpp>

namespace zeep::json::detail
{

template<typename T, std::enable_if_t<std::is_same_v<T, bool>, int> = 0>
void to_element(element& v, T b)
{
	factory<value_type::boolean>::construct(v, b);
}

template<typename J, typename T, size_t N,
	std::enable_if_t<std::is_constructible_v<typename J::string_type, const T(&)[N]>, int> = 0>
void to_element(J& j, const T(&arr)[N])
{
	factory<value_type::string>::construct(j, std::move(arr));
}

template<typename E, typename T, std::enable_if_t<std::is_constructible_v<typename E::string_type, T>, int> = 0>
void to_element(E& v, const T& s)
{
	factory<value_type::string>::construct(v, s);
}

template<typename E>
inline void to_element(E& v, const std::string& s)
{
	factory<value_type::string>::construct(v, s);
}

template<typename E>
inline void to_element(E& v, const std::wstring& s)
{
	factory<value_type::string>::construct(v, s);
}

template<typename E>
void to_element(E& v, std::string&& s)
{
	factory<value_type::string>::construct(v, std::move(s));
}

template<typename E>
void to_element(E& v, std::wstring&& s)
{
	factory<value_type::string>::construct(v, std::move(s));
}

template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
void to_element(element& v, T f)
{
	factory<value_type::number_float>::construct(v, f);
}

template<typename T, std::enable_if_t<std::is_integral_v<T> and not std::is_same_v<T, bool>, int> = 0>
void to_element(element& v, T i)
{
	factory<value_type::number_int>::construct(v, i);
}

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
void to_element(element& v, T e)
{
	using int_type = typename std::underlying_type_t<T>;
	factory<value_type::number_int>::construct(v, static_cast<int_type>(e));
}

template<typename T, std::enable_if_t<std::is_same_v<T, bool>, int> = 0>
void to_element(element& j, const std::vector<T>& v)
{
	factory<value_type::array>::construct(j, v);
}

template<typename T, std::enable_if_t<
	is_compatible_array_type_v<T> and
	not is_compatible_object_type_v<element,T> and
	not is_compatible_string_type_v<element,T> and
	not is_element_v<T>, int> = 0>
void to_element(element& j, const T& arr)
{
	factory<value_type::array>::construct(j, arr);
}

template<typename T, std::enable_if_t<std::is_convertible_v<element,T>, int> = 0>
void to_element(element& j, const std::valarray<T>& arr)
{
	factory<value_type::array>::construct(j, std::move(arr));
}

template<typename J>
void to_element(element& j, const typename J::array_type& arr)
{
	factory<value_type::array>::construct(j, std::move(arr));
}

template<typename J, typename T, size_t N,
	std::enable_if_t<not std::is_constructible_v<typename J::string_type, const T(&)[N]>, int> = 0>
void to_element(J& j, const T(&arr)[N])
{
	factory<value_type::array>::construct(j, std::move(arr));
} 

template<typename T, std::enable_if_t<is_object_type_v<element,T>, int> = 0>
void to_element(element& j, const T& obj)
{
	factory<value_type::object>::construct(j, std::move(obj));
}

template<typename J>
void to_element(J& j, const J& obj)
{
	factory<value_type::object>::construct(j, std::move(obj));
}

template<typename T, std::enable_if_t<is_compatible_type_v<T>>>
void to_element(element& j, const std::optional<T>& v)
{
	if (v)
		to_element(j, *v);
}


} // zeep::json::detail

