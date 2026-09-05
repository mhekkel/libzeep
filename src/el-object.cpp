// SPDX-FileCopyrightText: Maarten L. Hekkelman 2025
// SPDX-License-Identifier: BSL-1.0

#ifndef ZEEP_CXX_MODULE
# include "zeep/el/object.hpp"
# include "zeep/exception.hpp"
# include "zeep/unicode-support.hpp"

# include <algorithm>
# include <cassert>
# include <cctype>
# include <charconv>
# include <climits>
# include <cmath>
# include <compare>
# include <cstddef>
# include <cstdint>
# include <format>
# include <istream>
# include <limits>
# include <map>
# include <string>
# include <system_error>
# include <utility>
# include <vector>
#endif

namespace zeep::el
{
// --- overflow helpers for signed int64_t arithmetic ---

constexpr bool add_overflows(int64_t a, int64_t b)
{
	constexpr auto max = std::numeric_limits<int64_t>::max();
	constexpr auto min = std::numeric_limits<int64_t>::min();

	if (b > 0)
		return a > max - b;
	if (b < 0)
		return a < min - b;
	return false;
}

constexpr bool sub_overflows(int64_t a, int64_t b)
{
	constexpr auto max = std::numeric_limits<int64_t>::max();
	constexpr auto min = std::numeric_limits<int64_t>::min();

	if (b > 0)
		return a < min + b;
	if (b < 0)
		return a > max + b;
	return false;
}

constexpr bool mul_overflows(int64_t a, int64_t b)
{
	constexpr auto max = std::numeric_limits<int64_t>::max();
	constexpr auto min = std::numeric_limits<int64_t>::min();

	if (a == 0 or b == 0 or a == 1 or b == 1)
		return false;
	if (a == -1)
		return b == min;
	if (b == -1)
		return a == min;

	if (a > 0 and b > 0)
		return a > max / b;
	if (a < 0 and b < 0)
		return a < max / b;
	if (b < 0)
		return a > min / b;
	return a < min / b;
}

const object g_null_object{}; // To be returned in operator[] for const object

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
			case value_type::number_int:
			{
				auto a = std::get<int64_t>(lhs.m_data), b = std::get<int64_t>(rhs.m_data);
				if (add_overflows(a, b))
					throw object_error("Integer overflow in operator +");
				result = a + b;
			}
			break;

			case value_type::number_float:
				result = std::get<double>(lhs.m_data) + std::get<double>(rhs.m_data);
				break;

			case value_type::string:
				result = std::get<std::string>(lhs.m_data) + std::get<std::string>(rhs.m_data);
				break;

			case value_type::null:
				break;

			default:
				throw object_error("Invalid types for operator +");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = std::get<double>(lhs.m_data) + rhs.get<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
	{
		auto a = std::get<int64_t>(lhs.m_data), b = rhs.get<int64_t>();
		if (add_overflows(a, b))
			throw object_error("Integer overflow in operator +");
		result = a + b;
	}
	else if (lhs_type == value_type::null)
		result = rhs;
	else if (rhs_type == value_type::null)
		result = lhs;
	else if (lhs_type == value_type::string or rhs_type == value_type::string)
		result = lhs.get<std::string>() + rhs.get<std::string>();
	else
		throw object_error("Invalid types for operator +");

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
			case value_type::number_int:
			{
				auto a = std::get<int64_t>(lhs.m_data), b = std::get<int64_t>(rhs.m_data);
				if (sub_overflows(a, b))
					throw object_error("Integer overflow in operator -");
				result = a - b;
			}
			break;

			case value_type::number_float:
				result = std::get<double>(lhs.m_data) - std::get<double>(rhs.m_data);
				break;

			default:
				throw object_error("Invalid types for operator -");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = std::get<double>(lhs.m_data) - rhs.get<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
	{
		auto a = std::get<int64_t>(lhs.m_data), b = rhs.get<int64_t>();
		if (sub_overflows(a, b))
			throw object_error("Integer overflow in operator -");
		result = a - b;
	}
	else
		throw object_error("Invalid types for operator -");

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
			case value_type::number_int:
			{
				auto a = std::get<int64_t>(lhs.m_data), b = std::get<int64_t>(rhs.m_data);
				if (mul_overflows(a, b))
					throw object_error("Integer overflow in operator *");
				result = a * b;
			}
			break;

