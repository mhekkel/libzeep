//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/*! \file zeep/nvp.hpp
	\brief File containing the name_value_pair class
*/

#include <zeep/config.hpp>

namespace zeep
{

/// \brief class used in describing members having a name and a value
///
///	This class is very similar to the one used in boost::serialization,
/// it is used to bind a name to a member variable.

template<typename T>
class name_value_pair
{
  public:
	name_value_pair(const char* name, T& value)
		: m_name(name), m_value(value) {}

	const char* name() const		{ return m_name; }
	T&			value() const		{ return m_value; }
	const T&	const_value() const	{ return m_value; }

  private:
	const char* m_name;
	T&			m_value;
};

template<typename T>
name_value_pair<T> make_nvp(const char* name, T& v)
{
	return name_value_pair<T>(name, v);
}

} // namespace zeep
