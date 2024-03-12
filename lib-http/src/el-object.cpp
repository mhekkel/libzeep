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

#include "zeep/http/el-object.hpp"

namespace zeep::http
{

object operator+(const object &lhs, const object &rhs)
{
	using value_type = object::value_type;
	
	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	object result;

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::boolean:
			case value_type::number_int:
				result = lhs.m_data.m_int + rhs.m_data.m_int;
				break;

			case value_type::number_float:
				result = lhs.m_data.m_float + rhs.m_data.m_float;
				break;

			case value_type::string:
				result = *lhs.m_data.m_string + *rhs.m_data.m_string;
				break;

			case value_type::null:
				break;

			default:
				throw std::runtime_error("Invalid types for operator +");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = lhs.m_data.m_float + rhs.as<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
		result = lhs.m_data.m_int + rhs.as<int64_t>();
	else if (lhs_type == value_type::null)
		result = rhs;
	else if (rhs_type == value_type::null)
		result = lhs;
	else if (lhs_type == value_type::string or rhs_type == value_type::string)
		result = lhs.as<std::string>() + rhs.as<std::string>();
	else
		throw std::runtime_error("Invalid types for operator +");

	return result;
}

object operator-(const object &lhs, const object &rhs)
{
	using value_type = object::value_type;
	
	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	object result;

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::boolean:
			case value_type::number_int:
				result = lhs.m_data.m_int - rhs.m_data.m_int;
				break;

			case value_type::number_float:
				result = lhs.m_data.m_float - rhs.m_data.m_float;
				break;

			default:
				throw std::runtime_error("Invalid types for operator -");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = lhs.m_data.m_float - rhs.as<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
		result = lhs.m_data.m_int - rhs.as<int64_t>();
	else
		throw std::runtime_error("Invalid types for operator -");

	return result;
}

object operator*(const object &lhs, const object &rhs)
{
	using value_type = object::value_type;
	
	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	object result;

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::boolean:
			case value_type::number_int:
				result = lhs.m_data.m_int * rhs.m_data.m_int;
				break;

			case value_type::number_float:
				result = lhs.m_data.m_float * rhs.m_data.m_float;
				break;

			default:
				throw std::runtime_error("Invalid types for operator *");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = lhs.m_data.m_float * rhs.as<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
		result = lhs.m_data.m_int * rhs.as<int64_t>();
	else
		throw std::runtime_error("Invalid types for operator *");

	return result;
}

object operator/(const object &lhs, const object &rhs)
{
	using value_type = object::value_type;
	
	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	object result;

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::boolean:
			case value_type::number_int:
				result = lhs.m_data.m_int / rhs.m_data.m_int;
				break;

			case value_type::number_float:
				result = lhs.m_data.m_float / rhs.m_data.m_float;
				break;

			default:
				throw std::runtime_error("Invalid types for operator /");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = lhs.m_data.m_float / rhs.as<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
		result = lhs.m_data.m_int / rhs.as<int64_t>();
	else
		throw std::runtime_error("Invalid types for operator /");

	return result;
}

object operator%(const object &lhs, const object &rhs)
{
	using value_type = object::value_type;
	
	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	object result;

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::boolean:
			case value_type::number_int:
				result = lhs.m_data.m_int % rhs.m_data.m_int;
				break;

			default:
				throw std::runtime_error("Invalid types for operator %");
		}
	}
	else if (lhs_type == value_type::number_int and rhs.is_number())
		result = lhs.m_data.m_int % rhs.as<int64_t>();
	else
		throw std::runtime_error("Invalid types for operator %");

	return result;
}

bool operator==(const object &lhs, const object &rhs) noexcept
{
	using value_type = object::value_type;
	
	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::array: return *lhs.m_data.m_array == *rhs.m_data.m_array;
			case value_type::object: return *lhs.m_data.m_object == *rhs.m_data.m_object;
			case value_type::string: return *lhs.m_data.m_string == *rhs.m_data.m_string;
			case value_type::number_int: return lhs.m_data.m_int == rhs.m_data.m_int;
			case value_type::number_float: return lhs.m_data.m_float == rhs.m_data.m_float;
			case value_type::boolean: return lhs.m_data.m_boolean == rhs.m_data.m_boolean;
			case value_type::null: return true;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return lhs.m_data.m_float == static_cast<object::float_type>(rhs.m_data.m_int);
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<object::float_type>(lhs.m_data.m_int) == rhs.m_data.m_float;

	return false;
}

std::partial_ordering operator<=>(const object &lhs, const object &rhs) noexcept
{
	using value_type = object::value_type;
	
	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::array: return *lhs.m_data.m_array <=> *rhs.m_data.m_array;
			case value_type::object: return *lhs.m_data.m_object <=> *rhs.m_data.m_object;
			case value_type::string: return *lhs.m_data.m_string <=> *rhs.m_data.m_string;
			case value_type::number_int: return lhs.m_data.m_int <=> rhs.m_data.m_int;
			case value_type::number_float: return lhs.m_data.m_float <=> rhs.m_data.m_float;
			case value_type::boolean: return lhs.m_data.m_boolean <=> rhs.m_data.m_boolean;
			default: break;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return lhs.m_data.m_float <=> static_cast<object::float_type>(rhs.m_data.m_int);
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<object::float_type>(lhs.m_data.m_int) <=> rhs.m_data.m_float;

	return lhs_type <=> rhs_type;
}

