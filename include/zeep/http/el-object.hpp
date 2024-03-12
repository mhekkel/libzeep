/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Maarten L. Hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace zeep::http
{

class object;

// namespace detail
// {

// 	enum class value_type
// 	{
// 		null,
// 		object,
// 		array,
// 		string,
// 		number_int,
// 		number_float,
// 		boolean
// 	};

// 	inline bool operator<(value_type lhs, value_type rhs) noexcept
// 	{
// 		static constexpr std::array<std::uint8_t, 7> order{
// 			0, // null
// 			3, // object
// 			4, // array
// 			5, // string
// 			2, // number_int
// 			2, // number_float
// 			1  // boolean
// 		};

// 		const auto lix = static_cast<std::size_t>(lhs);
// 		const auto rix = static_cast<std::size_t>(rhs);
// 		return lix < order.size() and rix < order.size() and order[lix] < order[rix];
// 	}

// 	class object_reference;
// } // namespace detail

// // -
// template <typename T>
// struct ObjectSerializer
// {
// 	template <typename Arg = T>
// 	static void to_object(object &o, Arg &&v); /* noexcept */
// };

// // --------------------------------------------------------------------
// // type traits

// template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
// struct detector
// {
// 	using value_t = std::false_type;
// 	using type = Default;
// };

// template <class Default, template <class...> class Op, class... Args>
// struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
// {
// 	using value_t = std::true_type;
// 	using type = Op<Args...>;
// };

// struct nope
// {
// };

// template <template <class...> class Op, class... Args>
// using is_detected = typename detector<nope, void, Op, Args...>::value_t;

// template <template <class...> class Op, class... Args>
// constexpr inline bool is_detected_v = is_detected<Op, Args...>::value;

// // template<class Expected, template<class...> class Op, class... Args>
// // using is_detected_exact = std::is_same<Expected, detector<Op, Args...>>;

// template <template <class...> class Op, class... Args>
// constexpr inline bool is_detected_exact_v = std::experimental::is_detected_exact<Op, Args...>::value;

// template <typename T, typename Args...>
// using to_object_function = decltype(T::to_object(std::declval<Args>()...));

// // template <typename O, typename T>
// // using from_object_function = decltype(from_object(std::declval(const O &), std::declval(T &)));

// template <typename T, typename = void>
// struct has_to_object : std::false_type
// {
// };

// template <typename T>
// 	requires(not std::is_same_v<T, object>)
// struct has_to_object<T>
// {
// 	static constexpr bool value = is_detected_v<to_object_function, T>;
// };

// template <typename T>
// inline constexpr bool has_to_object_v = has_to_object<T>::value;

// template <typename T>
// struct is_convertible_type
// {
// 	static constexpr bool value = has_to_object_v<T>;
// };

// template <typename T>
// constexpr inline bool is_convertible_type_v = is_convertible_type<T>::value;

// template <typename T>
// using value_type_t = typename T::value_type;

// template <typename T>
// using iterator_t = typename T::iterator;

// template <typename T>
// struct is_compatible_array_type : std::false_type
// {
// };

// template <typename T>
// 	requires(
// 		has_to_object_v<typename T::value_type> and
// 		is_detected_v<iterator_t, T>)
// struct is_compatible_array_type<T> : std::true_type
// {
// };

// template <typename T>
// inline constexpr bool is_compatible_array_type_v = is_compatible_array_type<T>::value;

// // concepts

template <typename T>
concept ObjectType = std::is_same_v<object, std::remove_cvref_t<T>>;

template <typename T>
concept ScalarType = std::is_scalar_v<T>;

template <typename T>
concept StringType = std::is_assignable_v<std::string, T>;

template <typename T>
concept ConvertibleType = (std::is_same_v<object, std::remove_cvref_t<T>> or
						   std::is_scalar_v<std::remove_cvref_t<T>> or
						   std::is_same_v<bool, T> or
						   std::is_assignable_v<std::string, T>);

// // --------------------------------------------------------------------

// namespace detail
// {
// 	// Factory class to construct objects
// 	template <value_type>
// 	struct factory;

// 	template <typename T>
// 		requires std::is_same_v<T, bool>
// 	void to_object(object &v, T b);

// 	template <ConvertibleType T, size_t N>
// 	void to_object(object &o, const T (&arr)[N]);

// 	template <StringType T>
// 	void to_object(object &o, const T &s);

// 	template <typename T>
// 		requires std::is_floating_point_v<T>
// 	void to_object(object &o, T f);

// 	template <typename T>
// 		requires std::is_integral_v<T>
// 	void to_object(object &o, T i);

// 	template <typename T>
// 		requires std::is_same_v<T, bool>
// 	void to_object(object &o, const std::vector<T> &v);

// 	template <typename T>
// 		requires(
// 			is_compatible_array_type_v<T> /*  and
// 	         not is_compatible_object_type_v<object, T> and
// 	         not is_compatible_string_type_v<object, T> and
// 	         not is_object_v<T> */
// 			)
// 	void to_object(object &o, const T &arr);

// 	template <ConvertibleType T>
// 	void to_object(object &o, const std::valarray<T> &arr);

// 	template <typename T>
// 		requires std::is_same_v<T, std::vector<object>>
// 	void to_object(object &o, const T &arr);

// 	template <ConvertibleType T, size_t N>
// 	void to_object(object &o, const T (&arr)[N]);

// 	template <ConvertibleType T>
// 	void to_object(object &o, const std::optional<T> &v);

// } // namespace detail

// --------------------------------------------------------------------

class object
{
  public:
	enum class value_type
	{
		null,
		object,
		array,
		string,
		number_int,
		number_float,
		boolean
	};

	inline constexpr friend bool operator<(value_type lhs, value_type rhs) noexcept
	{
		const uint8_t order[] = {
			0, // null
			3, // object
			4, // array
			5, // string
			2, // number_int
			2, // number_float
			1  // boolean
		};

		const auto lix = static_cast<std::size_t>(lhs);
		const auto rix = static_cast<std::size_t>(rhs);
		return lix < sizeof(order) and rix < sizeof(order) and order[lix] < order[rix];
	}

	using nullptr_type = std::nullptr_t;
	using object_type = std::map<std::string, object>;
	using array_type = std::vector<object>;
	using string_type = std::string;
	using int_type = int64_t;
	using float_type = double;
	using boolean_type = bool;

	using pointer = object *;
	using const_pointer = const object *;

	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;

	// using initializer_list_t = std::initializer_list<detail::object_reference>;

	using reference = object &;
	using const_reference = const object &;

	// --------------------------------------------------------------------

	template <ObjectType T>
	struct iterator_impl
	{
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = T::difference_type;
		using pointer = typename std::conditional_t<std::is_const_v<T>, typename T::const_pointer, typename T::pointer>;
		using reference = typename std::conditional_t<std::is_const_v<T>, typename T::const_reference, typename T::reference>;

		iterator_impl() = default;

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

		auto operator<=>(const iterator_impl &other) const
		{
			if (m_obj != other.m_obj)
				throw std::runtime_error("Containers are not the same");

			assert(m_obj);
			switch (m_obj->m_type)
			{
				case value_type::array: return m_it.m_array_it <=> other.m_it.m_array_it;
				case value_type::object: throw std::runtime_error("Cannot compare order of object iterators");
				default: return m_it.m_p <=> other.m_it.m_p;
			}
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

		const std::string &key() const
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

		using array_iterator_type = typename T::array_type::iterator;
		using object_iterator_type = typename T::object_type::iterator;

		union
		{
			array_iterator_type m_array_it;
			object_iterator_type m_object_it;
			difference_type m_p;
		} m_it = {};
	};

	using iterator = iterator_impl<object>;
	using const_iterator = iterator_impl<const object>;

	// --------------------------------------------------------------------

	object() noexcept
	{
	}

	object(value_type t);

	object(const object &o)
		: m_type(o.m_type)
	{
		switch (m_type)
		{
			case value_type::null: break;
			case value_type::array: m_data = *o.m_data.m_array; break;
			case value_type::object: m_data = *o.m_data.m_object; break;
			case value_type::string: m_data = *o.m_data.m_string; break;
			case value_type::number_int: m_data = o.m_data.m_int; break;
			case value_type::number_float: m_data = o.m_data.m_float; break;
			case value_type::boolean: m_data = o.m_data.m_boolean; break;
		}
	}

	object(const std::vector<object> &v);

	object(std::initializer_list<object> v);

	object(const std::string &s);

	template <typename T>
		requires std::is_integral_v<T>
	object(T v);

	template <typename T>
		requires std::is_floating_point_v<T>
	object(T v);

	object(bool b);

	object(const nlohmann::json &j);

	// template <ConvertibleType T /* ,
	//      typename U = typename std::remove_cv_t<typename std::remove_reference_t<T>>,
	//      std::enable_if_t<not std::is_same_v<U, object> and detail::is_compatible_type_v<T>, int> = 0 */
	// 	>
	// object(T &&v) /* noexcept(noexcept(to_object(std::declval<object &>(), std::forward<T>(v)))) */
	// {
	// 	to_object(*this, std::forward<T>(v));
	// }

	object(object &&rhs) noexcept
	{
		swap(*this, rhs);
	}

	object &operator=(object rhs) noexcept
	{
		swap(*this, rhs);
		return *this;
	}

	// --------------------------------------------------------------------

	constexpr bool is_null() const noexcept { return m_type == value_type::null; }
	constexpr bool is_object() const noexcept { return m_type == value_type::object; }
	constexpr bool is_array() const noexcept { return m_type == value_type::array; }
	constexpr bool is_string() const noexcept { return m_type == value_type::string; }
	constexpr bool is_number() const noexcept { return is_number_int() or is_number_float(); }
	constexpr bool is_number_int() const noexcept { return m_type == value_type::number_int; }
	constexpr bool is_number_float() const noexcept { return m_type == value_type::number_float; }
	constexpr bool is_true() const noexcept { return is_boolean() and m_data.m_boolean == true; }
	constexpr bool is_false() const noexcept { return is_boolean() and m_data.m_boolean == false; }
	constexpr bool is_boolean() const noexcept { return m_type == value_type::boolean; }

	constexpr value_type type() const { return m_type; }
	// std::string type_name() const;

	explicit operator bool() const noexcept
	{
		bool result;
		switch (m_type)
		{
			case value_type::null: result = false; break;
			case value_type::boolean: result = m_data.m_boolean; break;
			case value_type::number_int: result = m_data.m_int != 0; break;
			case value_type::number_float: result = m_data.m_float != 0; break;
			case value_type::string: result = not m_data.m_string->empty(); break;
			default: result = not empty(); break;
		}
		return result;
	}

	template <typename T, typename U = typename std::remove_cvref_t<T>>
	// requires (is_convertible_type_v<U> and std::is_default_constructible_v<U>)
	T as() const; /* noexcept(noexcept(from_object(std::declval<const object &>(), std::declval<U &>()))) */
	// {
	// 	static_assert(std::is_default_constructible_v<U>, "Type must be default constructible to use with get()");

	// 	U ret = {};
	// 	if (not is_null())
	// 		from_object(*this, ret);
	// 	return ret;
	// }

	// --------------------------------------------------------------------

	friend void swap(object &a, object &b) noexcept
	{
		std::swap(a.m_type, b.m_type);
		std::swap(a.m_data, b.m_data);
	}

	// arithmetic operators

	object &operator-();

	friend object operator+(const_reference &lhs, const_reference &rhs);

	template <ScalarType T>
	friend object operator+(const_reference &lhs, const T &rhs)
	{
		return lhs + object(rhs);
	}

	template <ScalarType T>
	friend object operator+(const T &lhs, const_reference &rhs)
	{
		return object(lhs) + rhs;
	}

	friend object operator-(const_reference &lhs, const_reference &rhs);

	template <ScalarType T>
	friend object operator-(const_reference &lhs, const T &rhs)
	{
		return lhs - object(rhs);
	}

	template <ScalarType T>
	friend object operator-(const T &lhs, const_reference &rhs)
	{
		return object(lhs) - rhs;
	}

	friend object operator*(const_reference &lhs, const_reference &rhs);

	template <ScalarType T>
	friend object operator*(const_reference &lhs, const T &rhs)
	{
		return lhs * object(rhs);
	}

	template <ScalarType T>
	friend object operator*(const T &lhs, const_reference &rhs)
	{
		return object(lhs) * rhs;
	}

	friend object operator/(const_reference &lhs, const_reference &rhs);

	template <ScalarType T>
	friend object operator/(const_reference &lhs, const T &rhs)
	{
		return lhs / object(rhs);
	}

	template <ScalarType T>
	friend object operator/(const T &lhs, const_reference &rhs)
	{
		return object(lhs) / rhs;
	}

	friend object operator%(const_reference &lhs, const_reference &rhs);

	template <ScalarType T>
	friend object operator%(const_reference &lhs, const T &rhs)
	{
		return lhs % object(rhs);
	}

	template <ScalarType T>
	friend object operator%(const T &lhs, const_reference &rhs)
	{
		return object(lhs) % rhs;
	}

	friend bool operator==(const_reference &lhs, const_reference &rhs) noexcept;

	template <ScalarType T>
	friend bool operator==(const_reference &lhs, const T &rhs) noexcept
	{
		return lhs == object(rhs);
	}

	template <ScalarType T>
	friend bool operator==(const T &lhs, const_reference &rhs) noexcept
	{
		return object(lhs) == rhs;
	}

	friend bool operator<=>(const_reference &lhs, const_reference &rhs) noexcept;

	template <ScalarType T>
	friend bool operator<=>(const_reference &lhs, const T &rhs) noexcept
	{
		return lhs <=> object(rhs);
	}

	template <ScalarType T>
	friend bool operator<=>(const T &lhs, const_reference &rhs) noexcept
	{
		return object(lhs) <=> rhs;
	}

	// array/object interface

	bool contains(object test) const;

	bool empty() const noexcept;
	size_t size() const noexcept;
	size_t max_size() const noexcept;

	reference at(const std::string &key);
	const_reference at(const std::string &key) const;

	reference operator[](const std::string &key);
	const_reference operator[](const std::string &key) const;

	// access to array objects
	reference at(size_t index);
	const_reference at(size_t index) const;

	reference operator[](size_t index);
	const_reference operator[](size_t index) const;

	void push_back(object &&val);
	void push_back(const object &val);


	iterator begin();
	iterator end();

	const_iterator begin() const;
	const_iterator end() const;

	const_iterator cbegin();
	const_iterator cend();

	// I/O

	friend std::ostream &operator<<(std::ostream &os, const object &o);

  private:
	value_type m_type = value_type::null;
	union object_data
	{
		object_type *m_object;
		array_type *m_array;
		string_type *m_string;
		int64_t m_int;
		double m_float;
		bool m_boolean;

		object_data() = default;
		object_data(bool v) noexcept
			: m_boolean(v)
		{
		}
		object_data(int64_t v) noexcept
			: m_int(v)
		{
		}
		object_data(double v) noexcept
			: m_float(v)
		{
		}
		object_data(value_type t)
		{
			switch (t)
			{
				case value_type::array: m_array = create<array_type>(); break;
				case value_type::boolean: m_boolean = false; break;
				case value_type::null: m_object = nullptr; break;
				case value_type::number_float: m_float = 0; break;
				case value_type::number_int: m_int = 0; break;
				case value_type::object: m_object = create<object_type>(); break;
				case value_type::string: m_string = create<string_type>(); break;
			}
		}
		object_data(const object_type &v) { m_object = create<object_type>(v); }
		object_data(object_type &&v) { m_object = create<object_type>(std::move(v)); }
		object_data(const string_type &v) { m_string = create<string_type>(v); }
		object_data(string_type &&v) { m_string = create<string_type>(std::move(v)); }
		object_data(const array_type &v) { m_array = create<array_type>(v); }
		object_data(array_type &&v) { m_array = create<array_type>(std::move(v)); }

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
	} m_data{};

	template <typename T, typename... Args>
	static T *create(Args &&...args)
	{
		// return new T(args...);
		std::allocator<T> alloc;
		using AllocatorTraits = std::allocator_traits<std::allocator<T>>;

		auto deleter = [&](T *object)
		{
			AllocatorTraits::deallocate(alloc, object, 1);
		};

		std::unique_ptr<T, decltype(deleter)> object(AllocatorTraits::allocate(alloc, 1), deleter);
		assert(object != nullptr);
		AllocatorTraits::construct(alloc, object.get(), std::forward<Args>(args)...);
		return object.release();
	}
};

// namespace detail
// {

// 	template <>
// 	struct factory<value_type::boolean>
// 	{
// 		static void construct(object &o, bool b)
// 		{
// 			o.m_type = value_type::boolean;
// 			o.m_data = b;
// 		}
// 	};

// 	template <>
// 	struct factory<value_type::string>
// 	{
// 		static void construct(object &o, const std::string &s)
// 		{
// 			o.m_type = value_type::string;
// 			o.m_data = s;
// 		}

// 		static void construct(object &o, std::string &&s)
// 		{
// 			o.m_type = value_type::string;
// 			o.m_data = std::move(s);
// 		}
// 	};

// 	template <>
// 	struct factory<value_type::number_float>
// 	{
// 		static void construct(object &o, double d)
// 		{
// 			o.m_type = value_type::number_float;
// 			o.m_data = d;
// 		}
// 	};

// 	template <>
// 	struct factory<value_type::number_int>
// 	{
// 		static void construct(object &o, int64_t i)
// 		{
// 			o.m_type = value_type::number_int;
// 			o.m_data = i;
// 		}
// 	};

// 	template <>
// 	struct factory<value_type::array>
// 	{
// 		static void construct(object &o, const typename object::array_type &arr)
// 		{
// 			o.m_type = value_type::array;
// 			o.m_data = arr;
// 		}

// 		static void construct(object &o, typename object::array_type &&arr)
// 		{
// 			o.m_type = value_type::array;
// 			o.m_data = std::move(arr);
// 		}

// 		static void construct(object &o, const std::vector<bool> &arr)
// 		{
// 			o.m_type = value_type::array;
// 			o.m_data = value_type::array;
// 			o.m_data.m_array->reserve(arr.size());
// 			std::copy(arr.begin(), arr.end(), std::back_inserter(*o.m_data.m_array));
// 		}

// 		template <ConvertibleType T>
// 		static void construct(object &o, const std::vector<T> &arr)
// 		{
// 			o.m_type = value_type::array;
// 			o.m_data = value_type::array;
// 			o.m_data.m_array->resize(arr.size());
// 			std::copy(arr.begin(), arr.end(), o.m_data.m_array->begin());
// 		}

// 		template <ConvertibleType T, size_t N>
// 		static void construct(object &o, const T (&arr)[N])
// 		{
// 			o.m_type = value_type::array;
// 			o.m_data = value_type::array;
// 			o.m_data.m_array->resize(N);
// 			std::copy(arr, arr + N, o.m_data.m_array->begin());
// 		}
// 	};

// 	// template <>
// 	// struct factory<value_type::object>
// 	// {
// 	// 	static void construct(object &o, const typename object::object_type &obj)
// 	// 	{
// 	// 		o.m_type = value_type::object;
// 	// 		o.m_data = obj;
// 	// 	}

// 	// 	static void construct(object &o, typename object::object_type &&obj)
// 	// 	{
// 	// 		o.m_type = value_type::object;
// 	// 		o.m_data = std::move(obj);
// 	// 	}

// 	// 	// template <typename J, typename M,
// 	// 	// 	std::enable_if_t<not std::is_same_v<M, typename J::object_type>, int> = 0>
// 	// 	// static void construct(object &o, const M &obj)
// 	// 	// {
// 	// 	// 	using std::begin;
// 	// 	// 	using std::end;

// 	// 	// 	o.m_type = value_type::object;
// 	// 	// 	o.m_data.m_object = j.template create<typename J::object_type>(begin(obj), end(obj));
// 	// 	// }
// 	// };

// 	template <typename T>
// 		requires std::is_same_v<T, bool>
// 	void to_object(object &v, T b)
// 	{
// 		factory<value_type::boolean>::construct(v, b);
// 	}

// 	template <StringType T>
// 	void to_object(object &o, const T &s)
// 	{
// 		factory<value_type::string>::construct(o, s);
// 	}

// 	template <typename T>
// 		requires std::is_floating_point_v<T>
// 	void to_object(object &o, T f)
// 	{
// 		factory<value_type::number_float>::construct(o, f);
// 	}

// 	template <typename T>
// 		requires std::is_integral_v<T>
// 	void to_object(object &o, T i)
// 	{
// 		factory<value_type::number_int>::construct(o, i);
// 	}

// 	template <typename T>
// 		requires std::is_same_v<T, bool>
// 	void to_object(object &o, const std::vector<T> &v)
// 	{
// 		factory<value_type::array>::construct(o, v);
// 	}

// 	template <typename T>
// 		requires(
// 			is_compatible_array_type_v<T> /*  and
// 	         not is_compatible_object_type_v<object, T> and
// 	         not is_compatible_string_type_v<object, T> and
// 	         not is_object_v<T> */
// 			)
// 	void to_object(object &o, const T &arr)
// 	{
// 		factory<value_type::array>::construct(o, arr);
// 	}

// 	template <ConvertibleType T>
// 	void to_object(object &o, const std::valarray<T> &arr)
// 	{
// 		factory<value_type::array>::construct(o, std::move(arr));
// 	}

// 	template <typename T>
// 		requires std::is_same_v<T, std::vector<object>>
// 	void to_object(object &o, const T &arr)
// 	{
// 		factory<value_type::array>::construct(o, std::move(arr));
// 	}

// 	template <ConvertibleType T, size_t N>
// 	void to_object(object &o, const T (&arr)[N])
// 	{
// 		factory<value_type::array>::construct(o, std::move(arr));
// 	}

// 	template <ConvertibleType T>
// 	void to_object(object &o, const std::optional<T> &v)
// 	{
// 		if (v)
// 			to_object(o, *v);
// 	}

// } // namespace detail

// template <typename T>
// template <typename Arg = T>
// static auto ObjectSerializer<T>::to_object(object &o, Arg &&v) /* noexcept */
// 	-> decltype(::zeep::http::to_object(o, std::forward<Arg>(v)), void())
// {
// 	::zeep::http::to_object(0, std::forward<Arg>(v));
// }

// template <typename T>
// struct has_to_object_2<T>
// 	: requires (not std::is_same_v<T, object>)
// {
// 	using serializer = typename object::typename object_serializer<T>;

// 	static constexpr bool value = is_detected_exact<void, to_object_function, serializer, object &, T>::value;
// };

// static_assert(has_to_object_2<int>, "oh oh");
// static_assert(has_to_object_2<float>, "oh oh");
// static_assert(has_to_object_2<object>, "oh oh");
// static_assert(has_to_object_2<bool>, "oh oh");

// static_assert(has_to_object_v<std::string>, "oh oh");

} // namespace zeep::http
