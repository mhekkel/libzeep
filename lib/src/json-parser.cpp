// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <map>
#include <cassert>
#include <memory>
#include <algorithm>
#include <cmath>

#include <boost/algorithm/string.hpp>

#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
#include <zeep/xml/doctype.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/el/element.hpp>

#include "parser-internals.hpp"

#include <zeep/el/parser.hpp>

namespace ba = boost::algorithm;

namespace zeep
{

namespace el
{

using namespace xml;

class json_parser
{
  public:
	json_parser(std::istream& is)
		: m_is(is)
	{
	}

	void parse(el::element& object);

  private:

	enum class token_t : uint8_t
	{
		Eof,
		LeftBrace, RightBrace,
		LeftBracket, RightBracket,
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
			case token_t::Eof:			return "end of data"; break; 
			case token_t::LeftBrace:	return "left brace ('{')"; break; 
			case token_t::RightBrace:	return "richt brace ('}')"; break; 
			case token_t::LeftBracket:	return "left bracket ('[')"; break; 
			case token_t::RightBracket:	return "right bracket (']')"; break; 
			case token_t::Comma:		return "comma"; break; 
			case token_t::Colon:		return "colon"; break; 
			case token_t::String:		return "string"; break; 
			case token_t::Integer:		return "integer"; break; 
			case token_t::Number:		return "number"; break; 
			case token_t::True:			return "true"; break; 
			case token_t::False:		return "false"; break; 
			case token_t::Null:			return "null"; break; 
			case token_t::Undef:		return "undefined token"; break; 
		}
	}

	void match(token_t expected);

	void parse_value(el::element& e);
	void parse_object(el::element& e);
	void parse_array(el::element& e);

	uint8_t get_next_byte();
	unicode get_next_unicode();
	unicode get_next_char();
	void retract();

	token_t get_next_token();

