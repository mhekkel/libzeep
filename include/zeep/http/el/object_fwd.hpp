//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// forward declarations required for zeep::el::object, the JSON-like object in libzeep

#include <zeep/config.hpp>

#include <array>
#include <cstdint>

namespace zeep::http::el
{

class object;

namespace detail
{

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

	inline constexpr bool operator<(value_type lhs, value_type rhs) noexcept
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

	class object_reference;

} // namespace detail

template <typename, typename>
struct object_serializer;

} // namespace zeep::http::el