			case value_type::number_float:
				result = std::get<double>(lhs.m_data) * std::get<double>(rhs.m_data);
				break;

			default:
				throw object_error("Invalid types for operator *");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = std::get<double>(lhs.m_data) * rhs.get<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
	{
		auto a = std::get<int64_t>(lhs.m_data), b = rhs.get<int64_t>();
		if (mul_overflows(a, b))
			throw object_error("Integer overflow in operator *");
		result = a * b;
	}
	else
		throw object_error("Invalid types for operator *");

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
			case value_type::number_int:
				if (auto denom = std::get_if<int64_t>(&rhs.m_data); denom and *denom)
				{
					auto a = std::get<int64_t>(lhs.m_data), b = *denom;
					if (a == std::numeric_limits<int64_t>::min() and b == -1)
						throw object_error("Integer overflow in operator /");
					result = a / b;
				}
				else
					throw object_error("Division by zero");
				break;

			case value_type::number_float:
				if (auto denom = std::get_if<double>(&rhs.m_data); denom and *denom != 0)
					result = std::get<double>(lhs.m_data) / *denom;
				else
					throw object_error("Division by zero");
				break;

			default:
				throw object_error("Invalid types for operator /");
		}
	}
	else if (lhs_type == value_type::number_float and rhs.is_number())
		result = std::get<double>(lhs.m_data) / rhs.get<double>();
	else if (lhs_type == value_type::number_int and rhs.is_number())
	{
		auto denom = rhs.get<int64_t>();
		if (denom != 0)
		{
			auto a = std::get<int64_t>(lhs.m_data);
			if (a == std::numeric_limits<int64_t>::min() and denom == -1)
				throw object_error("Integer overflow in operator /");
			result = a / denom;
		}
		else
			throw object_error("Division by zero");
	}
	else
		throw object_error("Invalid types for operator /");

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
			case value_type::number_int:
				if (auto denom = std::get_if<int64_t>(&rhs.m_data); denom and *denom)
				{
					auto a = std::get<int64_t>(lhs.m_data), b = *denom;
					if (a == std::numeric_limits<int64_t>::min() and b == -1)
						throw object_error("Integer overflow in operator %");
					result = a % b;
				}
				else
					throw object_error("Modulo by zero");
				break;

			default:
				throw object_error("Invalid types for operator %");
		}
	}
	else if (lhs_type == value_type::number_int and rhs.is_number())
	{
		auto denom = rhs.get<int64_t>();
		if (denom != 0)
		{
			auto a = std::get<int64_t>(lhs.m_data);
			if (a == std::numeric_limits<int64_t>::min() and denom == -1)
				throw object_error("Integer overflow in operator %");
			result = a % denom;
		}
		else
			throw object_error("Modulo by zero");
	}
	else
		throw object_error("Invalid types for operator %");

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
			case value_type::array: return *std::get_if<object::array_type>(&lhs.m_data) == *std::get_if<object::array_type>(&rhs.m_data);
			case value_type::object: return *std::get_if<object::object_type>(&lhs.m_data) == *std::get_if<object::object_type>(&rhs.m_data);
			case value_type::string: return *std::get_if<std::string>(&lhs.m_data) == *std::get_if<std::string>(&rhs.m_data);
			case value_type::number_int: return *std::get_if<int64_t>(&lhs.m_data) == *std::get_if<int64_t>(&rhs.m_data);
			case value_type::number_float: return *std::get_if<double>(&lhs.m_data) == *std::get_if<double>(&rhs.m_data);
			case value_type::boolean: return *std::get_if<bool>(&lhs.m_data) == *std::get_if<bool>(&rhs.m_data);
			case value_type::null: return true;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return *std::get_if<double>(&lhs.m_data) == static_cast<object::float_type>(*std::get_if<int64_t>(&rhs.m_data));
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<object::float_type>(*std::get_if<int64_t>(&lhs.m_data)) == *std::get_if<double>(&rhs.m_data);

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
			case value_type::array: return *std::get_if<object::array_type>(&lhs.m_data) <=> *std::get_if<object::array_type>(&rhs.m_data);
			case value_type::object: return *std::get_if<object::object_type>(&lhs.m_data) <=> *std::get_if<object::object_type>(&rhs.m_data);
			case value_type::string: return *std::get_if<std::string>(&lhs.m_data) <=> *std::get_if<std::string>(&rhs.m_data);
			case value_type::number_int: return *std::get_if<int64_t>(&lhs.m_data) <=> *std::get_if<int64_t>(&rhs.m_data);
			case value_type::number_float: return *std::get_if<double>(&lhs.m_data) <=> *std::get_if<double>(&rhs.m_data);
			case value_type::boolean: return *std::get_if<bool>(&lhs.m_data) <=> *std::get_if<bool>(&rhs.m_data);
			default: break;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return *std::get_if<double>(&lhs.m_data) <=> static_cast<object::float_type>(*std::get_if<int64_t>(&rhs.m_data));
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<object::float_type>(*std::get_if<int64_t>(&lhs.m_data)) <=> *std::get_if<double>(&rhs.m_data);

	return lhs_type <=> rhs_type;
}

