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
#include <zeep/el/traits.hpp>
#include <zeep/el/factory.hpp>

#include <boost/optional.hpp>

namespace zeep
{
namespace el
{
namespace detail
{

template<typename T, std::enable_if_t<std::is_same<T, bool>::value, int> = 0>
void to_element(element& v, T b)
{
	factory<value_type::boolean>::construct(v, b);
}

template<typename J, typename T, size_t N,
	std::enable_if_t<std::is_constructible<typename J::string_type, const T(&)[N]>::value, int> = 0>
void to_element(J& j, const T(&arr)[N])
{
	factory<value_type::string>::construct(j, std::move(arr));
}

template<typename E, typename T, std::enable_if_t<std::is_constructible<typename E::string_type, T>::value, int> = 0>
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
void to_element(E& v, std::string&& s)
{
	factory<value_type::string>::construct(v, std::move(s));
}

template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
void to_element(element& v, T f)
{
	factory<value_type::number_float>::construct(v, f);
}

template<typename T, std::enable_if_t<std::is_integral<T>::value and not std::is_same<T, bool>::value, int> = 0>
void to_element(element& v, T i)
{
	factory<value_type::number_int>::construct(v, i);
}

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
void to_element(element& v, T e)
{
	using int_type = typename std::underlying_type<T>::type;
	factory<value_type::number_int>::construct(v, static_cast<int_type>(e));
}

template<typename T, std::enable_if_t<std::is_same<T, bool>::value, int> = 0>
void to_element(element& j, const std::vector<T>& v)
{
	factory<value_type::array>::construct(j, v);
}

template<typename T, std::enable_if_t<
	is_compatible_array_type<T>::value and
	not is_compatible_object_type<element,T>::value and
	not is_compatible_string_type<element,T>::value and
	not is_element<T>::value, int> = 0>
void to_element(element& j, const T& arr)
{
	factory<value_type::array>::construct(j, arr);
}

template<typename T, std::enable_if_t<std::is_convertible<element,T>::value, int> = 0>
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
	std::enable_if_t<not std::is_constructible<typename J::string_type, const T(&)[N]>::value, int> = 0>
void to_element(J& j, const T(&arr)[N])
{
	factory<value_type::array>::construct(j, std::move(arr));
} 

template<typename T, std::enable_if_t<is_object_type<element,T>::value, int> = 0>
void to_element(element& j, const T& obj)
{
	factory<value_type::object>::construct(j, std::move(obj));
}

template<typename J>
void to_element(J& j, const J& obj)
{
	factory<value_type::object>::construct(j, std::move(obj));
}

template<typename T, std::enable_if_t<has_to_element<T>::value>>
void to_element(element& j, const boost::optional<T>& v)
{
	if (v)
		to_element(j, *v);
}


} // detail
} // el
} // zeep

