// //          Copyright Maarten L. Hekkelman, 2019
// // Distributed under the Boost Software License, Version 1.0.
// //    (See accompanying file LICENSE_1_0.txt or copy at
// //          http://www.boost.org/LICENSE_1_0.txt)

// #pragma once

// /// \file
// /// various implementations of the to_object function that intializes a zeep::json::object object with some value

// #include <zeep/config.hpp>

// #include <optional>
// #include <valarray>

// #include <zeep/el/type_traits.hpp>

// namespace zeep::json::detail
// {

// template<typename T, std::enable_if_t<std::is_same_v<T, bool>, int> = 0>
// void to_object(object& v, T b)
// {
// 	factory<value_type::boolean>::construct(v, b);
// }

// template<typename J, typename T, size_t N,
// 	std::enable_if_t<std::is_constructible_v<typename J::string_type, const T(&)[N]>, int> = 0>
// void to_object(J& j, const T(&arr)[N])
// {
// 	factory<value_type::string>::construct(j, std::move(arr));
// }

// template<typename E, typename T, std::enable_if_t<std::is_constructible_v<typename E::string_type, T>, int> = 0>
// void to_object(E& v, const T& s)
// {
// 	factory<value_type::string>::construct(v, s);
// }

// template<typename E>
// inline void to_object(E& v, const std::string& s)
// {
// 	factory<value_type::string>::construct(v, s);
// }

// template<typename E>
// inline void to_object(E& v, const std::wstring& s)
// {
// 	factory<value_type::string>::construct(v, s);
// }

// template<typename E>
// void to_object(E& v, std::string&& s)
// {
// 	factory<value_type::string>::construct(v, std::move(s));
// }

// template<typename E>
// void to_object(E& v, std::wstring&& s)
// {
// 	factory<value_type::string>::construct(v, std::move(s));
// }

// template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
// void to_object(object& v, T f)
// {
// 	factory<value_type::number_float>::construct(v, f);
// }

// template<typename T, std::enable_if_t<std::is_integral_v<T> and not std::is_same_v<T, bool>, int> = 0>
// void to_object(object& v, T i)
// {
// 	factory<value_type::number_int>::construct(v, i);
// }

// template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
// void to_object(object& v, T e)
// {
// 	if (value_serializer<T>::empty())
// 	{
// 		using int_type = typename std::underlying_type_t<T>;
// 		factory<value_type::number_int>::construct(v, static_cast<int_type>(e));
// 	}
// 	else
// 		factory<value_type::string>::construct(v, value_serializer<T>::to_string(e));
// }

// template<typename T, std::enable_if_t<std::is_same_v<T, bool>, int> = 0>
// void to_object(object& j, const std::vector<T>& v)
// {
// 	factory<value_type::array>::construct(j, v);
// }

// template<typename T, std::enable_if_t<
// 	is_compatible_array_type_v<T> and
// 	not is_compatible_object_type_v<object,T> and
// 	not is_compatible_string_type_v<object,T> and
// 	not is_object_v<T>, int> = 0>
// void to_object(object& j, const T& arr)
// {
// 	factory<value_type::array>::construct(j, arr);
// }

// template<typename T, std::enable_if_t<std::is_convertible_v<object,T>, int> = 0>
// void to_object(object& j, const std::valarray<T>& arr)
// {
// 	factory<value_type::array>::construct(j, std::move(arr));
// }

// template<typename J>
// void to_object(object& j, const typename J::array_type& arr)
// {
// 	factory<value_type::array>::construct(j, std::move(arr));
// }

// template<typename J, typename T, size_t N,
// 	std::enable_if_t<not std::is_constructible_v<typename J::string_type, const T(&)[N]>, int> = 0>
// void to_object(J& j, const T(&arr)[N])
// {
// 	factory<value_type::array>::construct(j, std::move(arr));
// } 

// template<typename T, std::enable_if_t<is_object_type_v<object,T>, int> = 0>
// void to_object(object& j, const T& obj)
// {
// 	factory<value_type::object>::construct(j, std::move(obj));
// }

// template<typename J>
// void to_object(J& j, const J& obj)
// {
// 	factory<value_type::object>::construct(j, std::move(obj));
// }

// template<typename T, std::enable_if_t<is_compatible_type_v<T>>>
// void to_object(object& j, const std::optional<T>& v)
// {
// 	if (v)
// 		to_object(j, *v);
// }

// } // zeep::json::detail

