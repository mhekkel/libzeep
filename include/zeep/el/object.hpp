// SPDX-FileCopyrightText: Maarten L. Hekkelman 2025-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the el::object class, a dynamic JSON-like type supporting
/// null, boolean, integer, float, string, array, and object value types

#include "zeep/exception.hpp"
#include "zeep/streambuf.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#if __has_include(<nlohmann/json.hpp>)
# include <nlohmann/json.hpp>
# define HAVE_NLOHMANN_JSON 1
#endif

namespace zeep::el
{

/// \brief Exception thrown when an invalid type or access error occurs
///        on an el::object

class object_error : public zeep::exception
{
  public:
	object_error(const std::string &err)
		: zeep::exception(err)
	{
	}

	object_error(const char *err)
		: zeep::exception(err)
	{
	}

	object_error(const object_error &) noexcept = default;
};

class object;

// concepts

/// \brief Concept matching only \c bool types (after removing cvref qualifiers)
template <typename T>
concept BooleanType = std::is_same_v<bool, std::remove_cvref_t<T>>;

/// \brief Concept matching only zeep::el::object types (after removing cvref qualifiers)
template <typename T>
concept ObjectType = std::is_same_v<object, std::remove_cvref_t<T>>;

/// \brief Concept matching integral (excluding bool) and floating-point types
template <typename T>
concept NumberType = ((std::is_integral_v<std::remove_cvref_t<T>> or std::is_floating_point_v<std::remove_cvref_t<T>>) and not std::is_same_v<std::remove_cvref_t<T>, bool>);

/// \brief Concept matching types assignable to std::string, excluding
///        integral and floating-point types
template <typename T>
concept StringType = (std::is_assignable_v<std::string, T> and not std::is_integral_v<T> and not std::is_floating_point_v<T>);

// --------------------------------------------------------------------

/// \brief A dynamic JSON-like type supporting multiple value types
///
/// The \a object class provides a runtime-polymorphic container similar to
/// JSON. It can hold values of type null, boolean, integer, float, string,
/// array, or object. The class supports parsing from and serializing to
/// JSON, iteration over arrays and objects, and conversion to/from C++
/// types via the serializer framework (see zeep::el::serializer).
///
/// Example usage:
/// \code{.cpp}
///   zeep::el::object obj = zeep::el::object::parse_JSON(R"({"a":1,"b":2})");
///   int a = obj["a"].get<int>();
///   obj["c"] = "hello";
///   std::string json = obj.get_JSON();
/// \endcode

class object
{
  public:
	/// \brief The supported value types for an object
	enum class value_type
	{
		null,         ///< Represents a null/empty value
		object,       ///< A map of string keys to object values
		array,        ///< An ordered vector of object values
		string,       ///< A UTF-8 string value
		number_int,   ///< A signed 64-bit integer value
		number_float, ///< A double-precision floating-point value
		boolean       ///< A boolean value
	};

	/// \brief Defines an ordering over value_type enumerators
	///
	/// The ordering is: null < boolean < number < object < array < string
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

	/// \brief A bidirectional iterator for arrays and objects
	///
	/// When iterating an \a object of type array, dereferencing yields each
	/// element. When iterating an object, dereferencing yields the mapped
	/// value; use \a key() to retrieve the associated string key.
	/// \tparam T  object or const object

	template <ObjectType T>
	struct iterator_impl
	{
		friend class object;

		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = T::difference_type;
		using pointer = typename std::conditional_t<std::is_const_v<T>, typename T::const_pointer, typename T::pointer>;
		using reference = typename std::conditional_t<std::is_const_v<T>, typename T::const_reference, typename T::reference>;
		using value_type = std::remove_cv_t<T>;

		/// \brief Default constructor — creates a singular iterator
		iterator_impl() = default;

