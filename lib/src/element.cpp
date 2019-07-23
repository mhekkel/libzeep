//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>

#include <cmath>

#include <zeep/el/element.hpp>

namespace zeep
{
namespace el
{	

/// empty factory with a certain type
element::element(value_type t)
	: m_type(t), m_data(t)
{
	validate();
}

/// default factory
element::element(std::nullptr_t)
	: element(value_type::null)
{
	validate();
}

element::element(const element& j)
	: m_type(j.m_type)
{
	j.validate();
	switch (m_type)
	{
		case value_type::null:			break;
		case value_type::array:			m_data = *j.m_data.m_array; break;
		case value_type::object:		m_data = *j.m_data.m_object; break;
		case value_type::string:		m_data = *j.m_data.m_string; break;
		case value_type::number_int:	m_data = j.m_data.m_int; break;
		case value_type::number_float:	m_data = j.m_data.m_float; break;
		case value_type::boolean:		m_data = j.m_data.m_boolean; break;
	}
	validate();
}

element::element(element&& j)
	: m_type(std::move(j.m_type)), m_data(std::move(j.m_data))
{
	j.validate();

	j.m_type = value_type::null;
	j.m_data = {};

	validate();
}

element::element(const detail::element_reference& r)
	: element(r.data())
{
	validate();
}

element::element(initializer_list_t init)
{
	bool isAnObject = std::all_of(init.begin(), init.end(), [](auto& ref)
		{ return ref->is_array() and ref->m_data.m_array->size() == 2 and ref->m_data.m_array->front().is_string(); });

	if (isAnObject)
	{
		m_type = value_type::object;
		m_data = value_type::object;

		for (auto& ref: init)
		{
			auto element = ref.data();
			m_data.m_object->emplace(
				std::move(*element.m_data.m_array->front().m_data.m_string),
				std::move(element.m_data.m_array->back())
			);
		}
	}
	else
	{
		m_type = value_type::array;
		m_data.m_array = create<array_type>(init.begin(), init.end());
	}
}

element::element(size_t cnt, const element& v)
	: m_type(value_type::array)
{
	m_data.m_array = create<array_type>(cnt, v);
	validate();
}

element& element::operator=(element j) noexcept(
		std::is_nothrow_move_constructible<value_type>::value and
		std::is_nothrow_move_assignable<value_type>::value and
		std::is_nothrow_move_constructible<element_data>::value and
		std::is_nothrow_move_assignable<element_data>::value)
{
	j.validate();

	using std::swap;

	swap(m_type, j.m_type);
	swap(m_data, j.m_data);

	validate();

	return *this; 
}

element::~element() noexcept
{
	validate();
	m_data.destroy(m_type);
}

std::string element::type_name() const
{
  switch (m_type)
	{
		case value_type::null:			return "null";
		case value_type::array:			return "array";
		case value_type::object:		return "object";
		case value_type::string:		return "string";
		case value_type::number_int:	return "number_int";
		case value_type::number_float:	return "number_float";
		case value_type::boolean:		return "boolean";
        default:                        assert(false); return "";
	}
}

element::operator bool() const noexcept
{
	bool result;
	switch (m_type)
	{
		case value_type::null:			result = false; break;
		case value_type::boolean:		result = m_data.m_boolean; break;
		case value_type::number_int:	result = m_data.m_int != 0; break;
		case value_type::number_float:	result = m_data.m_float != 0; break;
		case value_type::string:		result = not m_data.m_string->empty(); break;
		default:						result = not empty(); break;
	}
	return result;
}

// array access

element::reference element::at(size_t index)
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use at()");
	
	return m_data.m_array->at(index);
}

element::const_reference element::at(size_t index) const
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use at()");
	
	return m_data.m_array->at(index);
}

element::reference element::operator[](size_t index)
{
	if (is_null())
	{
		m_type = value_type::array;
		m_data.m_array = create<array_type>();
	}
	else if (not is_array())
		throw std::runtime_error("Type should have been array to use operator[]");
	
	if (index < m_data.m_array->size())
		m_data.m_array->resize(index + 1);
	
	return m_data.m_array->operator[](index);
}

element::const_reference element::operator[](size_t index) const
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use operator[]");
	
	return m_data.m_array->operator[](index);
}

// object member access

element::reference element::at(const typename object_type::key_type& key)
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");
	
	return m_data.m_object->at(key);
}

element::const_reference element::at(const typename object_type::key_type& key) const
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");
	
	return m_data.m_object->at(key);
}

element::reference element::operator[](const typename object_type::key_type& key)
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

element::const_reference element::operator[](const typename object_type::key_type& key) const
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use operator[]");
	
	return m_data.m_object->operator[](key);
}