// --------------------------------------------------------------------

size_t object::size() const noexcept
{
	switch (type())
	{
		case value_type::null:
			return 0;

		case value_type::array:
			return std::get_if<array_type>(&m_data)->size();

		case value_type::object:
			return std::get_if<object_type>(&m_data)->size();

		default:
			return 1;
	}
}

size_t object::max_size() const noexcept
{
	switch (type())
	{
		case value_type::array:
			return std::get_if<array_type>(&m_data)->max_size();

		case value_type::object:
			return std::get_if<object_type>(&m_data)->max_size();

		default:
			return size();
	}
}

void object::push_back(object &&val)
{
	if (not(is_null() or is_array()))
		throw object_error("Invalid type for push_back");

	if (is_null())
		m_data = array_type{};

	std::get<object::array_type>(m_data).push_back(std::move(val));
}

void object::push_back(const object &val)
{
	if (not(is_null() or is_array()))
		throw object_error("Invalid type for push_back");

	if (is_null())
		m_data = array_type{};

	std::get<object::array_type>(m_data).push_back(val);
}

object::reference object::at(size_t index)
{
	if (not is_array())
		throw object_error("Type should have been array to use at()");
	return std::get<object::array_type>(m_data).at(index);
}

object::const_reference object::at(size_t index) const
{
	if (not is_array())
		throw object_error("Type should have been array to use at()");
	return std::get<object::array_type>(m_data).at(index);
}

bool object::contains(const object &test) const
{
	bool result = false;
	if (is_object())
		result = std::get<object_type>(m_data).count(test.get<std::string>()) > 0;
	else if (is_array())
		result = std::ranges::contains(std::get<array_type>(m_data), test);

	return result;
}

object::reference object::operator[](size_t index)
{
	if (is_null())
		m_data = array_type{};
	else if (not is_array())
		throw object_error("Type should have been array to use operator[]");

	if (index >= size())
	{
		if (index == std::numeric_limits<size_t>::max())
			throw object_error("Array index out of range in operator[]");
		std::get<array_type>(m_data).resize(index + 1);
	}

	return std::get<object::array_type>(m_data).operator[](index);
}

object::const_reference object::operator[](size_t index) const
{
	if (not is_array())
		throw object_error("Type should have been array to use operator[]");

	return index >= size()
	           ? g_null_object
	           : std::get<object::array_type>(m_data).operator[](index);
}

// object member access

object::reference object::at(const typename object_type::key_type &key)
{
	if (not is_object())
		throw object_error("Type should have been object to use at()");

	return std::get<object_type>(m_data).at(key);
}

object::const_reference object::at(const typename object_type::key_type &key) const
{
	if (not is_object())
		throw object_error("Type should have been object to use at()");

	return std::get<object_type>(m_data).at(key);
}

object::reference object::operator[](const typename object_type::key_type &key)
{
	if (is_null())
		m_data = object_type{};
	else if (not is_object())
		throw object_error("Type should have been object to use operator[]");

	return std::get<object_type>(m_data).operator[](key);
}

object::const_reference object::operator[](const typename object_type::key_type &key) const
{
	if (not is_object())
		throw object_error("Type should have been object to use operator[]");

	auto i = std::get<object_type>(m_data).find(key);
	return i == std::get<object_type>(m_data).end()
	           ? g_null_object
	           : i->second;
}

// --------------------------------------------------------------------

