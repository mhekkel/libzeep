//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// generic iterator classes used by zeep::json::element

#include <zeep/config.hpp>

#include <cassert>

namespace zeep::json::detail
{

template <typename Iterator>
class iteration_proxy;
template <typename Iterator>
class iteration_proxy_value;

// First a base for the iterator, both iterator and const iterator use this.

template <typename E>
class iterator_impl
{
  public:
	friend iterator_impl<typename std::conditional_t<std::is_const_v<E>, typename std::remove_const_t<E>, const E>>;
	friend E;
	friend iteration_proxy<iterator_impl>;
	friend iteration_proxy_value<iterator_impl>;

	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = typename E::value_type;
	using difference_type = typename E::difference_type;
	using pointer = typename std::conditional_t<std::is_const_v<E>, typename E::const_pointer, typename E::pointer>;
	using reference = typename std::conditional_t<std::is_const_v<E>, typename E::const_reference, typename E::reference>;

	using object_type = typename E::object_type;
	using array_type = typename E::array_type;

	static_assert(is_element_v<typename std::remove_const_t<E>>, "The iterator_impl template only accepts element classes");

  public:
	iterator_impl() = default;
	~iterator_impl() {}

	explicit iterator_impl(pointer obj) noexcept
		: m_obj(obj)
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: m_it.m_array_it = m_obj->m_data.m_array->begin(); break;
			case value_type::object: m_it.m_object_it = m_obj->m_data.m_object->begin(); break;
			default: m_it.m_p = 0; break;
		}
	}
	iterator_impl(pointer obj, int) noexcept
		: m_obj(obj)
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: m_it.m_array_it = m_obj->m_data.m_array->end(); break;
			case value_type::object: m_it.m_object_it = m_obj->m_data.m_object->end(); break;
			case value_type::null: m_it.m_p = 0; break;
			default: m_it.m_p = 1; break;
		}
	}
	iterator_impl(const iterator_impl &i)
		: m_obj(i.m_obj)
		, m_it(i.m_it)
	{
	}

	iterator_impl operator--(int)
	{
		auto result(*this);
		operator--();
		return result;
	}

	iterator_impl &operator--()
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: std::advance(m_it.m_array_it, -1); break;
			case value_type::object: std::advance(m_it.m_object_it, -1); break;
			default: --m_it.m_p; break;
		}
		return *this;
	}

	iterator_impl operator++(int)
	{
		auto result(*this);
		operator++();
		return result;
	}

	iterator_impl &operator++()
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: std::advance(m_it.m_array_it, +1); break;
			case value_type::object: std::advance(m_it.m_object_it, +1); break;
			default: ++m_it.m_p; break;
		}
		return *this;
	}

	reference operator*() const
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array:
				assert(m_it.m_array_it != m_obj->m_data.m_array->end());
				return *m_it.m_array_it;
				break;

			case value_type::object:
				assert(m_it.m_object_it != m_obj->m_data.m_object->end());
				return m_it.m_object_it->second;
				break;

			case value_type::null:
				throw std::runtime_error("Cannot get value");

			default:
				if (m_it.m_p == 0)
					return *m_obj;
				throw std::runtime_error("Cannot get value");
		}
	}

	pointer operator->() const
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array:
				assert(m_it.m_array_it != m_obj->m_data.m_array->end());
				return &(*m_it.m_array_it);
				break;

			case value_type::object:
				assert(m_it.m_object_it != m_obj->m_data.m_object->end());
				return &(m_it.m_object_it->second);
				break;

			case value_type::null:
				throw std::runtime_error("Cannot get value");

			default:
				if (m_it.m_p == 0)
					return m_obj;
				throw std::runtime_error("Cannot get value");
		}
	}

	bool operator==(const iterator_impl &other) const
	{
		if (m_obj != other.m_obj)
			throw std::runtime_error("Containers are not the same");

		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: return m_it.m_array_it == other.m_it.m_array_it;
			case value_type::object: return m_it.m_object_it == other.m_it.m_object_it;
			default: return m_it.m_p == other.m_it.m_p;
		}
	}

	bool operator!=(const iterator_impl &other) const
	{
		return not operator==(other);
	}

	bool operator<(const iterator_impl &other) const
	{
		if (m_obj != other.m_obj)
			throw std::runtime_error("Containers are not the same");

		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: return m_it.m_array_it < other.m_it.m_array_it;
			case value_type::object: throw std::runtime_error("Cannot compare order of object iterators");
			default: return m_it.m_p < other.m_it.m_p;
		}
	}

	bool operator<=(const iterator_impl &other) const
	{
		return not other.operator<(*this);
	}

	bool operator>(const iterator_impl &other) const
	{
		return not operator<=(other);
	}

	bool operator>=(const iterator_impl &other) const
	{
		return not operator<(other);
	}

	iterator_impl &operator+=(difference_type i)
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: std::advance(m_it.m_array_it, i);
			case value_type::object: throw std::runtime_error("Cannot use offsets with object iterators");
			default: m_it.m_p += i;
		}
		return *this;
	}

	iterator_impl &operator-=(difference_type i)
	{
		operator+=(-i);
		return *this;
	}

	iterator_impl operator+(difference_type i) const
	{
		auto result = *this;
		result += i;
		return result;
	}

	friend iterator_impl operator+(difference_type i, const iterator_impl &iter)
	{
		auto result = iter;
		result += i;
		return result;
	}

	iterator_impl operator-(difference_type i) const
	{
		auto result = *this;
		result -= i;
		return result;
	}

	friend iterator_impl operator-(difference_type i, const iterator_impl &iter)
	{
		auto result = iter;
		result -= i;
		return result;
	}

	difference_type operator-(const iterator_impl &other) const
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: return m_it.m_array_it - other.m_it.m_array_it;
			case value_type::object: throw std::runtime_error("Cannot use offsets with object iterators");
			default: return m_it.m_p - other.m_it.m_p;
		}
	}

	reference operator[](difference_type i) const
	{
		assert(m_obj);
		switch (m_obj->m_type)
		{
			case value_type::array: *std::next(m_it.m_array_it, i);
			case value_type::object: throw std::runtime_error("Cannot use offsets with object iterators");
			default:
				if (m_it.m_p == -i)
					return *m_obj;
				throw std::runtime_error("Cannot get value");
		}
	}

	const typename object_type::key_type &key() const
	{
		assert(m_obj);

		if (not m_obj->is_object())
			throw std::runtime_error("Can only use key() on object iterators");

		return m_it.m_object_it->first;
	}

	reference value() const
	{
		return operator*();
	}

  private:
	pointer m_obj = nullptr;

	// Ahw... MSVC does not allow a union here...
	// union

	struct
	{
		typename E::array_type::iterator m_array_it;
		typename E::object_type::iterator m_object_it;
		difference_type m_p;
	} m_it = {};
};