bool element::empty() const
{
	switch (m_type)
	{
		case value_type::null:
			return true;

		case value_type::array:
			return m_data.m_array->empty();
		
		case value_type::object:
			return m_data.m_object->empty();

		default:
			return false;
	}
}

size_t element::size() const
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

size_t element::max_size() const noexcept
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

void element::clear() noexcept
{
	switch (m_type)
	{
		case value_type::array:
			m_data.m_array->clear();
			break;

		case value_type::object:
			m_data.m_object->clear();
			break;

		case value_type::string:
			m_data.m_string->clear();
			break;

		case value_type::number_int:
			m_data.m_int = 0;
			break;

		case value_type::number_float:
			m_data.m_float = 0;
			break;

		case value_type::boolean:
			m_data.m_boolean = false;
			break;

		default:
			break;
	}
}

element::iterator element::insert(const_iterator pos, const element& val)
{
	if (is_array())
	{
		if (pos.m_obj != this)
			throw std::runtime_error("Invalid pos for array");
		return insert_iterator(pos, val);
	}
	throw std::runtime_error("Cannot use insert() with " + type_name());
}

element::iterator element::insert(const_iterator pos, size_type cnt, const element& val)
{
	if (is_array())
	{
		if (pos.m_obj != this)
			throw std::runtime_error("Invalid pos for array");
		return insert_iterator(pos, cnt, val);
	}
	throw std::runtime_error("Cannot use insert() with " + type_name());
}

element::iterator element::insert(const_iterator pos, const_iterator first, const_iterator last)
{
	if (is_array())
	{
		if (pos.m_obj != this or first.m_obj != this or last.m_obj != this)
			throw std::runtime_error("Invalid pos for array");
		return insert_iterator(pos, first.m_it.m_array_it, last.m_it.m_array_it);
	}
	throw std::runtime_error("Cannot use insert() with " + type_name());
}

element::iterator element::insert(const_iterator pos, initializer_list_t lst)
{
	if (is_array())
	{
		if (pos.m_obj != this)
			throw std::runtime_error("Invalid pos for array");
		return insert_iterator(pos, lst.begin(), lst.end());
	}
	throw std::runtime_error("Cannot use insert() with " + type_name());
}

void element::insert(const_iterator first, const_iterator last)
{
	if (not is_object())
		throw std::runtime_error("Cannot use insert() with " + type_name());

	if (first.m_obj != this or last.m_obj != this)
		throw std::runtime_error("Invalid iterator for object");

	m_data.m_object->insert(first.m_it.m_object_it, last.m_it.m_object_it);
}

void element::push_back(element&& val)
{
	if (not (is_null() or is_array()))
		throw std::runtime_error("Cannot use push_back with " + type_name());
	
	if (is_null())
	{
		m_type = value_type::array;
		m_data = value_type::array;
	}

	m_data.m_array->push_back(std::move(val));
	val.m_type = value_type::null;
}

void element::push_back(const element& val)
{
	if (not (is_null() or is_array()))
		throw std::runtime_error("Cannot use push_back with " + type_name());
	
	if (is_null())
	{
		m_type = value_type::array;
		m_data = value_type::array;
	}

	m_data.m_array->push_back(val);
}

bool element::contains(element test) const
{
	bool result = false;
	if (is_object())
		result = m_data.m_object->count(test.as<std::string>()) > 0;
	else if (is_array())
		result = std::find(m_data.m_array->begin(), m_data.m_array->end(), test) != m_data.m_array->end();

	return result;
}

bool operator==(element::const_reference& lhs, element::const_reference& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::array:			return *lhs.m_data.m_array == *rhs.m_data.m_array;
			case value_type::object:		return *lhs.m_data.m_object == *rhs.m_data.m_object;
			case value_type::string:		return *lhs.m_data.m_string == *rhs.m_data.m_string;
			case value_type::number_int:	return lhs.m_data.m_int == rhs.m_data.m_int;
			case value_type::number_float:	return lhs.m_data.m_float == rhs.m_data.m_float;
			case value_type::boolean:		return lhs.m_data.m_boolean == rhs.m_data.m_boolean;
			default: break;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return lhs.m_data.m_float == static_cast<element::float_type>(rhs.m_data.m_int);
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<element::float_type>(lhs.m_data.m_int) == rhs.m_data.m_float;
	
	return false;
}

