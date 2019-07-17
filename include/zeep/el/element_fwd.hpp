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

namespace zeep
{
namespace el
{

class element;

namespace detail
{

enum class value_type : std::uint8_t
{
	null,
	object,
	array,
	string,
	number_int,
	number_float,
	boolean
};

inline bool operator<(value_type lhs, value_type rhs) noexcept
{
	static constexpr std::array<std::uint8_t,7> order =
	{
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
	return lix < order.size() and rix < order.size() and order[lix] < order[rix];
}

class element_reference;

}

template<typename,typename>
struct element_serializer;

}
}