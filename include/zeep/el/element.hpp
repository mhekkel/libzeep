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
#include <zeep/el/to_element.hpp>
#include <zeep/el/from_element.hpp>
#include <zeep/el/serializer.hpp>
#include <zeep/el/iterator.hpp>

namespace zeep
{
namespace el
{

class element
{
public:
	using value_type = detail::value_type;

	using nullptr_type = std::nullptr_t;
	using object_type = std::map<std::string,element>;
	using array_type = std::vector<element>;
	using string_type = std::string;
	using int_type = int64_t;
	using float_type = double;
	using boolean_type = bool;

	using pointer = element*;
	using const_pointer = const element*;

	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;

    using initializer_list_t = std::initializer_list<detail::element_reference>;

	using reference = element&;
	using const_reference = const element&;

	template<typename E> friend class detail::iterator_impl;
	using iterator = detail::iterator_impl<element>;
	using const_iterator = detail::iterator_impl<const element>;

	template<typename Iterator> using iteration_proxy = detail::iteration_proxy<Iterator>;
	template<typename iterator> friend class detail::iteration_proxy_value;

    template<value_type> friend struct detail::factory;

  private:

	union element_data
	{
		object_type*	m_object;
		array_type*		m_array;
		string_type*	m_string;
		int64_t			m_int;
		double			m_float;
		bool			m_boolean;

		element_data() = default;
		element_data(bool v) noexcept : m_boolean(v) {}
		element_data(int64_t v) noexcept : m_int(v) {}
		element_data(double v) noexcept : m_float(v) {}
		element_data(value_type t)
		{
			switch (t)
			{
				case value_type::array:			m_array = create<array_type>(); 	break;
				case value_type::boolean:		m_boolean = false;					break;
				case value_type::null:			m_object = nullptr;					break;
				case value_type::number_float:	m_float = 0;						break;
				case value_type::number_int:	m_int = 0;							break;
				case value_type::object:		m_object = create<object_type>(); 	break;
				case value_type::string:		m_string = create<string_type>();	break;
			}
		}
		element_data(const object_type& v)		{ m_object = create<object_type>(v); }
		element_data(object_type&& v)			{ m_object = create<object_type>(std::move(v)); }
		element_data(const string_type& v)		{ m_string = create<string_type>(v); }
		element_data(string_type&& v)			{ m_string = create<string_type>(std::move(v)); }
		element_data(const array_type& v)		{ m_array = create<array_type>(v); }
		element_data(array_type&& v)			{ m_array = create<array_type>(std::move(v)); }

		void destroy(value_type t) noexcept
		{
			switch (t)
			{
				case value_type::object:
				{
					std::allocator<object_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_object);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_object, 1);
					break;
				}

				case value_type::array:
				{
					std::allocator<array_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_array);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_array, 1);
					break;
				}