bool operator<(element::const_reference& lhs, element::const_reference& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::array:			return *lhs.m_data.m_array < *rhs.m_data.m_array;
			case value_type::object:		return *lhs.m_data.m_object < *rhs.m_data.m_object;
			case value_type::string:		return *lhs.m_data.m_string < *rhs.m_data.m_string;
			case value_type::number_int:	return lhs.m_data.m_int < rhs.m_data.m_int;
			case value_type::number_float:	return lhs.m_data.m_float < rhs.m_data.m_float;
			case value_type::boolean:		return lhs.m_data.m_boolean < rhs.m_data.m_boolean;
			default: break;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return lhs.m_data.m_float < static_cast<element::float_type>(rhs.m_data.m_int);
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<element::float_type>(lhs.m_data.m_int) < rhs.m_data.m_float;
	
	return lhs_type < rhs_type;
}

element& element::operator-()
{
	switch (m_type)
	{
		case value_type::number_int:
			m_data.m_int = -m_data.m_int;
			break;

		case value_type::number_float:
			m_data.m_float = -m_data.m_float;
			break;

		default:
			throw std::runtime_error("Can only negate numbers");
	}

	return *this;
}

element operator+(const element& lhs, const element& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	element result;

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
	else
		throw std::runtime_error("Invalid types for operator +");

	return result;
}

element operator-(const element& lhs, const element& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	element result;

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

element operator*(const element& lhs, const element& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	element result;

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

element operator/(const element& lhs, const element& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	element result;

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

element operator%(const element& lhs, const element& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	element result;

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

template<>
std::string element::as<std::string>() const
{
	if (type() == value_type::string)
		return *m_data.m_string;
	
	std::ostringstream s;
	s << *this;
	return s.str();
}

template<>
bool element::as<bool>() const
{
	switch (m_type)
	{
		case value_type::boolean:
			return m_data.m_boolean;
		case value_type::number_int:
			return m_data.m_int != 0;
		case value_type::number_float:
			return m_data.m_float != 0;
		default:
			return not empty();
	}
}

// --------------------------------------------------------------------

void serialize(std::ostream& os, const element& v)
{
	switch (v.m_type) 
	{
		case element::value_type::array:
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

		case element::value_type::boolean:
			os << std::boolalpha << v.m_data.m_boolean;
			break;

		case element::value_type::null:
			os << "null";
			break;

		case element::value_type::number_float:
			if (std::isnan(v.m_data.m_float))
				os << "NaN";
			else
				os << v.m_data.m_float;
			break;

		case element::value_type::number_int:
			os << v.m_data.m_int;
			break;

		case element::value_type::object:
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

		case element::value_type::string:
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

// void serialize(std::ostream& os, const element& v, int indent, int level)
// {
//     switch (v.m_type)
//     {
//         case value_type::array:
//         {
//             auto& a = *v.m_data.m_array;
//             if (a.empty())
//                 os << std::setw(level * indent) << "[]" << std::endl;
//             else
//             {
//                 os << std::setw(level * indent) << '[' << std::endl;
//                 for (size_t i = 0; i < a.size(); ++i)
//                 {
//                     serialize(os, a[i], indent, level + 1);
//                     if (i + 1 < a.size())
//                         os << ',';
//                     os << std::endl;
//                 }
//                 os << std::setw(level * indent) << ']';
//             }
//             break;
//         }

//         case value_type::boolean:
//         {
//             auto& b = *v.m_data.m_boolean;
//             os << std::setw(level * indent) << std::boolalpha << b;
//             break;
//         }

//         case value_type::null:
//             os << std::setw(level * indent) << "null";
//             break;

//         case value_type::number_float:
//             os << std::setw(level * indent) << v.m_data.m_float;
//             break;

//         case value_type::number_int:
//             os << std::setw(level * indent) << v.m_data.m_int;
//             break;

//         case value_type::number_uint:
//             os << std::setw(level * indent) << v.m_data.m_uint;
//             break;

//         case value_type::object:
//         {
//             auto& o = *v.m_data.m_object;
//             if (o.empty())
//                 os << std::setw(level * indent) << "{}" << std::endl;
//             else
//             {
//                 os << std::setw(level * indent) << '{' << std::endl;
//                 for (size_t i = 0; i < a.size(); ++i)
//                 {


//                     os << 
//                     serialize(os, a[i], indent, level + 1);
//                     if (i + 1 < a.size())
//                         os << ',';
//                     os << std::endl;
//                 }
//                 os << std::setw(level * indent) << ']';
//             }
//             break;
//         }

//         case value_type::string:
//             os << std::setw(level * indent) << *v.m_data.m_string;
//             break;
//     }
// }

// --------------------------------------------------------------------

element parse(const std::string& s)
{
	return element();
}

std::ostream& operator<<(std::ostream& os, const element& v)
{
	// int indentation = os.width();
	// os.width(0);

	serialize(os, v);

	return os;
}
}
} // namespace zeep