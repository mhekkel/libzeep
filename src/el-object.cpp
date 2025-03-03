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

#include "zeep/crypto.hpp"
#include "zeep/exception.hpp"
#include "zeep/http/el-object.hpp"
#include "zeep/unicode-support.hpp"

#include <charconv>

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

void object::push_back(object &&val)
{
	if (not(is_null() or is_array()))
		throw std::runtime_error("Invalid type for push_back");

	if (is_null())
	{
		m_type = value_type::array;
		m_data = value_type::array;
	}

	m_data.m_array->push_back(std::move(val));
	val.m_type = value_type::null;
}

void object::push_back(const object &val)
{
	if (not(is_null() or is_array()))
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

object::reference object::at(const typename object_type::key_type &key)
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");

	return m_data.m_object->at(key);
}

object::const_reference object::at(const typename object_type::key_type &key) const
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");

	return m_data.m_object->at(key);
}

object::reference object::operator[](const typename object_type::key_type &key)
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

object::const_reference object::operator[](const typename object_type::key_type &key) const
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

void serialize(std::ostream &os, const object &v)
{
	switch (v.m_type)
	{
		case object::value_type::array:
		{
			auto &a = *v.m_data.m_array;
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
			for (auto &kv : *v.m_data.m_object)
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

			for (uint8_t c : *v.m_data.m_string)
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
							os << c;
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

	std::string describe_token(token_t t) const
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

	uint8_t get_next_byte();
	char32_t get_next_unicode();
	char32_t get_next_char();
	void retract();

	token_t get_next_token();

	std::istream &m_is;

	// a minimal stack for ungetc like operations
	char32_t m_buffer[2];
	char32_t *m_buffer_ptr = m_buffer;

	std::string m_token;
	double m_token_float;
	int64_t m_token_int;
	token_t m_lookahead;
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
				throw std::runtime_error("Invalid utf-8");
			result = ((result & 0x01F) << 6) | (ch[0] & 0x03F);
		}
		else if ((result & 0x0F0) == 0x0E0)
		{
			ch[0] = get_next_byte();
			ch[1] = get_next_byte();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080)
				throw std::runtime_error("Invalid utf-8");
			result = ((result & 0x00F) << 12) | ((ch[0] & 0x03F) << 6) | (ch[1] & 0x03F);
		}
		else if ((result & 0x0F8) == 0x0F0)
		{
			ch[0] = get_next_byte();
			ch[1] = get_next_byte();
			ch[2] = get_next_byte();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
				throw std::runtime_error("Invalid utf-8");
			result = ((result & 0x007) << 18) | ((ch[0] & 0x03F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);

			if (result > 0x10ffff)
				throw std::runtime_error("invalid utf-8 character (out of range)");
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
					throw std::runtime_error("character 0x"s + s + " is not allowed");
				else
					throw std::runtime_error("character "s + std::to_string(result) + " is not allowed");
			}

			// surrogate support
			else if (result >= 0x0D800 and result <= 0x0DBFF)
			{
				char32_t uc2 = get_next_char();
				if (uc2 >= 0x0DC00 and uc2 <= 0x0DFFF)
					result = (result - 0x0D800) * 0x400 + (uc2 - 0x0DC00) + 0x010000;
				else
					throw std::runtime_error("leading surrogate character without trailing surrogate character");
			}
			else if (result >= 0x0DC00 and result <= 0x0DFFF)
				throw std::runtime_error("trailing surrogate character without a leading surrogate");
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
		EscapeHex4
	} state = state_t::Start;

	token_t token = token_t::Undef;
	double fraction = 1.0, exponent = 1;
	bool negative = false, negativeExp = false;

	char32_t hx = {};

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
						else if (ch < 128 and std::isalpha(ch))
							state = state_t::Literal;
						else
							throw zeep::exception("invalid character (" + (std::isprint(ch) ? std::string(1, static_cast<char>(ch)) : to_hex(ch)) + ") in json");
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
					m_token_int = 10 * m_token_int + (ch - '0');
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
					exponent = 10 * exponent + (ch - '0');
				else
				{
					retract();
					m_token_float *= std::pow(10, (negativeExp ? -1 : 1) * exponent);
					if (negative)
						m_token_float = -m_token_float;
					token = token_t::Number;
				}
				break;

			case state_t::Literal:
				if (ch > 128 or not std::isalpha(ch))
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
				append(m_token, hx);
				state = state_t::String;
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
			throw std::runtime_error("Syntax error in json, unexpected token " + describe_token(m_lookahead));
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

} // namespace zeep::http
