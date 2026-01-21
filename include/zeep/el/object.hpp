//        Copyright Maarten L. Hekkelman 2025-2026
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "zeep/streambuf.hpp"
#include <algorithm>
#include <cstddef>

#if __has_include(<nlohmann/json.hpp>)
# include <nlohmann/json.hpp>
# define HAVE_NLOHMANN_JSON 1
#endif

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace zeep::el
{

class object;

// concepts

template <typename T>
concept BooleanType = std::is_same_v<bool, std::remove_cvref_t<T>>;

template <typename T>
concept ObjectType = std::is_same_v<object, std::remove_cvref_t<T>>;

template <typename T>
concept NumberType = ((std::is_integral_v<std::remove_cvref_t<T>> or std::is_floating_point_v<std::remove_cvref_t<T>>) and not std::is_same_v<std::remove_cvref_t<T>, bool>);

template <typename T>
concept StringType = (std::is_assignable_v<std::string, T> and not std::is_integral_v<T> and not std::is_floating_point_v<T>);

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

	using reference = object &;
	using const_reference = const object &;

	// --------------------------------------------------------------------

	template <ObjectType T>
	struct iterator_impl
	{
		friend class object;

		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = T::difference_type;
		using pointer = typename std::conditional_t<std::is_const_v<T>, typename T::const_pointer, typename T::pointer>;
		using reference = typename std::conditional_t<std::is_const_v<T>, typename T::const_reference, typename T::reference>;

		iterator_impl() = default;

		explicit iterator_impl(pointer obj) noexcept
			: m_obj(obj)
		{
			assert(m_obj);
			switch (m_obj->m_data.m_type)
			{
				case value_type::array: m_it.m_array_it = m_obj->m_data.m_value.m_array->begin(); break;
				case value_type::object: m_it.m_object_it = m_obj->m_data.m_value.m_object->begin(); break;
				default: m_it.m_p = 0; break;
			}
		}
		iterator_impl(pointer obj, [[maybe_unused]] int dummy) noexcept
			: m_obj(obj)
		{
			assert(m_obj);
			switch (m_obj->m_data.m_type)
			{
				case value_type::array: m_it.m_array_it = m_obj->m_data.m_value.m_array->end(); break;
				case value_type::object: m_it.m_object_it = m_obj->m_data.m_value.m_object->end(); break;
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
			switch (m_obj->m_data.m_type)
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
			switch (m_obj->m_data.m_type)
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
			switch (m_obj->m_data.m_type)
			{
				case value_type::array:
					assert(m_it.m_array_it != m_obj->m_data.m_value.m_array->end());
					return *m_it.m_array_it;
					break;

				case value_type::object:
					assert(m_it.m_object_it != m_obj->m_data.m_value.m_object->end());
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
			switch (m_obj->m_data.m_type)
			{
				case value_type::array:
					assert(m_it.m_array_it != m_obj->m_data.m_value.m_array->end());
					return &(*m_it.m_array_it);
					break;

				case value_type::object:
					assert(m_it.m_object_it != m_obj->m_data.m_value.m_object->end());
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
			switch (m_obj->m_data.m_type)
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
			switch (m_obj->m_data.m_type)
			{
				case value_type::array: return m_it.m_array_it <=> other.m_it.m_array_it;
				case value_type::object: throw std::runtime_error("Cannot compare order of object iterators");
				default: return m_it.m_p <=> other.m_it.m_p;
			}
		}

		iterator_impl &operator+=(difference_type i)
		{
			assert(m_obj);
			switch (m_obj->m_data.m_type)
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
			switch (m_obj->m_data.m_type)
			{
				case value_type::array: return m_it.m_array_it - other.m_it.m_array_it;
				case value_type::object: throw std::runtime_error("Cannot use offsets with object iterators");
				default: return m_it.m_p - other.m_it.m_p;
			}
		}

		reference operator[](difference_type i) const
		{
			assert(m_obj);
			switch (m_obj->m_data.m_type)
			{
				case value_type::array: *std::next(m_it.m_array_it, i);
				case value_type::object: throw std::runtime_error("Cannot use offsets with object iterators");
				default:
					if (m_it.m_p == -i)
						return *m_obj;
					throw std::runtime_error("Cannot get value");
			}
		}

		[[nodiscard]] const std::string &key() const
		{
			assert(m_obj);

			if (not m_obj->is_object())
				throw std::runtime_error("Can only use key() on object iterators");

			return m_it.m_object_it->first;
		}

		[[nodiscard]] reference value() const
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

	object() noexcept = default;

	object(value_type t) noexcept
		: m_data(t)
	{
	}

	object(const object &o)
	{
		m_data.m_type = o.m_data.m_type;
		switch (m_data.m_type)
		{
			case value_type::null: break;
			case value_type::array: m_data.m_value = *o.m_data.m_value.m_array; break;
			case value_type::object: m_data.m_value = *o.m_data.m_value.m_object; break;
			case value_type::string: m_data.m_value = *o.m_data.m_value.m_string; break;
			case value_type::number_int: m_data.m_value = o.m_data.m_value.m_int; break;
			case value_type::number_float: m_data.m_value = o.m_data.m_value.m_float; break;
			case value_type::boolean: m_data.m_value = o.m_data.m_value.m_boolean; break;
		}
	}

	object(const std::vector<object> &v)
	{
		m_data.m_type = value_type::array;
		m_data.m_value = v;
	}

	object(std::initializer_list<object> init)
	{
		bool isAnObject = std::ranges::all_of(init, [](auto &ref)
			{ return ref.is_array() and ref.m_data.m_value.m_array->size() == 2 and ref.m_data.m_value.m_array->front().is_string(); });

		if (isAnObject)
		{
			m_data.m_type = value_type::object;
			m_data.m_value = value_type::object;

			for (auto &el : init)
			{
				m_data.m_value.m_object->emplace(
					std::move(*el.m_data.m_value.m_array->front().m_data.m_value.m_string),
					std::move(el.m_data.m_value.m_array->back()));
			}
		}
		else
		{
			m_data.m_type = value_type::array;
			m_data.m_value.m_array = create<array_type>(init.begin(), init.end());
		}
	}

	object(std::nullptr_t)
	{
		m_data.m_type = value_type::null;
	}

	template <StringType T>
	object(const T &s)
	{
		m_data.m_type = value_type::string;
		m_data.m_value = std::string{ s };
	}

	template <NumberType T>
	object(T v)
	{
		if constexpr (std::is_integral_v<T>)
		{
			m_data.m_type = value_type::number_int;
			m_data.m_value = static_cast<int64_t>(v);
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			m_data.m_type = value_type::number_float;
			m_data.m_value = static_cast<double>(v);
		}
		else
			assert(false);
	}

	template <BooleanType T>
	object(T b)
	{
		m_data.m_type = value_type::boolean;
		m_data.m_value = static_cast<bool>(b);
	}

#if HAVE_NLOHMANN_JSON
	object(const nlohmann::json &j)
	{
		// to be implemented
		switch (j.type())
		{
			case nlohmann::json::value_t::null:
				m_data.m_type = value_type::null;
				break;

			case nlohmann::json::value_t::object:
				for (auto i = j.begin(); i != j.end(); ++i)
					operator[](i.key()) = object(i.value());
				break;

			case nlohmann::json::value_t::array:
				for (auto &e : j)
					push_back(object(e));
				break;

			case nlohmann::json::value_t::string:
				m_data.m_type = value_type::string;
				m_data.m_value = j.template get<std::string>();
				break;

			case nlohmann::json::value_t::boolean:
				m_data.m_type = value_type::boolean;
				m_data.m_value = j.template get<bool>();
				break;

			case nlohmann::json::value_t::number_integer:
				m_data.m_type = value_type::number_int;
				m_data.m_value = j.template get<int64_t>();
				break;

			case nlohmann::json::value_t::number_unsigned:
				m_data.m_type = value_type::number_int;
				m_data.m_value = static_cast<int64_t>(j.template get<uint64_t>());
				break;

			case nlohmann::json::value_t::number_float:
				m_data.m_type = value_type::number_float;
				m_data.m_value = j.template get<double>();
				break;

			case nlohmann::json::value_t::binary:
			case nlohmann::json::value_t::discarded:
				assert(false);
				break;
		}
	}
#endif

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

	[[nodiscard]] constexpr bool is_null() const noexcept { return m_data.m_type == value_type::null; }
	[[nodiscard]] constexpr bool is_object() const noexcept { return m_data.m_type == value_type::object; }
	[[nodiscard]] constexpr bool is_array() const noexcept { return m_data.m_type == value_type::array; }
	[[nodiscard]] constexpr bool is_string() const noexcept { return m_data.m_type == value_type::string; }
	[[nodiscard]] constexpr bool is_number() const noexcept { return is_number_int() or is_number_float(); }
	[[nodiscard]] constexpr bool is_number_int() const noexcept { return m_data.m_type == value_type::number_int; }
	[[nodiscard]] constexpr bool is_number_float() const noexcept { return m_data.m_type == value_type::number_float; }
	[[nodiscard]] constexpr bool is_true() const noexcept { return is_boolean() and m_data.m_value.m_boolean == true; }
	[[nodiscard]] constexpr bool is_false() const noexcept { return is_boolean() and m_data.m_value.m_boolean == false; }
	[[nodiscard]] constexpr bool is_boolean() const noexcept { return m_data.m_type == value_type::boolean; }

	[[nodiscard]] constexpr value_type type() const { return m_data.m_type; }

	explicit operator bool() const noexcept
	{
		bool result;
		switch (m_data.m_type)
		{
			case value_type::null: result = false; break;
			case value_type::boolean: result = m_data.m_value.m_boolean; break;
			case value_type::number_int: result = m_data.m_value.m_int != 0; break;
			case value_type::number_float: result = m_data.m_value.m_float != 0; break;
			case value_type::string: result = not m_data.m_value.m_string->empty(); break;
			default: result = not empty(); break;
		}
		return result;
	}

	// --------------------------------------------------------------------

	template <StringType T>
	[[nodiscard]] inline std::string get() const
	{
		if (m_data.m_type == value_type::string)
			return *m_data.m_value.m_string;

		return get_JSON();
	}

	template <BooleanType T>
	[[nodiscard]] inline bool get() const
	{
		switch (m_data.m_type)
		{
			case value_type::boolean:
				return m_data.m_value.m_boolean;
			case value_type::number_int:
				return m_data.m_value.m_int != 0;
			case value_type::number_float:
				return m_data.m_value.m_float != 0;
			default:
				return not empty();
		}
	}

	template <NumberType T>
	[[nodiscard]] std::remove_cvref_t<T> get() const
	{
		switch (m_data.m_type)
		{
			case value_type::boolean:
				return m_data.m_value.m_boolean;
			case value_type::number_int:
				return m_data.m_value.m_int;
			case value_type::number_float:
				return m_data.m_value.m_float;
			default:
				return not empty();
		}
	}

	// --------------------------------------------------------------------

	friend void swap(object &a, object &b) noexcept
	{
		std::swap(a.m_data.m_type, b.m_data.m_type);
		std::swap(a.m_data.m_value, b.m_data.m_value);
	}

	// --------------------------------------------------------------------
	// arithmetic operators

	object &operator-()
	{
		switch (m_data.m_type)
		{
			case value_type::number_int:
				m_data.m_value.m_int = -m_data.m_value.m_int;
				break;

			case value_type::number_float:
				m_data.m_value.m_float = -m_data.m_value.m_float;
				break;

			default:
				throw std::runtime_error("Can only negate numbers");
		}

		return *this;
	}

	friend object operator+(const_reference &lhs, const_reference &rhs);

	template <NumberType T>
	friend object operator+(const_reference &lhs, const T &rhs)
	{
		return lhs + object(rhs);
	}

	template <NumberType T>
	friend object operator+(const T &lhs, const_reference &rhs)
	{
		return object(lhs) + rhs;
	}

	friend object operator-(const_reference &lhs, const_reference &rhs);

	template <NumberType T>
	friend object operator-(const_reference &lhs, const T &rhs)
	{
		return lhs - object(rhs);
	}

	template <NumberType T>
	friend object operator-(const T &lhs, const_reference &rhs)
	{
		return object(lhs) - rhs;
	}

	friend object operator*(const_reference &lhs, const_reference &rhs);

	template <NumberType T>
	friend object operator*(const_reference &lhs, const T &rhs)
	{
		return lhs * object(rhs);
	}

	template <NumberType T>
	friend object operator*(const T &lhs, const_reference &rhs)
	{
		return object(lhs) * rhs;
	}

	friend object operator/(const_reference &lhs, const_reference &rhs);

	template <NumberType T>
	friend object operator/(const_reference &lhs, const T &rhs)
	{
		return lhs / object(rhs);
	}

	template <NumberType T>
	friend object operator/(const T &lhs, const_reference &rhs)
	{
		return object(lhs) / rhs;
	}

	friend object operator%(const_reference &lhs, const_reference &rhs);

	template <NumberType T>
	friend object operator%(const_reference &lhs, const T &rhs)
	{
		return lhs % object(rhs);
	}

	template <NumberType T>
	friend object operator%(const T &lhs, const_reference &rhs)
	{
		return object(lhs) % rhs;
	}

	friend bool operator==(const_reference &lhs, const_reference &rhs) noexcept;

	template <NumberType T>
	friend bool operator==(const_reference &lhs, const T &rhs) noexcept
	{
		return lhs == object(rhs);
	}

	template <NumberType T>
	friend bool operator==(const T &lhs, const_reference &rhs) noexcept
	{
		return object(lhs) == rhs;
	}

	friend std::partial_ordering operator<=>(const_reference &lhs, const_reference &rhs) noexcept;

	template <NumberType T>
	friend std::partial_ordering operator<=>(const_reference &lhs, const T &rhs) noexcept
	{
		return lhs <=> object(rhs);
	}

	template <NumberType T>
	friend std::partial_ordering operator<=>(const T &lhs, const_reference &rhs) noexcept
	{
		return object(lhs) <=> rhs;
	}

	// --------------------------------------------------------------------
	// array/object interface

	[[nodiscard]] bool contains(const object &test) const;

	[[nodiscard]] bool empty() const noexcept;
	[[nodiscard]] size_t size() const noexcept;
	[[nodiscard]] size_t max_size() const noexcept;

	[[nodiscard]] reference at(const std::string &key);
	[[nodiscard]] const_reference at(const std::string &key) const;

	[[nodiscard]] reference operator[](const std::string &key);
	[[nodiscard]] const_reference operator[](const std::string &key) const;

	// access to array objects
	[[nodiscard]] reference at(size_t index);
	[[nodiscard]] const_reference at(size_t index) const;

	[[nodiscard]] reference operator[](size_t index);
	[[nodiscard]] const_reference operator[](size_t index) const;

	void push_back(object &&val);
	void push_back(const object &val);

	template <typename... Args>
	std::pair<iterator, bool> emplace(Args &&...args)
	{
		if (is_null())
		{
			m_data.m_type = value_type::object;
			m_data.m_value = value_type::object;
		}
		else if (not is_object())
			throw std::runtime_error("emplace only works with object type");

		auto r = m_data.m_value.m_object->emplace(std::forward<Args>(args)...);
		auto i = begin();
		i.m_it.m_object_it = r.first;

		return { i, r.second };
	}

	template <typename... Args>
	object &emplace_back(Args &&...args)
	{
		if (not(is_null() or is_array()))
			throw std::runtime_error("emplace_back only works with array type");

		if (is_null())
		{
			m_data.m_type = value_type::array;
			m_data.m_value = value_type::array;
		}

		return m_data.m_value.m_array->emplace_back(std::forward<Args>(args)...);
	}

	template <typename Iterator>
		requires std::is_same_v<Iterator, iterator> or std::is_same_v<Iterator, const_iterator>
	Iterator erase(Iterator pos)
	{
		if (pos.m_obj != this)
			throw std::runtime_error("Invalid iterator");

		auto result = end();

		switch (m_data.m_type)
		{
			case value_type::array:
				result.m_it.m_array_it = m_data.m_value.m_array->erase(pos.m_it.m_array_it);
				break;

			case value_type::object:
				result.m_it.m_object_it = m_data.m_value.m_object->erase(pos.m_it.m_object_it);
				break;

			case value_type::null:
				throw std::runtime_error("Cannot erase in null values");

			default:
				if (pos.m_it.m_p != 0)
					throw std::runtime_error("Iterator out of range");

				if (m_data.m_type == value_type::string)
				{
					std::allocator<string_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.m_string);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.m_string, 1);
					m_data.m_value.m_string = nullptr;
				}

				m_data.m_type = value_type::null;
				break;
		}

		return result;
	}

	template <typename Iterator>
		requires std::is_same_v<Iterator, iterator> or std::is_same_v<Iterator, const_iterator>
	Iterator erase(Iterator first, Iterator last)
	{
		if (first.m_obj != this or last.m_obj != this)
			throw std::runtime_error("Invalid iterator");

		auto result = end();

		switch (m_data.m_type)
		{
			case value_type::array:
				result.m_it.m_array_it = m_data.m_value.m_array->erase(first.m_it.m_array_it, last.m_it.m_array_it);
				break;

			case value_type::object:
				result.m_it.m_object_it = m_data.m_value.m_object->erase(first.m_it.m_object_it, last.m_it.m_object_it);
				break;

			case value_type::null:
				throw std::runtime_error("Cannot erase in null values");

			default:
				if (first.m_it.m_p != 0 or last.m_it.m_p != 0)
					throw std::runtime_error("Iterator out of range");

				if (m_data.m_type == value_type::string)
				{
					std::allocator<string_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.m_string);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.m_string, 1);
					m_data.m_value.m_string = nullptr;
				}

				m_data.m_type = value_type::null;
				break;
		}

		return result;
	}

	size_type erase(const std::string &key)
	{
		if (is_object())
			return m_data.m_value.m_object->erase(key);
		throw std::runtime_error("erase with a string key only works with object type");
	}

	void erase(const size_type index)
	{
		if (is_array())
		{
			if (index >= size())
				throw std::runtime_error("Index out of range");
			m_data.m_value.m_array->erase(m_data.m_value.m_array->begin() + static_cast<difference_type>(index));
		}
		else
			throw std::runtime_error("erase with an index only works wiht array type");
	}

	// --------------------------------------------------------------------

	[[nodiscard]] iterator begin() { return iterator(this); }
	[[nodiscard]] iterator end() { return { this, 1 }; }

	[[nodiscard]] const_iterator begin() const { return const_iterator(this); }
	[[nodiscard]] const_iterator end() const { return { this, 1 }; }

	[[nodiscard]] const_iterator cbegin() { return const_iterator(this); }
	[[nodiscard]] const_iterator cend() { return { this, 1 }; }

	[[nodiscard]] object &front()
	{
		return *begin();
	}

	[[nodiscard]] const object &front() const
	{
		return *begin();
	}

	[[nodiscard]] object &back()
	{
		return *--end();
	}

	[[nodiscard]] const object &back() const
	{
		return *--end();
	}

	// I/O

	friend void serialize(std::ostream &os, const object &o);
	friend void deserialize(std::istream &is, object &o);

	// And some more alternatives
	static object parse_JSON(std::istream &is)
	{
		object result;
		deserialize(is, result);
		return result;
	}

	static object parse_JSON(std::string_view s)
	{
		char_streambuf b(s.data(), s.length());
		std::istream is(&b);
		return parse_JSON(is);
	}

	// And get the object as a JSON string
	[[nodiscard]] std::string get_JSON() const
	{
		std::ostringstream os;
		serialize(os, *this);
		return os.str();
	}

	// convenience
	friend std::ostream &operator<<(std::ostream &os, const object &o)
	{
		serialize(os, o);
		return os;
	}

  private:
	union object_value
	{
		object_type *m_object;
		array_type *m_array;
		string_type *m_string;
		int64_t m_int;
		double m_float;
		bool m_boolean;

		object_value() noexcept
			: m_object(nullptr)
		{
		}

		object_value(bool v) noexcept
			: m_boolean(v)
		{
		}
		object_value(int64_t v) noexcept
			: m_int(v)
		{
		}
		object_value(double v) noexcept
			: m_float(v)
		{
		}
		object_value(value_type t)
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
		object_value(const object_type &v) { m_object = create<object_type>(v); }
		object_value(object_type &&v) { m_object = create<object_type>(std::move(v)); }
		object_value(const string_type &v) { m_string = create<string_type>(v); }
		object_value(string_type &&v) { m_string = create<string_type>(std::move(v)); }
		object_value(const array_type &v) { m_array = create<array_type>(v); }
		object_value(array_type &&v) { m_array = create<array_type>(std::move(v)); }

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

	struct object_data
	{
		value_type m_type = value_type::null;
		object_value m_value{};

		object_data(const value_type t)
			: m_type(t)
			, m_value(t)
		{
		}

		object_data(size_type cnt, const object &val)
			: m_type(value_type::array)
		{
			m_value.m_array = create<array_type>(cnt, val);
		}

		object_data() noexcept = default;
		object_data(object_data &&) noexcept = default;
		object_data(const object_data &) noexcept = delete;
		object_data &operator=(object_data &&) noexcept = delete;
		object_data &operator=(const object_data &) noexcept = delete;

		~object_data() noexcept
		{
			m_value.destroy(m_type);
		}
	} m_data{};

	template <typename T, typename... Args>
	[[nodiscard]] static T *create(Args &&...args)
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

} // namespace zeep::el
