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
#include <optional>

#include <zeep/serialize.hpp>
#include <zeep/type_traits.hpp>

#include <zeep/el/element_fwd.hpp>
#include <zeep/el/factory.hpp>
#include <zeep/el/to_element.hpp>
#include <zeep/el/from_element.hpp>
#include <zeep/el/traits.hpp>

namespace zeep::el
{

template<typename T, typename Archive, typename = void>
struct is_serializable_map_type : std::false_type {};

template<typename T, typename Archive>
struct is_serializable_map_type<T, Archive,
	std::enable_if_t<
		std::experimental::is_detected<detail::mapped_type_t, T>::value and
		std::experimental::is_detected<detail::key_type_t, T>::value and
		std::experimental::is_detected<detail::iterator_t, T>::value and
		not detail::is_compatible_string_type<typename Archive::element_type,T>::value>>
{
	static constexpr bool value =
		std::is_same<typename T::key_type, std::string>::value and
		(
			detail::is_compatible_type<typename T::mapped_type>::value or
		has_serialize<typename T::mapped_type, Archive>::value
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
		std::experimental::is_detected<detail::value_type_t, T>::value and
		std::is_same<has_value_or_result<T>,typename T::value_type>::value and
		not detail::is_compatible_string_type<typename Archive::element_type,T>::value>>
{
	static constexpr bool value =
		detail::is_compatible_type<typename T::value_type>::value or
		has_serialize<typename T::value_type, Archive>::value;
};

template<typename T, typename Archive>
inline constexpr bool is_serializable_optional_type_v = is_serializable_optional_type<T, Archive>::value;

template<typename E>
struct serializer
{
	using element_type = E;

	template<typename T, typename = void>
	struct serializer_impl {};

	template<typename T>
	struct serializer_impl<T, std::enable_if_t<detail::is_compatible_type_v<T>>>
	{
		static void serialize(const T& data, element_type& e)
		{
			e = data;
		}
	};

	template<typename T>
	struct serializer_impl<T, std::enable_if_t<not detail::is_compatible_type_v<T> and is_type_with_value_serializer_v<T,serializer>>>
	{
		using value_serializer = zeep::value_serializer<T>;

		static void serialize(const T& data, element_type& e)
		{
			e = value_serializer::to_string(data);
		}
	};

	template<typename T>
	struct serializer_impl<T, std::enable_if_t<has_serialize_v<T,serializer>>>
	{
		static void serialize(const T& data, element_type& e)
		{
			serializer sr;
			const_cast<T&>(data).serialize(sr, 0);
			e.swap(sr.m_elem);
		}
	};

	template<typename T>
	struct serializer_impl<T, std::enable_if_t<
		detail::is_compatible_type_v<T> and
		not is_type_with_value_serializer_v<T,serializer> and
		not is_serializable_array_type_v<T,serializer> and
		not is_serializable_map_type_v<T,serializer>>>
	{
		static void serialize(const T& data, element_type& e)
		{
			e = data;
		}
	};

	template<typename T>
	struct serializer_impl<T, std::enable_if_t<is_serializable_array_type_v<T,serializer>>>
	{
		static void serialize(const T& data, element_type& e)
		{
			using serializer_impl = serializer_impl<typename T::value_type>;

			e = element_type::value_type::array;

			for (auto& i: data)
			{
				element_type ei;
				serializer_impl::serialize(i, ei);
				e.push_back(ei);
			}
		}
	};

	template<typename T>
	struct serializer_impl<T, std::enable_if_t<is_serializable_map_type_v<T,serializer>>>
	{
		static void serialize(const T& data, element_type& e)
		{
			using serializer_impl = serializer_impl<typename T::mapped_type>;

			e = element_type::value_type::object;

			for (auto& i: data)
			{
				element_type ei;
				serializer_impl::serialize(i.second, ei);
				e.emplace(i.first, ei);
			}
		}
	};

	template<typename T>
	struct serializer_impl<T, std::enable_if_t<is_serializable_optional_type_v<T,serializer>>>
	{
		static void serialize(const T& data, element_type& e)
		{
			using serializer_impl = serializer_impl<typename T::value_type>;

			if (data)
				serializer_impl::serialize(*data, e);
		}
	};

	serializer() {}

	template<typename T>
	serializer& operator&(name_value_pair<T>&& nvp)
	{
		serialize(nvp.name(), nvp.value());
		return *this;
	}

	template<typename T>
	void serialize(const char* name, T& data)
	{
		// element_type j;
		// zeep::el::detail::to_element(j, data);
		// m_elem.emplace(std::make_pair(name, std::move(j)));

		using serializer_impl = serializer_impl<T>;

		element_type e;
		serializer_impl::serialize(data, e);
		m_elem.emplace(std::make_pair(name, e));
	}

	template<typename T>
	static void serialize(E& e, const T& v)
	{
		using serializer_impl = serializer_impl<T>;
		serializer_impl::serialize(v, e);
	}

	element_type	m_elem;
};

template<typename E>
struct deserializer
{
	using element_type = E;

	template<typename T, typename = void>
	struct deserializer_impl {};

	template<typename T>
	struct deserializer_impl<T, std::enable_if_t<is_type_with_value_serializer_v<T,serializer>>>
	{
		using value_serializer = zeep::value_serializer<T>;

		static void deserialize(T& data, const element_type& e)
		{
			data = value_serializer::from_string(e);
		}
	};

	template<typename T>
	struct deserializer_impl<T, std::enable_if_t<has_serialize<T,deserializer>::value>>
	{
		static void deserialize(T& data, const element_type& e)
		{
			deserializer sr(e);
			data.serialize(sr, 0);
		}
	};

	template<typename T>
	struct deserializer_impl<T, std::enable_if_t<
		detail::is_compatible_type<T>::value and
		not is_serializable_array_type<T,deserializer>::value and
		not is_serializable_map_type<T,deserializer>::value>>
	{
		static void deserialize(T& data, const element_type& e)
		{
			data = e.template as<T>();
		}
	};

	template<typename T>
	struct deserializer_impl<T, std::enable_if_t<is_serializable_array_type<T,deserializer>::value>>
	{
		static void deserialize(T& data, const element_type& e)
		{
			using deserializer_impl = deserializer_impl<typename T::value_type>;

			data.clear();

			for (auto& i: e)
			{
				typename T::value_type v;
				
				deserializer_impl::deserialize(v, i);

				data.push_back(v);
			}
		}
	};

	template<typename T>
	struct deserializer_impl<T, std::enable_if_t<is_serializable_map_type<T,deserializer>::value>>
	{
		static void deserialize(T& data, const element_type& e)
		{
			using deserializer_impl = deserializer_impl<typename T::mapped_type>;

			data.clear();

			for (auto& i: e.items())
			{
				typename T::mapped_type v;
				
				deserializer_impl::deserialize(v, i.value());

				data[i.key()] = v;
			}
		}
	};

	template<typename T>
	struct deserializer_impl<T, std::enable_if_t<is_serializable_optional_type<T,deserializer>::value>>
	{
		static void deserialize(T& data, element_type& e)
		{
			using deserializer_impl = deserializer_impl<typename T::value_type>;

			typename T::value_type v;
			deserializer_impl::deserialize(v, e);
			data.reset(v);
		}
	};

	deserializer(const element_type& elem) : m_elem(elem) {}

	template<typename T>
	deserializer& operator&(name_value_pair<T> nvp)
	{
		deserialize(nvp.name(), nvp.value());
		return *this;
	}

	template<typename T>
	void deserialize(const char* name, T& data)
	{
		if (not m_elem.is_object() or m_elem.empty())
			return;

		using deserializer_impl = deserializer_impl<T>;

		auto value = m_elem[name];

		if (value.is_null())
			return;

		deserializer_impl::deserialize(data, value);
	}

	template<typename T>
	static void deserialize(const E& e, T& v)
	{
		using deserializer_impl = deserializer_impl<T>;
		deserializer_impl::deserialize(v, e);
	}

	const element_type&	m_elem;
};

template<typename J, typename T>
void to_element(J& e, T& v)
{
	serializer<J>::serialize(e, v);
}

template<typename J, typename T>
void from_element(const J& e, T& v)
{
	deserializer<typename std::remove_cv<J>::type>::deserialize(e, v);
}

namespace detail
{

struct to_element_fn
{
	template<typename T>
	auto operator()(element& j, T&& val) const noexcept(noexcept(to_element(j, std::forward<T>(val))))
	-> decltype(to_element(j, std::forward<T>(val)), void())
	{
		return to_element(j, std::forward<T>(val));
	}
};

namespace
{
	constexpr const auto& to_element = typename ::zeep::el::detail::to_element_fn{};
}

}

template<typename,typename = void>
struct element_serializer
{
	template<typename T>
	static auto to_element(element& j, T&& v)
		noexcept(noexcept(::zeep::el::detail::to_element(j, std::forward<T>(v))))
		-> decltype(::zeep::el::detail::to_element(j, std::forward<T>(v)))
	{
		::zeep::el::detail::to_element(j, std::forward<T>(v));
	}

	template<typename T>
	static auto from_element(const element& j, T& v)
		noexcept(noexcept(::zeep::el::detail::from_element(j, v)))
		-> decltype(::zeep::el::detail::from_element(j, v))
	{
		::zeep::el::detail::from_element(j, v);
	}

};

}