void serialize(std::ostream &os, const object &v)
{
	switch (v.type())
	{
		case object::value_type::array:
		{
			auto &a = std::get<object::array_type>(v.m_data);
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
			os << std::boolalpha << std::get<bool>(v.m_data);
			break;

		case object::value_type::null:
			os << "null";
			break;

		case object::value_type::number_float:
			if (std::get<double>(v.m_data) == 0 or std::isnormal(std::get<double>(v.m_data)))
				os << std::get<double>(v.m_data);
			else
				// os << "\"NaN\"";
				os << "null";
			break;

		case object::value_type::number_int:
			os << std::get<int64_t>(v.m_data);
			break;

		case object::value_type::object:
		{
			os << '{';
			bool first = true;
			for (const auto &[key, value] : std::get<object::object_type>(v.m_data))
			{
				if (not first)
					os << ',';

				os << '"';
				for (uint8_t c : key)
				{
					switch (c)
					{
						case '\"': os << "\\\""; break;
						case '\\': os << "\\\\"; break;
						case '/': os << "\\/"; break;
						case '\b': os << "\\b"; break;
						case '\n': os << "\\n"; break;
						case '\r': os << "\\r"; break;
						case '\t': os << "\\t"; break;
						case '\f': os << "\\f"; break;
						default:
							if (c < 0x0020)
							{
								static const char kHex[17] = "0123456789abcdef";
								os << "\\u00" << kHex[(c >> 4) & 0x0f] << kHex[c & 0x0f];
							}
							else
								os << static_cast<char>(c);
							break;
					}
				}

				os << "\":";
				serialize(os, value);
				first = false;
			}
			os << '}';
			break;
		}

		case object::value_type::string:
			os << '"';

			for (uint8_t c : std::get<std::string>(v.m_data))
			{
				switch (c)
				{
					case '\"': os << "\\\""; break;
					case '\\': os << "\\\\"; break;
					case '/': os << "\\/"; break;
					case '\b': os << "\\b"; break;
					case '\n': os << "\\n"; break;
					case '\r': os << "\\r"; break;
					case '\t': os << "\\t"; break;
					default:
						if (c < 0x0020)
						{
							static const char kHex[17] = "0123456789abcdef";
							os << "\\u00" << kHex[(c >> 4) & 0x0f] << kHex[c & 0x0f];
						}
						else
							os << static_cast<char>(c);
						break;
				}
			}

			os << '"';
			break;
	}
}

// --------------------------------------------------------------------
// deserialize is in fact a JSON parser :-)

class json_parser
{
  public:
	json_parser(std::istream &is)
		: m_is(is)
	{
	}

	void parse(object &object);

  private:
	enum class token_t : uint8_t
	{
		Eof,
		LeftBrace,
		RightBrace,
		LeftBracket,
		RightBracket,
		Comma,
		Colon,
		String,
		Integer,
		Number,
		True,
		False,
		Null,
		Undef
	};

	[[nodiscard]] std::string describe_token(token_t t) const
	{
		switch (t)
		{
			case token_t::Eof: return "end of data";
			case token_t::LeftBrace: return "left brace ('{')";
			case token_t::RightBrace: return "richt brace ('}')";
			case token_t::LeftBracket: return "left bracket ('[')";
			case token_t::RightBracket: return "right bracket (']')";
			case token_t::Comma: return "comma";
			case token_t::Colon: return "colon";
			case token_t::String: return "string";
			case token_t::Integer: return "integer";
			case token_t::Number: return "number";
			case token_t::True: return "true";
			case token_t::False: return "false";
			case token_t::Null: return "null";
			case token_t::Undef: return "undefined token";
			default: assert(false); return "???";
		}
	}

	void match(token_t expected);

	void parse_value(object &e);
	void parse_object(object &e);
	void parse_array(object &e);

	[[nodiscard]] uint8_t get_next_byte();
	[[nodiscard]] char32_t get_next_unicode();
	[[nodiscard]] char32_t get_next_char();
	void retract();

	[[nodiscard]] token_t get_next_token();

	std::istream &m_is;

	// a minimal stack for ungetc like operations
	char32_t m_buffer[2]{};
	char32_t *m_buffer_ptr = m_buffer;

	// recursion depth guard to prevent a stack overflow on attacker-supplied,
	// deeply nested input (e.g. an unauthenticated JWT header or request body)
	static constexpr size_t kMaxDepth = 1000;
	size_t m_depth = 0;

	std::string m_token;
	double m_token_float{};
	int64_t m_token_int{};
	token_t m_lookahead{ token_t::Eof };
};

uint8_t json_parser::get_next_byte()
{
	int result = m_is.rdbuf()->sbumpc();

	if (result == std::streambuf::traits_type::eof())
		result = 0;

	return static_cast<uint8_t>(result);
}

char32_t json_parser::get_next_unicode()
{
	char32_t result = get_next_byte();

	if (result & 0x080)
	{
		unsigned char ch[3];

		if ((result & 0x0E0) == 0x0C0)
		{
			ch[0] = get_next_byte();
			if ((ch[0] & 0x0c0) != 0x080)
				throw object_error("Invalid utf-8");
			result = ((result & 0x01F) << 6) | (ch[0] & 0x03F);
		}
		else if ((result & 0x0F0) == 0x0E0)
		{
			ch[0] = get_next_byte();
			ch[1] = get_next_byte();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080)
				throw object_error("Invalid utf-8");
			result = ((result & 0x00F) << 12) | ((ch[0] & 0x03F) << 6) | (ch[1] & 0x03F);
		}
		else if ((result & 0x0F8) == 0x0F0)
		{
			ch[0] = get_next_byte();
			ch[1] = get_next_byte();
			ch[2] = get_next_byte();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
				throw object_error("Invalid utf-8");
			result = ((result & 0x007) << 18) | ((ch[0] & 0x03F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);

			if (result > 0x10ffff)
				throw object_error("invalid utf-8 character (out of range)");
		}
	}

	return result;
}

char32_t json_parser::get_next_char()
{
	char32_t result = 0;

	if (m_buffer_ptr > m_buffer) // if buffer is not empty we already did all the validity checks
		result = *--m_buffer_ptr;
	else
	{
		result = get_next_unicode();

		if (result >= 0x080)
		{
			if (result == 0x0ffff or result == 0x0fffe)
			{
				using namespace std::literals;

				char s[32] = {};
				if (auto r = std::to_chars(s, s + sizeof(s), result, 16); r.ec == std::errc{})
					throw object_error("character 0x"s + s + " is not allowed");
				else
					throw object_error("character "s + std::to_string(result) + " is not allowed");
			}

			// surrogate support
			else if (result >= 0x0D800 and result <= 0x0DBFF)
			{
				char32_t uc2 = get_next_char();
				if (uc2 >= 0x0DC00 and uc2 <= 0x0DFFF)
					result = (result - 0x0D800) * 0x400 + (uc2 - 0x0DC00) + 0x010000;
				else
					throw object_error("leading surrogate character without trailing surrogate character");
			}
			else if (result >= 0x0DC00 and result <= 0x0DFFF)
				throw object_error("trailing surrogate character without a leading surrogate");
		}
	}

	//	append(m_token, result);
	// somehow, append refuses to inline, so we have to do it ourselves
	if (result < 0x080)
		m_token += (static_cast<char>(result));
	else if (result < 0x0800)
	{
		char ch[2] = {
			static_cast<char>(0x0c0 | (result >> 6)),
			static_cast<char>(0x080 | (result & 0x3f))
		};
		m_token.append(ch, 2);
	}
	else if (result < 0x00010000)
	{
		char ch[3] = {
			static_cast<char>(0x0e0 | (result >> 12)),
			static_cast<char>(0x080 | ((result >> 6) & 0x3f)),
			static_cast<char>(0x080 | (result & 0x3f))
		};
		m_token.append(ch, 3);
	}
	else
	{
		char ch[4] = {
			static_cast<char>(0x0f0 | (result >> 18)),
			static_cast<char>(0x080 | ((result >> 12) & 0x3f)),
			static_cast<char>(0x080 | ((result >> 6) & 0x3f)),
			static_cast<char>(0x080 | (result & 0x3f))
		};
		m_token.append(ch, 4);
	}

	return result;
}

void json_parser::retract()
{
	assert(not m_token.empty());
	*m_buffer_ptr++ = pop_last_char(m_token);
}

auto json_parser::get_next_token() -> token_t
{
	enum class state_t
	{
		Start,
		Negative,
		Zero,
		Number,
		NumberFraction,
		NumberExpSign,
		NumberExpDigit1,
		NumberExpDigit2,
		Literal,
		String,
		Escape,
		EscapeHex1,
		EscapeHex2,
		EscapeHex3,
		EscapeHex4,

		Surrogate2EscapeStart1,
		Surrogate2EscapeStart2,
		Surrogate2EscapeHex1,
		Surrogate2EscapeHex2,
		Surrogate2EscapeHex3,
		Surrogate2EscapeHex4,
	} state = state_t::Start;

	token_t token = token_t::Undef;
	double fraction = 1.0, exponent = 1;
	bool negative = false, negativeExp = false;

	char32_t hx = {}, surrogate = {};

	m_token.clear();

	while (token == token_t::Undef)
	{
		char32_t ch = get_next_char();

		switch (state)
		{
			case state_t::Start:
				switch (ch)
				{
					case 0:
						token = token_t::Eof;
						break;
					case '{':
						token = token_t::LeftBrace;
						break;
					case '}':
						token = token_t::RightBrace;
						break;
					case '[':
						token = token_t::LeftBracket;
						break;
					case ']':
						token = token_t::RightBracket;
						break;
					case ',':
						token = token_t::Comma;
						break;
					case ':':
						token = token_t::Colon;
						break;
					case ' ':
					case '\n':
					case '\r':
					case '\t':
						m_token.clear();
						break;
					case '"':
						m_token.pop_back();
						state = state_t::String;
						break;
					case '-':
						state = state_t::Negative;
						break;
					default:
						if (ch == '0')
						{
							state = state_t::Zero;
							m_token_int = 0;
						}
						else if (ch >= '1' and ch <= '9')
						{
							m_token_int = ch - '0';
							state = state_t::Number;
						}
						else if (ch < 128 and std::isalpha(static_cast<int>(ch)))
							state = state_t::Literal;
						else if (ch < UCHAR_MAX and std::isprint(static_cast<int>(ch)))
							throw zeep::exception(std::format("Invalid character '{}' in json", static_cast<char>(ch)));
						else
							throw zeep::exception(std::format("Invalid character '0x{:x}' in json", static_cast<int>(ch)));
				}
				break;

			case state_t::Negative:
				if (ch == '0')
				{
					state = state_t::Zero;
					negative = true;
				}
				else if (ch >= '1' and ch <= '9')
				{
					state = state_t::Number;
					m_token_int = ch - '0';
					negative = true;
				}
				else
					throw zeep::exception("invalid character '-' in json");
				break;

			case state_t::Zero:
#if DISALLOW_LEADING_ZERO
				if ((ch >= '0' and ch <= '9') or ch == '.')
					throw zeep::exception("invalid number in json, should not start with zero");
#else
				if (ch >= '0' and ch <= '9')
					throw zeep::exception("invalid number in json, should not start with zero");
				else if (ch == '.')
				{
					m_token_float = 0;
					m_token_int = 0;
					fraction = 0.1;
					state = state_t::NumberFraction;
				}
#endif
				else
				{
					retract();
					m_token_int = 0;
					token = token_t::Integer;
				}
				break;

			case state_t::Number:
				if (ch >= '0' and ch <= '9')
				{
					if (m_token_int > std::numeric_limits<decltype(m_token_int)>::max() / 10 or
						std::numeric_limits<decltype(m_token_int)>::max() - 10 * m_token_int < static_cast<int64_t>(ch - '0'))
						throw zeep::exception("overflow of integer value in json");
					m_token_int = 10 * m_token_int + (ch - '0');
				}
				else if (ch == '.')
				{
					m_token_float = static_cast<double>(m_token_int);
					fraction = 0.1;
					state = state_t::NumberFraction;
				}
				else if (ch == 'e' or ch == 'E')
				{
					m_token_float = static_cast<double>(m_token_int);
					state = state_t::NumberExpSign;
				}
				else
				{
					retract();
					token = token_t::Integer;
					if (negative)
						m_token_int = -m_token_int;
				}
				break;

			case state_t::NumberFraction:
				if (ch >= '0' and ch <= '9')
				{
					m_token_float += fraction * (ch - '0');
					fraction /= 10;
				}
				else if (ch == 'e' or ch == 'E')
					state = state_t::NumberExpSign;
				else
				{
					retract();
					token = token_t::Number;
					if (negative)
						m_token_float = -m_token_float;
				}
				break;

			case state_t::NumberExpSign:
				if (ch == '+')
					state = state_t::NumberExpDigit1;
				else if (ch == '-')
				{
					negativeExp = true;
					state = state_t::NumberExpDigit1;
				}
				else if (ch >= '0' and ch <= '9')
				{
					exponent = (ch - '0');
					state = state_t::NumberExpDigit2;
				}
				else
					throw zeep::exception("invalid floating point format in json");
				break;

			case state_t::NumberExpDigit1:
				if (ch >= '0' and ch <= '9')
				{
					exponent = (ch - '0');
					state = state_t::NumberExpDigit2;
				}
				else
					throw zeep::exception("invalid floating point format in json");
				break;

			case state_t::NumberExpDigit2:
				if (ch >= '0' and ch <= '9')
				{
					exponent = 10 * exponent + (ch - '0');
					if (exponent > std::numeric_limits<double>::max_exponent10)
						throw zeep::exception("Number out of range");
				}
				else
				{
					retract();
					// while (exponent-- > 0)
					// 	m_token_float *= negativeExp ? -10 : 10;
					m_token_float *= std::pow(10, (negativeExp ? -1 : 1) * exponent);
					if (negative)
						m_token_float = -m_token_float;
					token = token_t::Number;
				}
				break;

			case state_t::Literal:
				if (ch >= 128 or not std::isalpha(static_cast<int>(ch)))
				{
					retract();
					if (m_token == "true")
						token = token_t::True;
					else if (m_token == "false")
						token = token_t::False;
					else if (m_token == "null")
						token = token_t::Null;
					else
						throw zeep::exception("Invalid literal found in json: " + m_token);
				}
				break;

			case state_t::String:
				if (ch == '\"')
				{
					token = token_t::String;
					m_token.pop_back();
				}
				else if (ch == 0)
					throw zeep::exception("Invalid unterminated string in json");
				else if (ch == '\\')
				{
					state = state_t::Escape;
					m_token.pop_back();
				}
				break;

			case state_t::Escape:
				switch (ch)
				{
					case '"':
					case '\\':
					case '/':
						break;

					case 'n': m_token.back() = '\n'; break;
					case 't': m_token.back() = '\t'; break;
					case 'r': m_token.back() = '\r'; break;
					case 'f': m_token.back() = '\f'; break;
					case 'b': m_token.back() = '\b'; break;

					case 'u':
						state = state_t::EscapeHex1;
						m_token.pop_back();
						break;

					default:
						throw zeep::exception("Invalid escape sequence in json (\\" + std::string{ static_cast<char>(ch) } + ')');
				}
				if (state == state_t::Escape)
					state = state_t::String;
				break;

			case state_t::EscapeHex1:
				if (ch >= '0' and ch <= '9')
					hx = ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();
				state = state_t::EscapeHex2;
				break;

			case state_t::EscapeHex2:
				if (ch >= '0' and ch <= '9')
					hx = 16 * hx + ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 16 * hx + 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 16 * hx + 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();
				state = state_t::EscapeHex3;
				break;

			case state_t::EscapeHex3:
				if (ch >= '0' and ch <= '9')
					hx = 16 * hx + ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 16 * hx + 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 16 * hx + 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();
				state = state_t::EscapeHex4;
				break;

			case state_t::EscapeHex4:
				if (ch >= '0' and ch <= '9')
					hx = 16 * hx + ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 16 * hx + 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 16 * hx + 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();

				if (hx >= 0x0d800 and hx <= 0x0dbff)
				{
					surrogate = hx - 0x0d800;
					hx = 0;
					state = state_t::Surrogate2EscapeStart1;
				}
				else if (hx >= 0x0dc00 and hx <= 0x0dfff)
					throw zeep::exception("trailing surrogate character without a leading surrogate");
				else
				{
					append(m_token, hx);
					state = state_t::String;
				}
				break;

			case state_t::Surrogate2EscapeStart1:
				if (ch != '\\')
					throw zeep::exception("Expected second surrogate");
				else
				{
					state = state_t::Surrogate2EscapeStart2;
					m_token.pop_back();
				}
				break;

			case state_t::Surrogate2EscapeStart2:
				if (ch != 'u')
					throw zeep::exception("Expected second surrogate");
				else
				{
					state = state_t::Surrogate2EscapeHex1;
					m_token.pop_back();
				}
				break;

			case state_t::Surrogate2EscapeHex1:
				if (ch >= '0' and ch <= '9')
					hx = ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();
				state = state_t::Surrogate2EscapeHex2;
				break;

			case state_t::Surrogate2EscapeHex2:
				if (ch >= '0' and ch <= '9')
					hx = 16 * hx + ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 16 * hx + 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 16 * hx + 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();
				state = state_t::Surrogate2EscapeHex3;
				break;

			case state_t::Surrogate2EscapeHex3:
				if (ch >= '0' and ch <= '9')
					hx = 16 * hx + ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 16 * hx + 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 16 * hx + 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();
				state = state_t::Surrogate2EscapeHex4;
				break;

			case state_t::Surrogate2EscapeHex4:
				if (ch >= '0' and ch <= '9')
					hx = 16 * hx + ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 16 * hx + 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 16 * hx + 10 + ch - 'A';
				else
					throw zeep::exception("Invalid hex sequence in json");
				m_token.pop_back();

				if (hx >= 0x0dc00 and hx <= 0x0dfff)
				{
					hx = 0x10000 + surrogate * 0x0400 + (hx - 0x0dc00);
					append(m_token, hx);
					state = state_t::String;
				}
				else
					throw zeep::exception("Invalid second surrogate");
				break;
		}
	}

#if __cpp_lib_to_chars >= 201611L
	if (token == token_t::Number)
	{
		double vf;
		if (auto r = std::from_chars(m_token.data(), m_token.data() + m_token.length(), vf); r.ec == std::errc{})
			m_token_float = vf;
	}
#endif

	return token;
}

void json_parser::match(token_t expected)
{
	if (m_lookahead != expected)
		throw zeep::exception("Syntax error in json, expected " + describe_token(expected) + " but found " + describe_token(m_lookahead));

	m_lookahead = get_next_token();
}

void json_parser::parse_value(object &e)
{
	if (++m_depth > kMaxDepth)
	{
		--m_depth;
		throw object_error("Maximum nesting depth exceeded in json");
	}

	// guard scope: decrement m_depth on all exits
	struct depth_guard
	{
		size_t &d;
		~depth_guard() { --d; }
	} guard{ m_depth };

	switch (m_lookahead)
	{
		case token_t::Eof:
			break;

		case token_t::Null:
			match(m_lookahead);
			break;

		case token_t::False:
			match(m_lookahead);
			e = false;
			break;

		case token_t::True:
			match(m_lookahead);
			e = true;
			break;

		case token_t::Integer:
			match(m_lookahead);
			e = m_token_int;
			break;

		case token_t::Number:
			match(m_lookahead);
			e = m_token_float;
			break;

		case token_t::LeftBrace:
			match(m_lookahead);
			parse_object(e);
			match(token_t::RightBrace);
			break;

		case token_t::LeftBracket:
			match(m_lookahead);
			parse_array(e);
			match(token_t::RightBracket);
			break;

		case token_t::String:
			e = m_token;
			match(m_lookahead);
			break;

		default:
			throw object_error("Syntax error in json, unexpected token " + describe_token(m_lookahead));
	}
}

void json_parser::parse_object(object &e)
{
	for (;;)
	{
		if (m_lookahead == token_t::RightBrace or m_lookahead == token_t::Eof)
			break;

		auto name = m_token;
		match(token_t::String);
		match(token_t::Colon);

		object v;
		parse_value(v);
		e.emplace(name, v);

		if (m_lookahead != token_t::Comma)
			break;

		match(m_lookahead);
	}
}

void json_parser::parse_array(object &e)
{
	for (;;)
	{
		if (m_lookahead == token_t::RightBracket or m_lookahead == token_t::Eof)
			break;

		object v;
		parse_value(v);
		e.emplace_back(v);

		if (m_lookahead != token_t::Comma)
			break;

		match(m_lookahead);
	}
}

void json_parser::parse(object &obj)
{
	m_lookahead = get_next_token();
	parse_value(obj);
	if (m_lookahead != token_t::Eof)
		throw zeep::exception("Extraneaous data after parsing json");
}

// --------------------------------------------------------------------

void deserialize(std::istream &is, object &o)
{
	json_parser p(is);
	p.parse(o);
}

} // namespace zeep::el
