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

namespace zeep
{
namespace el
{
namespace detail
{

// Factory class to construct elements 
template<value_type> struct factory {};

template<>
struct factory<value_type::boolean>
{
	template<typename J>
	static void construct(J& j, bool b)
	{
		j.m_type = value_type::boolean;
		j.m_data = b;
		j.validate();
	}
};

template<>
struct factory<value_type::string>
{
	template<typename J>
	static void construct(J& j, const std::string& s)
	{
		j.m_type = value_type::string;
		j.m_data = s;
		j.validate();
	}

	template<typename J>
	static void construct(J& j, std::string&& s)
	{
		j.m_type = value_type::string;
		j.m_data = std::move(s);
		j.validate();
	}

	// template<typename J, typename T,
	// 	std::enable_if_t<not std::is_same<T, typename J::string_type>::value, int> = 0>
	// static void construct(J& j, const T& s)
	// {
	// 	j.m_type = value_type::string;
	// 	j.m_data = j.template create<typename J::string_type>(s);
	// 	j.validate();
	// }

	// template<typename J, typename T, size_t N,
	// 	std::enable_if_t<std::is_same<T, typename J::string_type::value_type>::value, int> = 0>
	// static void construct(J& j, const T(&a)[N])
	// {
	// 	j.m_type = value_type::string;
	// 	j.m_data = j.template create<typename J::string_type>(a, a + N);
	// 	j.validate();
	// }

};

template<>
struct factory<value_type::number_float>
{
	template<typename J>
	static void construct(J& j, double d)
	{
		j.m_type = value_type::number_float;
		j.m_data = d;
		j.validate();
	}
};

template<>
struct factory<value_type::number_int>
{
	template<typename J>
	static void construct(J& j, int64_t i)
	{
		j.m_type = value_type::number_int;
		j.m_data = i;
		j.validate();
	}
};

template<>
struct factory<value_type::array>
{
	template<typename J>
	static void construct(J& j, const typename J::array_type& arr)
	{
		j.m_type = value_type::array;
		j.m_data = arr;
		j.validate();
	}

	template<typename J>
	static void construct(J& j, typename J::array_type&& arr)
	{
		j.m_type = value_type::array;
		j.m_data = std::move(arr);
		j.validate();
	}

	template<typename J>
	static void construct(J& j, const std::vector<bool>& arr)
	{
		j.m_type = value_type::array;
		j.m_data = value_type::array;
		j.m_data.m_array->reserve(arr.size());
		std::copy(arr.begin(), arr.end(), std::back_inserter(*j.m_data.m_array));
		j.validate();
	}

	template<typename J, typename T,
		typename std::enable_if_t<std::is_convertible<T, element>::value, int> = 0>
	static void construct(J& j, const std::vector<T>& arr)
	{
		j.m_type = value_type::array;
		j.m_data = value_type::array;
		j.m_data.m_array->resize(arr.size());
		std::copy(arr.begin(), arr.end(), j.m_data.m_array->begin());
		j.validate();
	}

	template<typename J, typename T, size_t N,
		typename std::enable_if_t<std::is_convertible<T, J>::value, int> = 0>
	static void construct(J& j, const T(&arr)[N])
	{
		j.m_type = value_type::array;
		j.m_data = value_type::array;
		j.m_data.m_array->resize(N);
		std::copy(arr, arr + N, j.m_data.m_array->begin());
		j.validate();
	}
};

template<>
struct factory<value_type::object>
{
	template<typename J>
	static void construct(J& j, const typename J::object_type& obj)
	{
		j.m_type = value_type::object;
		j.m_data = obj;
		j.validate();
	}

	template<typename J>
	static void construct(J& j, typename J::object_type&& obj)
	{
		j.m_type = value_type::object;
		j.m_data = std::move(obj);
		j.validate();
	}

    template<typename J, typename M,
             std::enable_if_t<not std::is_same<M, typename J::object_type>::value, int> = 0>
    static void construct(J& j, const M& obj)
    {
        using std::begin;
        using std::end;

        j.m_type = value_type::object;
        j.m_data.m_object = j.template create<typename J::object_type>(begin(obj), end(obj));
        j.validate();
    }
};

}
}
}