	std::istream& m_is;
	::zeep::detail::mini_stack m_buffer;
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

unicode json_parser::get_next_unicode()
{
	unicode result = get_next_byte();

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

unicode json_parser::get_next_char()
{
	unicode result = 0;

	if (not m_buffer.empty()) // if buffer is not empty we already did all the validity checks
	{
		result = m_buffer.top();
		m_buffer.pop();
	}
	else
	{
		result = get_next_unicode();

		if (result >= 0x080)
		{
			if (result == 0x0ffff or result == 0x0fffe)
				throw std::runtime_error("character " + to_hex(result) + " is not allowed");

			// surrogate support
			else if (result >= 0x0D800 and result <= 0x0DBFF)
			{
				unicode uc2 = get_next_char();
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
			static_cast<char>(0x080 | (result & 0x3f))};
		m_token.append(ch, 2);
	}
	else if (result < 0x00010000)
	{
		char ch[3] = {
			static_cast<char>(0x0e0 | (result >> 12)),
			static_cast<char>(0x080 | ((result >> 6) & 0x3f)),
			static_cast<char>(0x080 | (result & 0x3f))};
		m_token.append(ch, 3);
	}
	else
	{
		char ch[4] = {
			static_cast<char>(0x0f0 | (result >> 18)),
			static_cast<char>(0x080 | ((result >> 12) & 0x3f)),
			static_cast<char>(0x080 | ((result >> 6) & 0x3f)),
			static_cast<char>(0x080 | (result & 0x3f))};
		m_token.append(ch, 4);
	}

	return result;
}

void json_parser::retract()
{
	assert(not m_token.empty());
	m_buffer.push(pop_last_char(m_token));
}

auto json_parser::get_next_token() -> token_t
{
	enum class state_t
	{
 		Start,
		Negative,
		Zero,
		NegativeZero,
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

	unicode hx;

	m_token.clear();

	while (token == token_t::Undef)
	{
		unicode ch = get_next_char();

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
				token  = token_t::Colon;
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
				else if (zeep::xml::is_name_start_char(ch))
					state = state_t::Literal;
				else
					throw zeep::exception("invalid character (" + xml::to_hex(ch) + ") in json");
			}
			break;

		case state_t::Negative:
			if (ch == '0')
				state = state_t::NegativeZero;
			else if (ch >= '1' and ch <= '9')
			{
				state = state_t::Number;
				m_token_int = ch - '0';
				negative = true;
			}
			else
				throw zeep::exception("invalid character '-' in json");
			break;

		case state_t::NegativeZero:
			if (ch >= '0' or ch <= '9')
				throw zeep::exception("invalid number in json, should not start with zero");
			token = token_t::Number;
			break;

		case state_t::Zero:
			if (ch >= '0' or ch <= '9')
				throw zeep::exception("invalid number in json, should not start with zero");
			token = token_t::Number;
			break;

		case state_t::Number:
			if (ch >= '0' and ch <= '9')
				m_token_int = 10 * m_token_int + (ch - '0');
			else if (ch == '.')
			{
				m_token_float = m_token_int;
				fraction = 0.1;
				state = state_t::NumberFraction;
			}
			else
			{
				retract();
				token = token_t::Integer;
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
			if (not std::isalpha(ch))
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
				
				case 'n':	m_token.back() = '\n'; break;
				case 't':	m_token.back() = '\t'; break;
				case 'r':	m_token.back() = '\r'; break;
				case 'f':	m_token.back() = '\f'; break;
				case 'b':	m_token.back() = '\b'; break;

				case 'u':
					state = state_t::EscapeHex1;
					m_token.pop_back();
					break;

				default:
					throw zeep::exception("Invalid escape sequence in json (\\" + std::string{static_cast<char>(ch)} + ')');
			}
			if (state == state_t::Escape)
				state = state_t::String;
			break;

		case state_t::EscapeHex1:
			if (ch >= 0 and ch <= '9')
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
			if (ch >= 0 and ch <= '9')
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
			if (ch >= 0 and ch <= '9')
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
			if (ch >= 0 and ch <= '9')
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

	return token;
}

void json_parser::match(token_t expected)
{
	if (m_lookahead != expected)
		throw zeep::exception("Syntax error in json, expected " + describe_token(expected) + " but found " + describe_token(m_lookahead));
	
	m_lookahead = get_next_token();
}

void json_parser::parse_value(el::element& e)
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
			throw std::runtime_error("Syntax eror in json, unexpected token " + describe_token(m_lookahead));
	}
}

void json_parser::parse_object(el::element& e)
{
	for (;;)
	{
		if (m_lookahead == token_t::RightBrace or m_lookahead == token_t::Eof)
			break;
		
		auto name = m_token;
		match(token_t::String);
		match(token_t::Colon);

		el::element v;
		parse_value(v);
		e.emplace(name, v);

		if (m_lookahead != token_t::Comma)
			break;
		
		match(m_lookahead);
	}
}

void json_parser::parse_array(el::element& e)
{
	for (;;)
	{
		if (m_lookahead == token_t::RightBracket or m_lookahead == token_t::Eof)
			break;
		
		el::element v;
		parse_value(v);
		e.emplace_back(v);

		if (m_lookahead != token_t::Comma)
			break;
		
		match(m_lookahead);
	}
}

void json_parser::parse(el::element& obj)
{
	m_lookahead = get_next_token();
	parse_value(obj);
	if (m_lookahead != token_t::Eof)
		throw zeep::exception("Extraneaous data after parsing json");
}

void parse_json(const std::string& json, element& object)
{
	std::istringstream s(json);
	json_parser p(s);

	p.parse(object);
}

void parse_json(std::istream& is, element& object)
{
	json_parser p(is);
	p.parse(object);
}

}
}

zeep::el::element operator""_json(const char* s, size_t n)
{
	zeep::el::element result;
	zeep::el::parse_json(std::string{s, n}, result);
	return result;
}