		/// \brief Construct an iterator pointing to the first element
		explicit iterator_impl(pointer obj) noexcept
			: m_obj(obj)
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array: m_it = std::get<array_type>(m_obj->m_data).begin(); break;
				case object::value_type::object: m_it = std::get<object_type>(m_obj->m_data).begin(); break;
				default: m_it = 0; break;
			}
		}
		/// \brief Construct an iterator pointing past the last element
		iterator_impl(pointer obj, [[maybe_unused]] int dummy) noexcept
			: m_obj(obj)
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array: m_it = std::get<array_type>(m_obj->m_data).end(); break;
				case object::value_type::object: m_it = std::get<object_type>(m_obj->m_data).end(); break;
				case object::value_type::null: m_it = 0; break;
				default: m_it = 1; break;
			}
		}
		iterator_impl(const iterator_impl &i) noexcept
			: m_obj(i.m_obj)
			, m_it(i.m_it)
		{
		}

		/// \brief Postfix decrement
		iterator_impl operator--(int)
		{
			auto result(*this);
			operator--();
			return result;
		}

		/// \brief Prefix decrement
		iterator_impl &operator--()
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array: std::advance(std::get<array_iterator_type>(m_it), -1); break;
				case object::value_type::object: std::advance(std::get<object_iterator_type>(m_it), -1); break;
				default: --std::get<difference_type>(m_it); break;
			}
			return *this;
		}

		/// \brief Postfix increment
		iterator_impl operator++(int)
		{
			auto result(*this);
			operator++();
			return result;
		}

		/// \brief Prefix increment
		iterator_impl &operator++()
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array: std::advance(std::get<array_iterator_type>(m_it), +1); break;
				case object::value_type::object: std::advance(std::get<object_iterator_type>(m_it), +1); break;
				default: ++std::get<difference_type>(m_it); break;
			}
			return *this;
		}

		/// \brief Dereference — access the current element
		reference operator*() const
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array:
					return *std::get<array_iterator_type>(m_it);
					break;

				case object::value_type::object:
					return std::get<object_iterator_type>(m_it)->second;
					break;

				case object::value_type::null:
					throw object_error("Cannot get value");

				default:
					if (std::get<difference_type>(m_it) == 0)
						return *m_obj;
					throw object_error("Cannot get value");
			}
		}

		/// \brief Arrow operator — access the current element through a pointer
		pointer operator->() const
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array:
					assert(std::get<array_iterator_type>(m_it) != std::get<array_type>(m_obj->m_data).end());
					return &(*std::get<array_iterator_type>(m_it));
					break;

				case object::value_type::object:
					assert(std::get<object_iterator_type>(m_it) != std::get<object_type>(m_obj->m_data).end());
					return &(std::get<object_iterator_type>(m_it)->second);
					break;

				case object::value_type::null:
					throw object_error("Cannot get value");

				default:
					if (std::get<difference_type>(m_it) == 0)
						return m_obj;
					throw object_error("Cannot get value");
			}
		}

		/// \brief Equality comparison
		bool operator==(const iterator_impl &other) const
		{
			if (m_obj != other.m_obj)
				throw object_error("Containers are not the same");

			assert(m_obj);
			return m_it == other.m_it;
		}

		/// \brief Three-way comparison (not supported for object iterators)
		auto operator<=>(const iterator_impl &other) const
		{
			if (m_obj != other.m_obj)
				throw object_error("Containers are not the same");

			assert(m_obj);
			return m_it <=> other.m_it;
		}

		/// \brief Advance by \a i positions
		iterator_impl &operator+=(difference_type i)
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array: std::advance(std::get<array_iterator_type>(m_it), i); break;
				case object::value_type::object: throw object_error("Cannot use offsets with object iterators");
				default: std::get<difference_type>(m_it) += i;
			}
			return *this;
		}

		/// \brief Retreat by \a i positions
		iterator_impl &operator-=(difference_type i)
		{
			operator+=(-i);
			return *this;
		}

		/// \brief Return a new iterator advanced by \a i positions
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

		/// \brief Return a new iterator retreated by \a i positions
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

		/// \brief Distance between two iterators
		difference_type operator-(const iterator_impl &other) const
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array: return std::get<array_iterator_type>(m_it) - std::get<array_iterator_type>(other.m_it);
				case object::value_type::object: throw object_error("Cannot use offsets with object iterators");
				default: return std::get<difference_type>(m_it) - std::get<difference_type>(other.m_it);
			}
		}

		/// \brief Indexed access
		reference operator[](difference_type i) const
		{
			assert(m_obj);
			switch (m_obj->type())
			{
				case object::value_type::array: return *std::next(std::get<array_iterator_type>(m_it), i); break;
				case object::value_type::object: throw object_error("Cannot use offsets with object iterators");
				default:
					if (std::get<difference_type>(m_it) == -i)
						return *m_obj;
					throw object_error("Cannot get value");
			}
			std::unreachable();
		}

		/// \brief Return the key of the current element (object type only)
		[[nodiscard]] const std::string &key() const
		{
			assert(m_obj);

			if (not m_obj->is_object())
				throw object_error("Can only use key() on object iterators");

			return std::get<object_iterator_type>(m_it)->first;
		}

		/// \brief Return a reference to the current value (equivalent to operator*)
		[[nodiscard]] reference value() const
		{
			return operator*();
		}

	  private:
		/// @cond

		pointer m_obj = nullptr;

		using array_iterator_type = std::conditional_t<std::is_const_v<T>, typename T::array_type::const_iterator, typename T::array_type::iterator>;
		using object_iterator_type = std::conditional_t<std::is_const_v<T>, typename T::object_type::const_iterator, typename T::object_type::iterator>;

		std::variant<array_iterator_type, object_iterator_type, difference_type> m_it = {};

		/// @endcond
	};

	using iterator = iterator_impl<object>;
	using const_iterator = iterator_impl<const object>;

	static_assert(std::input_iterator<iterator>);
	static_assert(std::input_iterator<const_iterator>);

	// --------------------------------------------------------------------

	/// \brief Default constructor — creates a null object
	constexpr object() noexcept = default;

	/// \brief Construct an object of a specific \a value_type
	/// \param t  The type to construct (e.g., value_type::array)
	constexpr object(value_type t) noexcept
	{
		switch (t)
		{
			case value_type::null: break;
			case value_type::array: m_data = array_type{}; break;
			case value_type::object: m_data = object_type{}; break;
			case value_type::string: m_data = string_type{}; break;
			case value_type::number_int: m_data = 0; break;
			case value_type::number_float: m_data = 0.f; break;
			case value_type::boolean: m_data = false; break;
		}
	}

	/// \brief Copy constructor — deep copies the value
	object(const object &o)
		: m_data(o.m_data)
	{
	}

	/// \brief Construct an array object from a vector of objects
	object(const std::vector<object> &v)
		: m_data(v)
	{
	}

	/// \brief Construct from an initializer list
	///
	/// If every element of the initializer list is a two-element array whose
	/// first element is a string, the result is an object (map). Otherwise
	/// the result is an array.
	object(std::initializer_list<object> init)
	{
		bool isAnObject = std::ranges::all_of(init, [](const object &ref)
			{ return ref.is_array() and ref.size() == 2 and ref.front().is_string(); });

		if (isAnObject)
		{
			m_data = object_type{};

			for (auto &el : init)
				emplace(el[0].get<std::string>(), el[1]);
		}
		else
			m_data = array_type{ init.begin(), init.end() };
	}

	/// \brief Construct a null object from nullptr
	constexpr object(std::nullptr_t) noexcept
	{
	}

	/// \brief Construct a string object from a string-compatible type
	object(StringType auto const &s)
	{
		m_data = std::string{ s };
	}

	/// \brief Construct a number object from an integral or floating-point value
	constexpr object(NumberType auto v) noexcept
	{
		if constexpr (std::is_floating_point_v<decltype(v)>)
			m_data = static_cast<double>(v);
		else
			m_data = static_cast<int64_t>(v);
	}

	/// \brief Construct a boolean object
	constexpr object(BooleanType auto b) noexcept
		: m_data(b)
	{
	}