				case value_type::string:
				{
					std::allocator<string_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_string);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_string, 1);
					break;
				}

				default:
					break;
			}
		}
	};

	template<typename T, typename... Args>
	static T* create(Args&&... args)
	{
		// return new T(args...);
        std::allocator<T> alloc;
        using AllocatorTraits = std::allocator_traits<std::allocator<T>>;

        auto deleter = [&](T * object)
        {
            AllocatorTraits::deallocate(alloc, object, 1);
        };

        std::unique_ptr<T, decltype(deleter)> object(AllocatorTraits::allocate(alloc, 1), deleter);
        assert(object != nullptr);
        AllocatorTraits::construct(alloc, object.get(), std::forward<Args>(args)...);
        return object.release();
	}

  public:

	/// empty constructor with a certain type
	element(value_type t);

	/// default constructor
	element(std::nullptr_t = nullptr);
	element(const element& j);
	element(element&& j);
	element(const detail::element_reference& r);
	template<typename T,
		typename U = typename std::remove_cv<typename std::remove_reference<T>::type>::type,
		std::enable_if_t<not std::is_same<U,element>::value and detail::is_compatible_type<T>::value, int> = 0>
	element(T&& v) noexcept(noexcept(element_serializer<U,void>::to_element(std::declval<element&>(), std::forward<T>(v))))
	{
		element_serializer<U,void>::to_element(*this, std::forward<T>(v));
	}
 
	element(initializer_list_t init);
	element(size_t cnt, const element& v);

	element& operator=(element j) noexcept(
        std::is_nothrow_move_constructible<value_type>::value and
        std::is_nothrow_move_assignable<value_type>::value and
        std::is_nothrow_move_constructible<element_data>::value and
        std::is_nothrow_move_assignable<element_data>::value);

	~element() noexcept;

	constexpr bool is_null() const noexcept						{ return m_type == value_type::null; }
	constexpr bool is_object() const noexcept					{ return m_type == value_type::object; }
	constexpr bool is_array() const noexcept					{ return m_type == value_type::array; }
	constexpr bool is_string() const noexcept					{ return m_type == value_type::string; }
	constexpr bool is_number() const noexcept					{ return is_number_int() or is_number_float(); }
	constexpr bool is_number_int() const noexcept				{ return m_type == value_type::number_int; }
	constexpr bool is_number_float() const noexcept				{ return m_type == value_type::number_float; }
	constexpr bool is_true() const noexcept						{ return is_boolean() and m_data.m_boolean == true; }
	constexpr bool is_false() const noexcept					{ return is_boolean() and m_data.m_boolean == false; }
	constexpr bool is_boolean() const noexcept					{ return m_type == value_type::boolean; }

	constexpr value_type type() const							{ return m_type; }
	std::string type_name() const;

	explicit operator bool() const noexcept;

	// access to object elements
	reference at(const typename object_type::key_type& key);
	const_reference at(const typename object_type::key_type& key) const;

	reference operator[](const typename object_type::key_type& key);
	const_reference operator[](const typename object_type::key_type& key) const;

	// access to array elements
	reference at(size_t index);
	const_reference at(size_t index) const;

	reference operator[](size_t index);
	const_reference operator[](size_t index) const;

	iterator begin() noexcept									{ return iterator(this); }
	iterator end() noexcept										{ return iterator(this, -1); }

	const_iterator begin() const noexcept						{ return cbegin(); }
	const_iterator end() const noexcept							{ return cend(); }

	const_iterator cbegin() const noexcept						{ return const_iterator(this); }
	const_iterator cend() const noexcept						{ return const_iterator(this, -1); }

	reference front()											{ return *begin(); }
	const_reference front() const								{ return *cbegin(); }

	reference back()											{ auto tmp = end(); --tmp; return *tmp; }
	const_reference back() const								{ auto tmp = cend(); --tmp; return *tmp; }

  private:
	template<typename... Args>
	iterator insert_iterator(const_iterator pos, Args... args)
	{
		iterator result(this);
		auto insert_pos = std::distance(m_data.m_array->begin(), pos.m_it.m_array_it);
		m_data.m_array->insert(m_data.m_array->begin() + insert_pos, std::forward<Args>(args)...);
		result.m_it.m_array_it = m_data.m_array->begin() + insert_pos;

		return result;
	}
  public:

	void clear() noexcept;
	iterator insert(const_iterator pos, const element& val);
	iterator insert(const_iterator pos, element&& val)
	{
		return insert(pos, val);
	}
	iterator insert(const_iterator pos, size_type cnt, const element& val);
	iterator insert(const_iterator pos, const_iterator first, const_iterator last);
	iterator insert(const_iterator pos, initializer_list_t lst);
	void insert(const_iterator first, const_iterator last);

	void push_back(element&& val);
	void push_back(const element& val);

	template<typename... Args>
	std::pair<iterator,bool> emplace(Args&&... args)
	{
		if (is_null())
		{
			m_type = value_type::object;
			m_data = value_type::object;
		}
		else if (not is_object())
			throw std::runtime_error("Cannot emplace with json value of type " + type_name());
		
		validate();

		auto r = m_data.m_object->emplace(std::forward<Args>(args)...);
		auto i = begin();
		i.m_it.m_object_it = r.first;

		return { i, r.second };
	}

	template<typename... Args>
	void emplace_back(Args&&... args)
	{
		if (not (is_null() or is_array()))
			throw std::runtime_error("Cannot use emplace_back with " + type_name());
		
		if (is_null())
		{
			m_type = value_type::array;
			m_data = value_type::array;
		}

		m_data.m_array->emplace_back(std::forward<Args>(args)...);
	}

	template<typename Iterator,
		typename std::enable_if<std::is_same<Iterator, iterator>::value or std::is_same<Iterator, const_iterator>::value, int>::type = 0>
		Iterator erase(Iterator pos)
	{
		if (pos.m_obj != this)
			throw std::runtime_error("Invalid iterator");
		
		auto result = end();

		switch (m_type)
		{
			case value_type::array:
				result.m_it.m_array_it = m_data.m_array->erase(pos.m_it.m_array_it);
				break;

			case value_type::object:
				result.m_it.m_object_it = m_data.m_object->erase(pos.m_it.m_object_it);
				break;

			case value_type::null:
				throw std::runtime_error("Cannot erase in null values");

			default:
				if (pos.m_it.m_p != 0)
					throw std::runtime_error("Iterator out of range");

				if (m_type == value_type::string)
				{
					std::allocator<string_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_string);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_string, 1);
					m_data.m_string = nullptr;
				}

				m_type = value_type::null;
				break;
		}

		return result;
	}

	template<typename Iterator,
		typename std::enable_if<std::is_same<Iterator, iterator>::value or std::is_same<Iterator, const_iterator>::value, int>::type = 0>
		Iterator erase(Iterator first, Iterator last)
	{
		if (first.m_obj != this or last.m_obj != this)
			throw std::runtime_error("Invalid iterator");
		
		auto result = end();

		switch (m_type)
		{
			case value_type::array:
				result.m_it.m_array_it = m_data.m_array->erase(first.m_it.m_array_it, last.m_it.m_array_it);
				break;

			case value_type::object:
				result.m_it.m_object_it = m_data.m_object->erase(first.m_it.m_object_it, last.m_it.m_object_it);
				break;

			case value_type::null:
				throw std::runtime_error("Cannot erase in null values");

			default:
				if (first.m_it.m_p != 0 or last.m_it.m_p != 0)
					throw std::runtime_error("Iterator out of range");

				if (m_type == value_type::string)
				{
					std::allocator<string_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_string);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_string, 1);
					m_data.m_string = nullptr;
				}

				m_type = value_type::null;
				break;
		}

		return result;
	}

	size_type erase(const typename object_type::key_type& key)
	{
		if (is_object())
			return m_data.m_object->erase(key);
		throw std::runtime_error("Cannot use erase() with " + type_name());
	}

	void erase(const size_type index)
	{
		if (is_array())
		{
			if (index >= size())
				throw std::runtime_error("Index out of range");
			m_data.m_array->erase(m_data.m_array->begin() + static_cast<difference_type>(index));
		}
		else
			throw std::runtime_error("Cannot use erase() with " + type_name());
	}

	void swap(reference other) noexcept (
        std::is_nothrow_move_constructible<value_type>::value and
        std::is_nothrow_move_assignable<value_type>::value and
        std::is_nothrow_move_constructible<element_data>::value and
        std::is_nothrow_move_assignable<element_data>::value
    )
    {
        std::swap(m_type, other.m_type);
        std::swap(m_data, other.m_data);
        validate();
    }

    // template<typename T>
    // const_iterator find(T&& key) const
    // {
    //     auto result = cend();

    //     if (is_object())
    //     {
    //         result.m_it.m_object_it = m_data.m_object->find(std::forward<T>(key));
    //     }
	// 	else if (is_array())
	// 	{
	// 		result.m_it.m_array_it = std::find(m_data.m_array->begin(), m_data.m_array->end(), std::forward<T>(key));
	// 	}

    //     return result;
    // }

	bool contains(element test) const;

	// TODO: no reverse iterators yet

	iteration_proxy<iterator> items() noexcept
	{
		return iteration_proxy<iterator>(*this);
	}

	iteration_proxy<const_iterator> items() const noexcept
	{
		return iteration_proxy<const_iterator>(*this);
	}

	bool empty() const;
	size_t size() const;
	size_t max_size() const noexcept;

	friend bool operator==(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator==(const_reference& lhs, const T& rhs)
	{
		return lhs == element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator==(const T& lhs, const_reference& rhs)
	{
		return element(lhs) == rhs;
	}

	friend bool operator!=(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator!=(const_reference& lhs, const T& rhs)
	{
		return lhs != element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator!=(const T& rhs, const_reference& lhs)
	{
		return element(lhs) == rhs;
	}

	friend bool operator<(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator<(const_reference& lhs, const T& rhs)
	{
		return lhs < element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator<(const T& lhs, const_reference& rhs)
	{
		return element(lhs) < rhs;
	}

	friend bool operator<=(const_reference& lhs, const_reference& rhs)
	{
		return not (rhs < lhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator<=(const_reference& lhs, const T& rhs)
	{
		return lhs <= element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator<=(const T& lhs, const_reference& rhs)
	{
		return element(lhs) <= rhs;
	}

	friend bool operator>(const_reference& lhs, const_reference& rhs)
	{
		return not (lhs <= rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator>(const_reference& lhs, const T& rhs)
	{
		return lhs > element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator>(const T& lhs, const_reference& rhs)
	{
		return element(lhs) > rhs;
	}

	friend bool operator>=(const_reference& lhs, const_reference& rhs)
	{
		return not (lhs < rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator>=(const_reference& lhs, const T& rhs)
	{
		return lhs >= element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend bool operator>=(const T& lhs, const_reference& rhs)
	{
		return element(lhs) >= rhs;
	}

	// arithmetic operators

	element& operator-();

	friend element operator+(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator+(const_reference& lhs, const T& rhs)
	{
		return lhs + element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator+(const T& lhs, const_reference& rhs)
	{
		return element(lhs) + rhs;
	}

	friend element operator-(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator-(const_reference& lhs, const T& rhs)
	{
		return lhs - element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator-(const T& lhs, const_reference& rhs)
	{
		return element(lhs) - rhs;
	}

	friend element operator*(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator*(const_reference& lhs, const T& rhs)
	{
		return lhs * element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator*(const T& lhs, const_reference& rhs)
	{
		return element(lhs) * rhs;
	}

	friend element operator/(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator/(const_reference& lhs, const T& rhs)
	{
		return lhs / element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator/(const T& lhs, const_reference& rhs)
	{
		return element(lhs) / rhs;
	}

	friend element operator%(const_reference& lhs, const_reference& rhs);

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator%(const_reference& lhs, const T& rhs)
	{
		return lhs % element(rhs);
	}

	template<typename T, std::enable_if_t<std::is_scalar<T>::value, int> = 0>
	friend element operator%(const T& lhs, const_reference& rhs)
	{
		return element(lhs) % rhs;
	}

private:

	// get_impl_ptr
	object_type* get_impl_ptr(object_type*) noexcept								{ return is_object() ? m_data.m_object : nullptr; }
	constexpr const object_type* get_impl_ptr(const object_type*) const noexcept	{ return is_object() ? m_data.m_object : nullptr; }
	array_type* get_impl_ptr(array_type*) noexcept									{ return is_array() ? m_data.m_array : nullptr; }
	constexpr const array_type* get_impl_ptr(const array_type*) const noexcept		{ return is_array() ? m_data.m_array : nullptr; }
	string_type* get_impl_ptr(string_type*) noexcept								{ return is_string() ? m_data.m_string : nullptr; }
	constexpr const string_type* get_impl_ptr(const string_type*) const noexcept	{ return is_string() ? m_data.m_string : nullptr; }
	int_type* get_impl_ptr(int_type*) noexcept										{ return is_number_int() ? &m_data.m_int : nullptr; }
	constexpr const int_type* get_impl_ptr(const int_type*) const noexcept			{ return is_number_int() ? &m_data.m_int : nullptr; }
	float_type* get_impl_ptr(float_type*) noexcept									{ return is_number_float() ? &m_data.m_float : nullptr; }
	constexpr const float_type* get_impl_ptr(const float_type*) const noexcept		{ return is_number_float() ? &m_data.m_float : nullptr; }
	boolean_type* get_impl_ptr(boolean_type*) noexcept								{ return is_boolean() ? &m_data.m_boolean : nullptr; }
	constexpr const boolean_type* get_impl_ptr(const boolean_type*) const noexcept	{ return is_boolean() ? &m_data.m_boolean : nullptr; }


public:

	// access to data
	// these return a pointer to the internal storage
	template<typename P, typename std::enable_if<std::is_pointer<P>::value, int>::type = 0>
	auto get_ptr() noexcept -> decltype(std::declval<element&>().get_impl_ptr(std::declval<P>()))
	{
		return get_impl_ptr(static_cast<P>(nullptr));
	}

	template<typename P, typename std::enable_if<std::is_pointer<P>::value and std::is_const<typename std::remove_pointer<P>::type>::value, int>::type = 0>
	constexpr auto get_ptr() const noexcept -> decltype(std::declval<element&>().get_impl_ptr(std::declval<P>()))
	{
		return get_impl_ptr(static_cast<P>(nullptr));
	}

	template<typename T,
		typename U = typename std::remove_cv<typename std::remove_reference<T>::type>::type,
		std::enable_if_t<detail::is_compatible_type<T>::value, int> = 0>
	T as() const noexcept(noexcept(element_serializer<U>::from_element(std::declval<const element&>(), std::declval<U&>())))
	{
		static_assert(std::is_default_constructible<U>::value, "Type must be default constructible to use with get()");

		U ret = {};
		if (not is_null())
			element_serializer<U>::from_element(*this, ret);
		return ret;
	}

	friend std::ostream& operator<<(std::ostream& os, const element& v);
	friend void serialize(std::ostream& os, const element& data);
	// friend void serialize(std::ostream& os, const element& data, int indent, int level = 0);

private:

	void validate() const
	{
        assert(m_type != value_type::object or m_data.m_object != nullptr);
        assert(m_type != value_type::array or m_data.m_array != nullptr);
        assert(m_type != value_type::string or m_data.m_string != nullptr);
	}

	value_type		m_type;
	element_data	m_data;
};

template<>
std::string element::as<std::string>() const;

template<>
bool element::as<bool>() const;

namespace detail
{

class element_reference
{
public:
	element_reference(element&& value)
		: m_owned(std::move(value)), m_reference(&m_owned), m_rvalue(true) {}

	element_reference(const element& value)
		: m_reference(const_cast<element*>(&value)), m_rvalue(false) {}
	
	element_reference(std::initializer_list<element_reference> init)
		: m_owned(init), m_reference(&m_owned), m_rvalue(true) {}
	
    template <typename... Args, std::enable_if_t<std::is_constructible<element, Args...>::value, int> = 0>
    element_reference(Args&& ... args)
        : m_owned(std::forward<Args>(args)...), m_reference(&m_owned), m_rvalue(true) {}

    // class should be movable only
    element_reference(element_reference&&) = default;
    element_reference(const element_reference&) = delete;
    element_reference& operator=(const element_reference&) = delete;
    element_reference& operator=(element_reference&&) = delete;
    ~element_reference() = default;

    element data() const
    {
        if (m_rvalue)
            return std::move(*m_reference);
        return *m_reference;
    }

    element const& operator*() const
    {
        return *static_cast<element const*>(m_reference);
    }

    element const* operator->() const
    {
        return static_cast<element const*>(m_reference);
    }

private:
	element m_owned;
	element* m_reference = nullptr;
	bool m_rvalue;
};

} // detail
} // el
} // namespace zeep
