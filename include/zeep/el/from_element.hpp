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

namespace zeep
{
namespace el
{
namespace detail
{

template<typename E>
void from_element(const E& e, std::nullptr_t& n)
{
    if (not e.is_null())
        throw std::runtime_error("Type should have been null but was " + e.type_name());
    n = nullptr;
}

template<typename E, typename T,
    std::enable_if_t<std::is_arithmetic<T>::value and not std::is_same<T, bool>::value, int> = 0>
void get_number(const E& e, T& v)
{
    switch (e.type())
    {
        case value_type::number_int:
            v = static_cast<T>(*e.template get_ptr<const typename E::int_type*>());
            break;

        case value_type::number_float:
            v = static_cast<T>(*e.template get_ptr<const typename E::float_type*>());
            break;
        
        default:
            throw std::runtime_error("Type should have been number but was " + e.type_name());
            break;
    }
}

template<typename E>
void from_element(const E& e, typename E::boolean_type& b)
{
    if (not e.is_boolean())
        throw std::runtime_error("Type should have been boolean but was" + e.type_name());
    b = *e.template get_ptr<const typename E::boolean_type*>();
}

// template<typename E>
// void from_element(const E& e, typename E::string_type& s)
// {
//     if (not e.is_string())
//         throw std::runtime_error("Type should have been string but was " + e.type_name());
//     s = *e.template get_ptr<const typename E::string_type*>();
// }

template<typename E>
void from_element(const E& e, std::string& s)
{
    if (not e.is_string())
        throw std::runtime_error("Type should have been string but was " + e.type_name());
    s = *e.template get_ptr<const typename E::string_type*>();
}

template<typename E>
void from_element(const E& e, typename E::int_type& i)
{
    get_number(e, i);
}

template<typename E>
void from_element(const E& e, typename E::float_type& f)
{
    get_number(e, f);
}

template<typename E, typename A,
    std::enable_if_t<
        std::is_arithmetic<A>::value and
        not std::is_same<A, typename E::boolean_type>::value and
        not std::is_same<A, typename E::int_type>::value and
        not std::is_same<A, typename E::float_type>::value, int> = 0>
void from_element(const E& e, A& v)
{
    switch (e.type())
    {
        case value_type::boolean:       v = *e.template get_ptr<const typename E::boolean_type*>(); break;
        case value_type::number_int:    v = *e.template get_ptr<const typename E::int_type*>(); break;
        case value_type::number_float:  v = *e.template get_ptr<const typename E::float_type*>(); break;
        default:    throw std::runtime_error("Type should have been number but was " + e.type_name()); break;
    }
}

template<typename E, typename Enum, std::enable_if_t<std::is_enum<Enum>::value, int> = 0>
void from_element(const E& e, Enum &en)
{
    typename std::underlying_type<Enum>::type v;
    get_number(e, v);
    en = static_cast<Enum>(v);
}

// arrays

// nice trick to enforce order in template selection
template<unsigned N> struct priority_tag : priority_tag < N - 1 > {};
template<> struct priority_tag<0> {};

template<typename E>
void from_element_array_impl(const E& e, typename E::array_type& arr, priority_tag<3>)
{
    arr = *e.template get_ptr<const typename E::array_type*>();
}

template<typename E, typename T, size_t N>
auto from_element_array_impl(const E& e, std::array<T, N>& arr, priority_tag<2>)
    -> decltype(e.template as<T>(), void())
{
    for (size_t i = 0; i < N; ++i)
        arr[i] = e.at(i).template as<T>();
}

template<typename E, typename A>
auto from_element_array_impl(const E& e, A& arr, priority_tag<1>)
    -> decltype(
        arr.reserve(std::declval<typename A::size_type>()),
        e.template as<typename A::value_type>(),
        void()
    )
{
    arr.reserve(e.size());
    std::transform(e.begin(), e.end(), std::inserter(arr, std::end(arr)),
        [](const E& i)
    {
        return i.template as<typename A::value_type>();
    });
}

template<typename E, typename A>
void from_element_array_impl(const E& e, A& arr, priority_tag<0>)
{
    std::transform(e.begin(), e.end(), std::inserter(arr, std::end(arr)),
        [](const E& i)
    {
        return i.template as<typename A::value_type>();
    });
}

template<typename E, typename A,
    std::enable_if_t<
        is_constructible_array_type<E, A>::value, int> = 0>
void from_element(const E& e, A& arr)
{
    if (not e.is_array())
        throw std::runtime_error("Type should have been array but was " + e.type_name());
    from_element_array_impl(e, arr, priority_tag<3>{});
}

struct from_element_fn
{
    template<typename T>
    auto operator()(const element& j, T& val) const noexcept(noexcept(from_element(j, val)))
    -> decltype(from_element(j, val), void())
    {
        return from_element(j, val);
    }
};

namespace
{
    constexpr const auto& from_element = typename ::zeep::el::detail::from_element_fn{};
}

}
}
}