// --------------------------------------------------------------------

size_t object::size() const noexcept
{
	switch (m_type)
	{
		case value_type::null:
			return 0;

		case value_type::array:
			return m_data.m_array->size();
		
		case value_type::object:
			return m_data.m_object->size();

		default:
			return 1;
	}
}

size_t object::max_size() const noexcept
{
	switch (m_type)
	{
		case value_type::array:
			return m_data.m_array->max_size();
		
		case value_type::object:
			return m_data.m_object->max_size();

		default:
			return size();
	}
}

void object::push_back(object&& val)
{
	if (not (is_null() or is_array()))
		throw std::runtime_error("Invalid type for push_back");
	
	if (is_null())
	{
		m_type = value_type::array;
		m_data = value_type::array;
	}

	m_data.m_array->push_back(std::move(val));
	val.m_type = value_type::null;
}

void object::push_back(const object& val)
{
	if (not (is_null() or is_array()))
		throw std::runtime_error("Invalid type for push_back");
	
	if (is_null())
	{
		m_type = value_type::array;
		m_data = value_type::array;
	}

	m_data.m_array->push_back(val);
}

object::reference object::at(size_t index)
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use at()");
	
	return m_data.m_array->at(index);
}

object::const_reference object::at(size_t index) const
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use at()");
	
	return m_data.m_array->at(index);
}

bool object::contains(object test) const
{
	bool result = false;
	if (is_object())
		result = m_data.m_object->count(test.as<std::string>()) > 0;
	else if (is_array())
		result = std::find(m_data.m_array->begin(), m_data.m_array->end(), test) != m_data.m_array->end();

	return result;
}

object::reference object::operator[](size_t index)
{
	if (is_null())
	{
		m_type = value_type::array;
		m_data.m_array = create<array_type>();
	}
	else if (not is_array())
		throw std::runtime_error("Type should have been array to use operator[]");
	
	if (index + 1 > m_data.m_array->size())
		m_data.m_array->resize(index + 1);
	
	return m_data.m_array->operator[](index);
}

object::const_reference object::operator[](size_t index) const
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use operator[]");
	
	return m_data.m_array->operator[](index);
}

// object member access

object::reference object::at(const typename object_type::key_type& key)
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");
	
	return m_data.m_object->at(key);
}

object::const_reference object::at(const typename object_type::key_type& key) const
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");
	
	return m_data.m_object->at(key);
}

object::reference object::operator[](const typename object_type::key_type& key)
{
	if (is_null())
	{
		m_type = value_type::object;
		m_data.m_object = create<object_type>();
	}
	else if (not is_object())
		throw std::runtime_error("Type should have been object to use operator[]");
	
	return m_data.m_object->operator[](key);
}

object::const_reference object::operator[](const typename object_type::key_type& key) const
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use operator[]");
	
	return m_data.m_object->operator[](key);
}

bool object::empty() const noexcept
{
	switch (m_type)
	{
		case value_type::null:
			return true;

		case value_type::array:
			return m_data.m_array->empty();
		
		case value_type::object:
			return m_data.m_object->empty();

		case value_type::string:
			return m_data.m_string->empty();

		default:
			return false;
	}
}

// --------------------------------------------------------------------

void serialize(std::ostream& os, const object& v)
{
	switch (v.m_type) 
	{
		case object::value_type::array:
		{
			auto& a = *v.m_data.m_array;
			os << '[';
			for (size_t i = 0; i < a.size(); ++i)
			{
				serialize(os, a[i]);
				if (i + 1 < a.size())
					os << ',';
			}
			os << ']';
			break;
		}

		case object::value_type::boolean:
			os << std::boolalpha << v.m_data.m_boolean;
			break;

		case object::value_type::null:
			os << "null";
			break;

		case object::value_type::number_float:
			if (v.m_data.m_float == 0 or std::isnormal(v.m_data.m_float))
				os << v.m_data.m_float;
			else
				// os << "\"NaN\"";
				os << "null";
			break;

		case object::value_type::number_int:
			os << v.m_data.m_int;
			break;

		case object::value_type::object:
		{
			os << '{';
			bool first = true;
			for (auto& kv: *v.m_data.m_object)
			{
				if (not first)
					os << ',';
				os << '"' << kv.first << "\":";
				serialize(os, kv.second);
				first = false;
			}
			os << '}';
			break;
		}

		case object::value_type::string:
			os << '"';
			
			for (uint8_t c: *v.m_data.m_string)
			{
				switch (c)
				{
					case '\"':	os << "\\\""; break;
					case '\\':	os << "\\\\"; break;
					case '/':	os << "\\/"; break;
					case '\b':	os << "\\b"; break;
					case '\n':	os << "\\n"; break;
					case '\r':	os << "\\r"; break;
					case '\t':	os << "\\t"; break;
					default:	if (c <  0x0020)
								{
									static const char kHex[17] = "0123456789abcdef";
									os << "\\u00" << kHex[(c >> 4) & 0x0f] << kHex[c & 0x0f];
								}
								else	
									os << c;
								break;
				}
			}
			
			os << '"';
			break;
	}
}

// --------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const object& v)
{
	serialize(os, v);
	return os;
}


} // namespace zeep::http