#if HAVE_NLOHMANN_JSON
	/// \brief Construct an object from an nlohmann::json value
	object(const nlohmann::json &j)
	{
		// to be implemented
		switch (j.type())
		{
			case nlohmann::json::value_t::null:
				m_data = std::monostate{};
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
				m_data = j.template get<std::string>();
				break;

			case nlohmann::json::value_t::boolean:
				m_data = j.template get<bool>();
				break;

			case nlohmann::json::value_t::number_integer:
				m_data = j.template get<int64_t>();
				break;

			case nlohmann::json::value_t::number_unsigned:
				m_data = static_cast<int64_t>(j.template get<uint64_t>());
				break;

			case nlohmann::json::value_t::number_float:
				m_data = j.template get<double>();
				break;

			case nlohmann::json::value_t::binary:
			case nlohmann::json::value_t::discarded:
				assert(false);
				break;
		}
	}
#endif

	/// \brief Move constructor
	object(object &&rhs) noexcept
	{
		swap(*this, rhs);
	}

	/// \brief Copy-and-swap assignment operator
	object &operator=(object rhs) noexcept
	{
		swap(*this, rhs);
		return *this;
	}

	// --------------------------------------------------------------------

	/// \name Type queries

	///@{

	/// \brief Return true if the value is null
	[[nodiscard]] constexpr bool is_null() const noexcept { return std::holds_alternative<std::monostate>(m_data); }
	/// \brief Return true if the value is an object (string map)
	[[nodiscard]] constexpr bool is_object() const noexcept { return std::holds_alternative<object_type>(m_data); }
	/// \brief Return true if the value is an array
	[[nodiscard]] constexpr bool is_array() const noexcept { return std::holds_alternative<array_type>(m_data); }
	/// \brief Return true if the value is a string
	[[nodiscard]] constexpr bool is_string() const noexcept { return std::holds_alternative<string_type>(m_data); }
	/// \brief Return true if the value is a number (integer or float)
	[[nodiscard]] constexpr bool is_number() const noexcept { return is_number_int() or is_number_float(); }
	/// \brief Return true if the value is an integer
	[[nodiscard]] constexpr bool is_number_int() const noexcept { return std::holds_alternative<int64_t>(m_data); }
	/// \brief Return true if the value is a float
	[[nodiscard]] constexpr bool is_number_float() const noexcept { return std::holds_alternative<double>(m_data); }
	/// \brief Return true if the value is boolean, and is true
	[[nodiscard]] constexpr bool is_true() const noexcept { return is_boolean() and std::get<bool>(m_data) == true; }
	/// \brief Return true if the value is boolean, and is false
	[[nodiscard]] constexpr bool is_false() const noexcept { return is_boolean() and std::get<bool>(m_data) == false; }
	/// \brief Return true if the value is a boolean
	[[nodiscard]] constexpr bool is_boolean() const noexcept { return std::holds_alternative<bool>(m_data); }

	///@}

	/// \brief Return the underlying value_type enumerator
	[[nodiscard]] constexpr value_type type() const noexcept
	{
		return static_cast<value_type>(m_data.index());
	}

	/// \brief Truthiness conversion
	///
	/// Returns false for null, false, zero, and empty strings/arrays/objects.
	/// Returns true for all other values.
	constexpr explicit operator bool() const noexcept
	{
		bool result;
		switch (type())
		{
			case value_type::null: result = false; break;
			case value_type::boolean: result = std::get<bool>(m_data); break;
			case value_type::number_int: result = std::get<int64_t>(m_data) != 0; break;
			case value_type::number_float: result = std::get<double>(m_data) != 0; break;
			default: result = not empty(); break;
		}
		return result;
	}

	// --------------------------------------------------------------------
	/// \name Type accessors

	///@{

	/// \brief Extract the value as a string
	///
	/// If the stored type is string, returns it directly. Otherwise returns
	/// the JSON serialization of the value.
	/// \tparam T  A type satisfying \a StringType (used to select this overload)
	/// \return The string value
	template <StringType T>
	[[nodiscard]] constexpr std::string get() const
	{
		if (std::holds_alternative<string_type>(m_data))
			return std::get<string_type>(m_data);

		return get_JSON();
	}

	/// \brief Extract the value as a boolean
	///
	/// Integers and floats are considered true when non-zero, containers
	/// when non-empty.
	/// \tparam T  A type satisfying \a BooleanType (used to select this overload)
	template <BooleanType T>
	[[nodiscard]] constexpr bool get() const noexcept
	{
		switch (type())
		{
			case value_type::boolean:
				return std::get<bool>(m_data);
			case value_type::number_int:
				return std::get<int64_t>(m_data) != 0;
			case value_type::number_float:
				return std::get<double>(m_data) != 0;
			default:
				return not empty();
		}
	}

	/// \brief Extract the value as a number
	///
	/// Booleans are converted to 0 or 1. For non-numeric types returns
	/// the truthiness (0 or 1).
	/// \tparam T  A type satisfying \a NumberType (used to select this overload)
	template <NumberType T>
	[[nodiscard]] constexpr std::remove_cvref_t<T> get() const noexcept
	{
		switch (type())
		{
			case value_type::boolean:
				return std::get<bool>(m_data);
			case value_type::number_int:
				return static_cast<T>(std::get<int64_t>(m_data));
			case value_type::number_float:
				return static_cast<T>(std::get<double>(m_data));
			default:
				return not empty();
		}
	}

	///@}

	// --------------------------------------------------------------------

	/// \brief Swap the contents of two objects
	friend constexpr void swap(object &a, object &b) noexcept
	{
		std::swap(a.m_data, b.m_data);
	}

	// --------------------------------------------------------------------
	/// \name Arithmetic operators

	///@{

	/// \brief Unary negation (integer and float types only)
	constexpr object &operator-()
	{
		switch (type())
		{
			case value_type::number_int:
				std::get<int64_t>(m_data) = -std::get<int64_t>(m_data);
				break;

			case value_type::number_float:
				std::get<double>(m_data) = -std::get<double>(m_data);
				break;

			default:
				throw object_error("Can only negate numbers");
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

	///@}

	// --------------------------------------------------------------------
	/// \name Array and object interface

	///@{

	/// \brief Check if the object contains a given value
	[[nodiscard]] bool contains(const object &test) const;

	/// \brief Return true if the object is null or empty
	[[nodiscard]] constexpr bool empty() const noexcept
	{
		switch (type())
		{
			case value_type::null:
				return true;

			case value_type::array:
				return std::get<object::array_type>(m_data).empty();

			case value_type::object:
				return std::get<object_type>(m_data).empty();

			case value_type::string:
				return std::get<std::string>(m_data).empty();

			default:
				return false;
		}
	}

	/// \brief Return the number of elements
	[[nodiscard]] size_t size() const noexcept;
	/// \brief Return the maximum number of elements (from the underlying container)
	[[nodiscard]] size_t max_size() const noexcept;

	/// \brief Access a value by key (object type only) with bounds checking
	[[nodiscard]] reference at(const std::string &key);
	[[nodiscard]] const_reference at(const std::string &key) const;

	/// \brief Access a value by key (object type only), inserts a null if missing
	[[nodiscard]] reference operator[](const std::string &key);
	[[nodiscard]] const_reference operator[](const std::string &key) const;

	/// \brief Access an element by index (array type only) with bounds checking
	[[nodiscard]] reference at(size_t index);
	[[nodiscard]] const_reference at(size_t index) const;

	/// \brief Access an element by index (array type only)
	[[nodiscard]] reference operator[](size_t index);
	[[nodiscard]] const_reference operator[](size_t index) const;

	/// \brief Append a value (array type only, or null is auto-converted to array)
	void push_back(object &&val);
	void push_back(const object &val);

	/// \brief Insert a key-value pair (object type only, or null is auto-converted
	///        to object)
	template <typename... Args>
	std::pair<iterator, bool> emplace(Args &&...args)
	{
		if (is_null())
			m_data = object_type{};
		else if (not is_object())
			throw object_error("emplace only works with object type");

		auto r = std::get<object_type>(m_data).emplace(std::forward<Args>(args)...);
		auto i = begin();
		i.m_it = r.first;

		return { i, r.second };
	}

	/// \brief Append a constructed value (array type only, or null is auto-converted
	///        to array)
	template <typename... Args>
	object &emplace_back(Args &&...args)
	{
		if (not(is_null() or is_array()))
			throw object_error("emplace_back only works with array type");

		if (is_null())
			m_data = array_type{};

		return std::get<array_type>(m_data).emplace_back(std::forward<Args>(args)...);
	}

	/// \brief Erase the element at position \a pos
	template <typename Iterator>
		requires std::is_same_v<Iterator, iterator> or std::is_same_v<Iterator, const_iterator>
	Iterator erase(Iterator pos)
	{
		if (pos.m_obj != this)
			throw object_error("Invalid iterator");

		auto result = end();

		switch (type())
		{
			case value_type::array:
				result.m_it = std::get<array_type>(m_data).erase(std::get<typename Iterator::array_iterator_type>(pos.m_it));
				break;

			case value_type::object:
				result.m_it = std::get<object_type>(m_data).erase(std::get<typename Iterator::object_iterator_type>(pos.m_it));
				break;

			case value_type::null:
				throw object_error("Cannot erase in null values");

			default:
				if (std::get<difference_type>(pos.m_it) != 0)
					throw object_error("Iterator out of range");

				m_data = std::monostate{};
				result = end();
				break;
		}

		return result;
	}

	/// \brief Erase elements in the range [first, last)
	template <typename Iterator>
		requires std::is_same_v<Iterator, iterator> or std::is_same_v<Iterator, const_iterator>
	Iterator erase(Iterator first, Iterator last)
	{
		if (first.m_obj != this or last.m_obj != this)
			throw object_error("Invalid iterator");

		auto result = end();

		switch (type())
		{
			case value_type::array:
				result.m_it = std::get<array_type>(m_data).erase(std::get<typename Iterator::array_iterator_type>(first.m_it), std::get<typename Iterator::array_iterator_type>(last.m_it));
				break;

			case value_type::object:
				result.m_it = std::get<object_type>(m_data).erase(std::get<typename Iterator::object_iterator_type>(first.m_it), std::get<typename Iterator::object_iterator_type>(last.m_it));
				break;

			case value_type::null:
				throw object_error("Cannot erase in null values");

			default:
				if (std::get<difference_type>(first.m_it) != 0 or std::get<difference_type>(last.m_it) != 0)
					throw object_error("Iterator out of range");

				m_data = std::monostate{};
				result = end();
				break;
		}

		return result;
	}

	/// \brief Erase an element by key (object type only)
	/// \return Number of elements erased (0 or 1)
	size_type erase(const std::string &key)
	{
		if (is_object())
			return std::get<object_type>(m_data).erase(key);
		throw object_error("erase with a string key only works with object type");
	}

	/// \brief Erase an element by index (array type only)
	void erase(const size_type index)
	{
		if (is_array())
		{
			if (index >= size())
				throw object_error("Index out of range");
			std::get<array_type>(m_data).erase(std::get<array_type>(m_data).begin() + static_cast<difference_type>(index));
		}
		else
			throw object_error("erase with an index only works wiht array type");
	}

	///@}

	// --------------------------------------------------------------------
	/// \name Iterators

	///@{

	/// \brief Return an iterator to the first element
	[[nodiscard]] iterator begin() noexcept { return iterator(this); }
	/// \brief Return an iterator past the last element
	[[nodiscard]] iterator end() noexcept { return { this, 1 }; }

	/// \brief Return a const iterator to the first element
	[[nodiscard]] const_iterator begin() const noexcept { return const_iterator(this); }
	/// \brief Return a const iterator past the last element
	[[nodiscard]] const_iterator end() const noexcept { return { this, 1 }; }

	/// \brief Return a const iterator to the first element
	[[nodiscard]] const_iterator cbegin() noexcept { return const_iterator(this); }
	/// \brief Return a const iterator past the last element
	[[nodiscard]] const_iterator cend() noexcept { return { this, 1 }; }

	///@}

	/// \brief Access the first element (non-const)
	[[nodiscard]] object &front()
	{
		if (empty())
			throw exception("empty object");
		return *begin();
	}

	/// \brief Access the first element (const)
	[[nodiscard]] const object &front() const
	{
		if (empty())
			throw exception("empty object");
		return *begin();
	}

	/// \brief Access the last element (non-const)
	[[nodiscard]] object &back()
	{
		if (empty())
			throw exception("empty object");
		return *--end();
	}

	/// \brief Access the last element (const)
	[[nodiscard]] const object &back() const
	{
		if (empty())
			throw exception("empty object");
		return *--end();
	}

	// I/O

	/// \brief Serialize an object to an output stream as JSON
	friend void serialize(std::ostream &os, const object &o);
	/// \brief Deserialize JSON from an input stream into an object
	friend void deserialize(std::istream &is, object &o);

	/// \brief Parse JSON from an input stream
	static object parse_JSON(std::istream &is)
	{
		object result;
		deserialize(is, result);
		return result;
	}

	/// \brief Parse JSON from a string view
	static object parse_JSON(std::string_view s)
	{
		char_streambuf b(s.data(), s.length());
		std::istream is(&b);
		return parse_JSON(is);
	}

	/// \brief Serialize this object to a JSON string
	[[nodiscard]] std::string get_JSON() const
	{
		std::ostringstream os;
		serialize(os, *this);
		return os.str();
	}

	/// \brief Stream insertion operator — writes JSON to \a os
	friend std::ostream &operator<<(std::ostream &os, const object &o)
	{
		serialize(os, o);
		return os;
	}

  private:
	/// @cond

	using variant_type = std::variant<std::monostate, object_type, array_type, string_type, int64_t, double, bool>;

	variant_type m_data{};

	static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<static_cast<int>(value_type::null)>(std::declval<variant_type>()))>, std::monostate>, "Check match between value_type and variant_type");
	static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<static_cast<int>(value_type::object)>(std::declval<variant_type>()))>, object_type>, "Check match between value_type and variant_type");
	static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<static_cast<int>(value_type::array)>(std::declval<variant_type>()))>, array_type>, "Check match between value_type and variant_type");
	static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<static_cast<int>(value_type::string)>(std::declval<variant_type>()))>, std::string>, "Check match between value_type and variant_type");
	static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<static_cast<int>(value_type::number_int)>(std::declval<variant_type>()))>, int64_t>, "Check match between value_type and variant_type");
	static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<static_cast<int>(value_type::number_float)>(std::declval<variant_type>()))>, double>, "Check match between value_type and variant_type");
	static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<static_cast<int>(value_type::boolean)>(std::declval<variant_type>()))>, bool>, "Check match between value_type and variant_type");

	/// @endcond
};

} // namespace zeep::el
