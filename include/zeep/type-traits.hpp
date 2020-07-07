//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of various classes that help classify data used to select the correct conversion routines

#include <zeep/config.hpp>

#include <experimental/type_traits>

#include <zeep/value-serializer.hpp>

#if (defined(__cpp_lib_experimental_detect) and (__cpp_lib_experimental_detect < 201505)) or (defined(_LIBCPP_VERSION) and _LIBCPP_VERSION < 5000)
// This code is copied from:
// https://ld2015.scusa.lsu.edu/cppreference/en/cpp/experimental/is_detected.html

namespace std
{
	template< class... >
	using void_t = void;

	namespace experimental
	{
		namespace detail
		{
			template <class Default, class AlwaysVoid,
					template<class...> class Op, class... Args>
			struct detector {
			using value_t = false_type;
			using type = Default;
			};
			
			template <class Default, template<class...> class Op, class... Args>
			struct detector<Default, std::tr1::void_t<Op<Args...>>, Op, Args...> {
			// Note that std::void_t is a c++17 feature
			using value_t = true_type;
			using type = Op<Args...>;
			};
		
		} // namespace detail

		struct nonesuch {

			nonesuch() = delete;
			~nonesuch() = delete;
			nonesuch(nonesuch const&) = delete;
			void operator=(nonesuch const&) = delete;
		};

		template <template<class...> class Op, class... Args>
		using is_detected = typename detail::detector_v<nonesuch, void, Op, Args...>_t;
		
		template <template<class...> class Op, class... Args>
		using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;
		
		template <class Default, template<class...> class Op, class... Args>
		using detected_or = detail::detector<Default, void, Op, Args...>;

		template <class Expected, template <class...> class Op, class... Args>
		using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;
	}
}

#endif

/// \brief Templates to help selecting the correct serialization class.

namespace zeep
{

template <typename T>
using value_type_t = typename T::value_type;

template <typename T>
using key_type_t = typename T::key_type;

template <typename T>
using mapped_type_t = typename T::mapped_type;

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

template<typename T, typename = void>
struct is_complete_type : std::false_type {};

template<typename T>
struct is_complete_type<T, decltype(void(sizeof(T)))> : std::true_type {};

template<typename T>
inline constexpr bool is_complete_type_v = is_complete_type<T>::value;

template<typename T>
using std_string_npos_t = decltype(T::npos);

template<typename T>
using serialize_value_t = decltype(std::declval<value_serializer<T>&>().from_string(std::declval<const std::string&>()));

template<typename T, typename Archive>
using serialize_function = decltype(std::declval<T&>().serialize(std::declval<Archive&>(), std::declval<unsigned long>()));

template<typename T, typename Archive, typename = void>
struct has_serialize : std::false_type {};

template<typename T, typename Archive>
struct has_serialize<T, Archive, typename std::enable_if_t<std::is_class_v<T>>>
{
	static constexpr bool value = std::experimental::is_detected_v<serialize_function,T,Archive>;
};

template<typename T, typename S>
inline constexpr bool has_serialize_v = has_serialize<T, S>::value;

template<typename T, typename S>
struct is_serializable_type
{
	using value_type = std::remove_const_t<typename std::remove_reference_t<T>>;
	static constexpr bool value =
		std::experimental::is_detected_v<serialize_value_t,value_type> or
		has_serialize_v<value_type,S>;
};

template<typename T, typename S>
inline constexpr bool is_serializable_type_v = is_serializable_type<T,S>::value;

template<typename T>
inline constexpr bool is_type_with_value_serializer_v = std::experimental::is_detected_v<serialize_value_t,T>;

template<typename T, typename S, typename = void>
struct is_serializable_array_type : std::false_type {};

template<typename T, typename S>
struct is_serializable_array_type<T, S,
	std::enable_if_t<
		std::experimental::is_detected_v<value_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		not std::experimental::is_detected_v<std_string_npos_t, T>>>
{
	static constexpr bool value = is_serializable_type_v<typename T::value_type,S>;
};

template<typename T, typename S>
inline constexpr bool is_serializable_array_type_v = is_serializable_array_type<T,S>::value;

}