// iterator_proxy_value is used for range based for loops, so you can still get key/value

template <typename Iterator>
class iteration_proxy_value
{
  public:
	using difference_type = std::ptrdiff_t;
	using value_type = iteration_proxy_value;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::input_iterator_tag;

  public:
	explicit iteration_proxy_value(Iterator iter) noexcept
		: m_anchor(iter)
	{
	}

	iteration_proxy_value &operator*()
	{
		return *this;
	}

	iteration_proxy_value &operator++()
	{
		++m_anchor;
		++m_index;
		return *this;
	}

	bool operator==(const iteration_proxy_value &other) const
	{
		return m_anchor == other.m_anchor;
	}

	bool operator!=(const iteration_proxy_value &other) const
	{
		return m_anchor != other.m_anchor;
	}

	const std::string &key() const
	{
		assert(m_anchor.m_obj != nullptr);
		switch (m_anchor.m_obj->m_type)
		{
			case ::zeep::json::detail::value_type::array:
				if (m_index != m_index_last)
				{
					m_index_str = std::to_string(m_index);
					m_index_last = m_index;
				}
				return m_index_str;
				break;

			case ::zeep::json::detail::value_type::object:
				return m_anchor.key();

			default:
				return k_empty;
		}
	}

	typename Iterator::reference value() const
	{
		return m_anchor.value();
	}

	// support for structured binding
	template <size_t N>
	decltype(auto) get() const
	{
		if constexpr (N == 0)
			return key();
		else if constexpr (N == 1)
			return value();
	}

  private:
	Iterator m_anchor;
	size_t m_index = 0;
	mutable size_t m_index_last = 0;
	mutable std::string m_index_str = "0";
	const std::string k_empty;
};

template <typename Iterator>
class iteration_proxy
{
  private:
	typename Iterator::reference m_container;

  public:
	explicit iteration_proxy(typename Iterator::reference cont) noexcept
		: m_container(cont)
	{
	}

	iteration_proxy_value<Iterator> begin() noexcept
	{
		return iteration_proxy_value<Iterator>(m_container.begin());
	}

	iteration_proxy_value<Iterator> end() noexcept
	{
		return iteration_proxy_value<Iterator>(m_container.end());
	}
};

} // namespace zeep::json::detail

namespace std
{
template <typename IteratorType>
struct tuple_size<::zeep::json::detail::iteration_proxy_value<IteratorType>>
	: public std::integral_constant<std::size_t, 2>
{
};

template <typename IteratorType>
struct tuple_element<0, ::zeep::json::detail::iteration_proxy_value<IteratorType>>
{
	using proxy_type = typename ::zeep::json::detail::iteration_proxy_value<IteratorType>;
	using type = decltype(std::declval<proxy_type>().key());
};

template <typename IteratorType>
struct tuple_element<1, ::zeep::json::detail::iteration_proxy_value<IteratorType>>
{
	using proxy_type = typename ::zeep::json::detail::iteration_proxy_value<IteratorType>;
	using type = decltype(std::declval<proxy_type>().value());
};

// template <std::size_t N, typename IteratorType>
// struct tuple_element<N, ::zeep::json::detail::iteration_proxy_value<IteratorType >>
// {
//   public:
// 	using proxy_type = typename ::zeep::json::detail::iteration_proxy_value<IteratorType>;
//     using type = decltype(std::declval<proxy_type>().get<N>());
// };

} // namespace std