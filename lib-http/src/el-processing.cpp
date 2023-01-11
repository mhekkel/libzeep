// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp::json is a set of classes used to implement an 'expression language'
// A script language used in the XHTML templates used by the zeep webapp.
//

#include <zeep/config.hpp>

#include <locale>
#include <codecvt>

#include <zeep/crypto.hpp>
#include <zeep/unicode-support.hpp>
#include <zeep/http/el-processing.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/uri.hpp>

#include "format.hpp"

namespace zeep::http
{

bool is_name_start_char(unicode uc)
{
	return
		uc == L':' or
		(uc >= L'A' and uc <= L'Z') or
		uc == L'_' or
		(uc >= L'a' and uc <= L'z') or
		(uc >= 0x0C0 and uc <= 0x0D6) or
		(uc >= 0x0D8 and uc <= 0x0F6) or
		(uc >= 0x0F8 and uc <= 0x02FF) or
		(uc >= 0x0370 and uc <= 0x037D) or
		(uc >= 0x037F and uc <= 0x01FFF) or
		(uc >= 0x0200C and uc <= 0x0200D) or
		(uc >= 0x02070 and uc <= 0x0218F) or
		(uc >= 0x02C00 and uc <= 0x02FEF) or
		(uc >= 0x03001 and uc <= 0x0D7FF) or
		(uc >= 0x0F900 and uc <= 0x0FDCF) or
		(uc >= 0x0FDF0 and uc <= 0x0FFFD) or
		(uc >= 0x010000 and uc <= 0x0EFFFF);	
}

bool is_name_char(unicode uc)
{
	return
		uc == '-' or
		uc == '.' or
		(uc >= '0' and uc <= '9') or
		uc == 0x0B7 or
		is_name_start_char(uc) or
		(uc >= 0x00300 and uc <= 0x0036F) or
		(uc >= 0x0203F and uc <= 0x02040);
}


using request = http::request;
using object = json::element;

// --------------------------------------------------------------------

enum class token_type
{
	undef,
	eof,
	number_int,
	number_float,
	string,
	object,

	assign,

	and_,
	or_,
	not_,
	eq,
	ne,
	lt,
	le,
	ge,
	gt,
	plus,
	minus,
	div,
	mod,
	mult,
	lparen,
	rparen,
	lbracket,
	rbracket,
	lbrace,
	rbrace,
	if_,
	elvis,
	else_,
	dot,
	hash,
	bar,

	true_,
	false_,
	in,
	comma,

	whitespace,

	fragment_separator,

	variable_template,
	selection_template,
	message_template,
	link_template,
	fragment_template,

	error
};

// --------------------------------------------------------------------
// interpreter for expression language

struct interpreter
{
	using unicode = uint32_t;

	interpreter(const scope &scope)
		: m_scope(scope) {}

	// template <class OutputIterator, class Match>
	// OutputIterator operator()(Match &m, OutputIterator out, std::regex::match_flag_type);

	object evaluate(const std::string &s);

	std::vector<std::pair<std::string,std::string>> evaluate_attr_expr(const std::string& s);

	bool evaluate_assert(const std::string& s);
	void evaluate_with(scope& scope, const std::string& s);
	object evaluate_link(const std::string& s);

	bool process(std::string &s);

	void match(token_type t);

	unsigned char next_byte();
	unicode get_next_char();
	void retract();
	void get_next_token();

	object parse_expr();				// or_expr ( '?' expr ':' expr )?
	object parse_or_expr();				// and_expr ( 'or' and_expr)*
	object parse_and_expr();			// equality_expr ( 'and' equality_expr)*
	object parse_equality_expr();		// relational_expr ( ('=='|'!=') relational_expr )?
	object parse_relational_expr();		// additive_expr ( ('<'|'<='|'>='|'>') additive_expr )*
	object parse_additive_expr();		// multiplicative_expr ( ('+'|'-') multiplicative_expr)*
	object parse_multiplicative_expr(); // unary_expr (('%'|'/') unary_expr)*
	object parse_unary_expr();			// ('-')? primary_expr
	object parse_primary_expr();		// '(' expr ')' | number | string
	object parse_literal_substitution();// '|xxx ${var}|'
	object parse_template_expr();

	object parse_link_template_expr();
	object parse_fragment_expr();
	object parse_selector();

	object parse_utility_expr();

	object call_method(const std::string& className, const std::string& method, std::vector<object>& params);

	const scope &m_scope;
	token_type m_lookahead;
	std::string m_token_string;
	int64_t m_token_number_int;
	double m_token_number_float;
	std::string::const_iterator m_ptr, m_end;
	bool m_return_whitespace = false;
	bool m_expect_fragment_spec = false;
};

// template <class OutputIterator, class Match>
// inline OutputIterator interpreter::operator()(Match &m, OutputIterator out, std::regex_constants::match_flag_type)
// {
// 	string s(m[1]);

// 	process(s);

// 	copy(s.begin(), s.end(), out);

// 	return out;
// }

object interpreter::evaluate(const std::string &s)
{
	object result;

	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();

		if (m_lookahead != token_type::eof)
			result = parse_expr();
		match(token_type::eof);
	}
	catch (const std::exception& e)
	{
		using namespace std::literals;
		result = "Error parsing expression: "s + e.what();
	}

	return result;
}

std::vector<std::pair<std::string,std::string>> interpreter::evaluate_attr_expr(const std::string& s)
{
	std::vector<std::pair<std::string,std::string>> result;

	m_ptr = s.begin();
	m_end = s.end();
	get_next_token();

	for (;;)
	{
		std::string var = m_token_string;
		match(token_type::object);

		match (token_type::assign);

		auto value = parse_expr();

		result.emplace_back(var, value.as<std::string>());

		if (m_lookahead != token_type::comma)
			break;
		
		match(token_type::comma);
	}

	match(token_type::eof);

	return result;
}

bool interpreter::evaluate_assert(const std::string& s)
{
	bool result = true;

	m_ptr = s.begin();
	m_end = s.end();
	get_next_token();

	for (;;)
	{
		auto value = parse_expr();

		if (not value)
		{
			result = false;
			break;
		}

		if (m_lookahead != token_type::comma)
			break;
		
		match(token_type::comma);
	}

	// match(token_type::eof);

	return result;
}

void interpreter::evaluate_with(scope& scope, const std::string& s)
{
	m_ptr = s.begin();
	m_end = s.end();
	get_next_token();

	while (m_lookahead == token_type::object)
	{
		auto name = m_token_string;
		match(token_type::object);
		match(token_type::assign);

		auto value = parse_expr();

		scope.put(name, value);
		
		if (m_lookahead == token_type::comma)
		{
			match(token_type::comma);
			continue;
		}
	}

	match(token_type::eof);
}

object interpreter::evaluate_link(const std::string& s)
{
	object result;

	try
	{
		m_ptr = s.begin();
		m_end = s.end();

		get_next_token();

		switch (m_lookahead)
		{
			case token_type::object:
			case token_type::fragment_separator:
				result = parse_fragment_expr();
				break;
			
			case token_type::link_template:
				match(token_type::link_template);
				result = parse_fragment_expr();
				match(token_type::rbrace);
				break;
			
			default:
				m_expect_fragment_spec = true;
				result = parse_primary_expr();
				break;
		}

		match(token_type::eof);
	}
	catch (const std::exception& e)
	{
		using namespace std::literals;
		result = "Error parsing expression: "s + e.what();
	}

	return result;
}

bool interpreter::process(std::string &s)
{
	bool result = false;

	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();

		object obj;

		if (m_lookahead != token_type::eof)
			obj = parse_expr();
		match(token_type::eof);

		if (obj.is_null())
			s.clear();
		else
			s = obj.as<std::string>();

		result = true;
	}
	catch (const std::exception &e)
	{
		result = false;
	}

	return result;
}

void interpreter::match(token_type t)
{
	if (t != m_lookahead)
		throw zeep::exception("syntax error");
	get_next_token();
}

unsigned char interpreter::next_byte()
{
	char result = 0;

	if (m_ptr < m_end)
	{
		result = *m_ptr;
		++m_ptr;
	}

	m_token_string += result;

	return static_cast<unsigned char>(result);
}

// We assume all paths are in valid UTF-8 encoding
interpreter::unicode interpreter::get_next_char()
{
	unicode result = 0;
	unsigned char ch[5];

	ch[0] = next_byte();

	if ((ch[0] & 0x080) == 0)
		result = ch[0];
	else if ((ch[0] & 0x0E0) == 0x0C0)
	{
		ch[1] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F);
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F);
	}

	if (result > 0x10ffff)
		throw zeep::exception("invalid utf-8 character (out of range)");

	return result;
}

void interpreter::retract()
{
	std::string::iterator c = m_token_string.end();

	// skip one valid character back in the input buffer
	// since we've arrived here, we can safely assume input
	// is valid UTF-8
	do
		--c;
	while ((*c & 0x0c0) == 0x080);

	if (m_ptr != m_end or *c != 0)
		m_ptr -= m_token_string.end() - c;
	m_token_string.erase(c, m_token_string.end());
}

void interpreter::get_next_token()
{
	enum class State
	{
		Start,
		Equals,
		ExclamationMark,
		LessThan,
		GreaterThan,
		Question,
		Number,
		NumberFraction,
		Name,
		Literal,

		Colon,

		Hash,

		TemplateStart
	} state = State::Start;

	token_type token = token_type::undef;
	double fraction = 1.0;

	m_token_string.clear();

	while (token == token_type::undef)
	{
		unicode ch = get_next_char();

		switch (state)
		{
		case State::Start:
			switch (ch)
			{
			case 0:
				token = token_type::eof;
				break;
			case '(':
				token = token_type::lparen;
				break;
			case ')':
				token = token_type::rparen;
				break;
			case '[':
				token = token_type::lbracket;
				break;
			case ']':
				token = token_type::rbracket;
				break;
			case '{':
				token = token_type::lbrace;
				break;
			case '}':
				token = token_type::rbrace;
				break;
			case '?':
				state = State::Question;
				break;
			case '/':
				token = token_type::div;
				break;
			case '+':
				token = token_type::plus;
				break;
			case '-':
				token = token_type::minus;
				break;
			case '.':
				token = token_type::dot;
				break;
			case ',':
				token = token_type::comma;
				break;
			case '|':
				token = token_type::bar;
				break;
			case '=':
				state = State::Equals;
				break;
			case '!':
				state = State::ExclamationMark;
				break;
			case '<':
				state = State::LessThan;
				break;
			case '>':
				state = State::GreaterThan;
				break;
			case ':':
				state = State::Colon;
				break;

			case '*':
			case '$':
			case '#':
			case '@':
			case '~':
				state = State::TemplateStart;
				break;

			case ' ':
			case '\n':
			case '\r':
			case '\t':
				if (m_return_whitespace)
					token = token_type::whitespace;
				else
					m_token_string.clear();
				break;
			case '\'':
				state = State::Literal;
				break;

			default:
				if (ch >= '0' and ch <= '9')
				{
					m_token_number_int = ch - '0';
					state = State::Number;
				}
				else if (is_name_start_char(ch))
					state = State::Name;
				else
					token = token_type::error;
					// throw zeep::exception("invalid character (" + to_hex(ch) + ") in expression");
			}
			break;

		case State::TemplateStart:
			if (ch == '{')
			{
				switch (m_token_string[0])
				{
					case '$':	token = token_type::variable_template; break;
					case '*':	token = token_type::selection_template; break;
					case '#':	token = token_type::message_template; break;
					case '@':	token = token_type::link_template; break;
					case '~':	token = token_type::fragment_template; break;
					default: assert(false);
				}
			}
			else
			{
				retract();
				if (m_token_string[0] == '*')
					token = token_type::mult;
				else if (m_token_string[0] == '#')
					state = State::Hash;
				else
					token = token_type::error;
				// else
				// 	throw zeep::exception("invalid character (" + std::string{static_cast<char>(isprint(ch) ? ch : ' ')} + '/' + to_hex(ch) + ") in expression");
			}
			break;

		case State::Equals:
			if (ch != '=')
			{
				retract();
				token = token_type::assign;
			}
			else
				token = token_type::eq;
			break;

		case State::Question:
			if (ch == ':')
				token = token_type::elvis;
			else
			{
				retract();
				token = token_type::if_;
			}
			break;

		case State::ExclamationMark:
			if (ch != '=')
			{
				retract();
				token = token_type::error;
				// throw zeep::exception("unexpected character ('!') in expression");
			}
			token = token_type::ne;
			break;

		case State::LessThan:
			if (ch == '=')
				token = token_type::le;
			else
			{
				retract();
				token = token_type::lt;
			}
			break;

		case State::GreaterThan:
			if (ch == '=')
				token = token_type::ge;
			else
			{
				retract();
				token = token_type::gt;
			}
			break;

		case State::Number:
			if (ch >= '0' and ch <= '9')
				m_token_number_int = 10 * m_token_number_int + (ch - '0');
			else if (ch == '.')
			{
				m_token_number_float = static_cast<double>(m_token_number_int);
				fraction = 0.1;
				state = State::NumberFraction;
			}
			else
			{
				retract();
				token = token_type::number_int;
			}
			break;

		case State::NumberFraction:
			if (ch >= '0' and ch <= '9')
			{
				m_token_number_float += fraction * (ch - '0');
				fraction /= 10;
			}
			else
			{
				retract();
				token = token_type::number_float;
			}
			break;

		case State::Name:
			if (ch == '.' or ch == ':' or not is_name_char(ch))
			{
				retract();
				if (m_token_string == "div")
					token = token_type::div;
				else if (m_token_string == "mod")
					token = token_type::mod;
				else if (m_token_string == "and")
					token = token_type::and_;
				else if (m_token_string == "or")
					token = token_type::or_;
				else if (m_token_string == "not")
					token = token_type::not_;
				else if (m_token_string == "lt")
					token = token_type::lt;
				else if (m_token_string == "le")
					token = token_type::le;
				else if (m_token_string == "ge")
					token = token_type::ge;
				else if (m_token_string == "gt")
					token = token_type::gt;
				else if (m_token_string == "ne")
					token = token_type::ne;
				else if (m_token_string == "eq")
					token = token_type::eq;
				else if (m_token_string == "true")
					token = token_type::true_;
				else if (m_token_string == "false")
					token = token_type::false_;
				else if (m_token_string == "in")
					token = token_type::in;
				else
					token = token_type::object;
			}
			break;

		case State::Literal:
			if (ch == 0)
				token = token_type::error;
				// throw zeep::exception("run-away string, missing quote character?");
			else if (ch == '\'')
			{
				token = token_type::string;
				m_token_string = m_token_string.substr(1, m_token_string.length() - 2);
			}
			break;
		
		case State::Hash:
			if (ch == '.' or not is_name_char(ch))
			{
				retract();
				token = token_type::hash;
			}
			break;
		
		case State::Colon:
			if (ch == ':')
				token = token_type::fragment_separator;
			else
			{
				retract();
				token = token_type::else_;
			}
			break;
		
		}
	}

	m_lookahead = token;
}

object interpreter::parse_expr()
{
	object result;
	
	result = parse_or_expr();

	if (m_lookahead == token_type::if_)
	{
		match(m_lookahead);
		object a = parse_expr();

		if (m_lookahead == token_type::else_)
		{
			match(token_type::else_);
			object b = parse_expr();
			if (result)
				result = a;
			else
				result = b;
		}
		else if (result)
			result = a;
		else
			result = {};
	}
	else if (m_lookahead == token_type::elvis)
	{
		match(m_lookahead);
		object a = parse_expr();

		if (not result)
			result = a;
	}

	return result;
}

object interpreter::parse_or_expr()
{
	object result = parse_and_expr();
	while (m_lookahead == token_type::or_)
	{
		match(m_lookahead);
		bool b1 = result.as<bool>();
		bool b2 = parse_and_expr().as<bool>();
		result = b1 or b2;
	}
	return result;
}

object interpreter::parse_and_expr()
{
	object result = parse_equality_expr();
	while (m_lookahead == token_type::and_)
	{
		match(m_lookahead);
		bool b1 = result.as<bool>();
		bool b2 = parse_equality_expr().as<bool>();
		result = b1 and b2;
	}
	return result;
}

object interpreter::parse_equality_expr()
{
	object result = parse_relational_expr();
	if (m_lookahead == token_type::eq)
	{
		match(m_lookahead);
		result = (result == parse_relational_expr());
	}
	else if (m_lookahead == token_type::ne)
	{
		match(m_lookahead);
		result = not(result == parse_relational_expr());
	}
	return result;
}

object interpreter::parse_relational_expr()
{
	object result = parse_additive_expr();
	switch (m_lookahead)
	{
	case token_type::lt:
		match(m_lookahead);
		result = (result < parse_additive_expr());
		break;
	case token_type::le:
		match(m_lookahead);
		result = (result <= parse_additive_expr());
		break;
	case token_type::ge:
		match(m_lookahead);
		result = (parse_additive_expr() <= result);
		break;
	case token_type::gt:
		match(m_lookahead);
		result = (parse_additive_expr() < result);
		break;
	case token_type::not_:
	{
		match(token_type::not_);
		match(token_type::in);

		object list = parse_additive_expr();

		result = not list.contains(result);
		break;
	}
	case token_type::in:
	{
		match(m_lookahead);
		object list = parse_additive_expr();

		result = list.contains(result);
		break;
	}
	default:
		break;
	}
	return result;
}

object interpreter::parse_additive_expr()
{
	object result = parse_multiplicative_expr();
	while (m_lookahead == token_type::plus or m_lookahead == token_type::minus)
	{
		if (m_lookahead == token_type::plus)
		{
			match(m_lookahead);
			result = (result + parse_multiplicative_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result - parse_multiplicative_expr());
		}
	}
	return result;
}

object interpreter::parse_multiplicative_expr()
{
	object result = parse_unary_expr();
	while (m_lookahead == token_type::div or m_lookahead == token_type::mod or m_lookahead == token_type::mult)
	{
		if (m_lookahead == token_type::mult)
		{
			match(m_lookahead);
			result = (result * parse_unary_expr());
		}
		else if (m_lookahead == token_type::div)
		{
			match(m_lookahead);
			result = (result / parse_unary_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result % parse_unary_expr());
		}
	}
	return result;
}

object interpreter::parse_unary_expr()
{
	object result;
	if (m_lookahead == token_type::minus)
	{
		match(m_lookahead);
		result = -(parse_primary_expr());
	}
	else if (m_lookahead == token_type::not_)
	{
		match(m_lookahead);
		result = not parse_primary_expr().as<bool>();
	}
	else
		result = parse_primary_expr();
	return result;
}

object interpreter::parse_template_expr()
{
	object result;

	bool save_return_white_space = m_return_whitespace;
	m_return_whitespace = false;

	switch (m_lookahead)
	{
		case token_type::variable_template:
			match(token_type::variable_template);
			result = parse_expr();
			m_return_whitespace = save_return_white_space;
			match(token_type::rbrace);
			break;
		
		case token_type::link_template:
			match(token_type::link_template);
			result = parse_link_template_expr();
			m_return_whitespace = save_return_white_space;
			match(token_type::rbrace);
			break;

		case token_type::selection_template:
			match(token_type::selection_template);
			if (m_lookahead == token_type::object)
			{
				result = m_scope.lookup(m_token_string, true);
				match(token_type::object);
				for (;;)
				{
					if (m_lookahead == token_type::dot)
					{
						match(m_lookahead);
						if (result.type() == object::value_type::array and (m_token_string == "count" or m_token_string == "length"))
							result = result.size();
						else if (m_token_string == "empty")
							result = result.empty();
						else if (result.type() == object::value_type::object)
							result = const_cast<const object &>(result)[m_token_string];
						else
							result = object::value_type::null;
						match(token_type::object);
						continue;
					}

					if (m_lookahead == token_type::lbracket)
					{
						match(m_lookahead);

						object index = parse_expr();
						match(token_type::rbracket);

						if (index.empty() or (result.type() != object::value_type::array and result.type() != object::value_type::object))
							result = object();
						else if (result.type() == object::value_type::array)
							result = result[index.as<int>()];
						else if (result.type() == object::value_type::object)
							result = result[index.as<std::string>()];
						else
							result = object::value_type::null;
						continue;
					}

					break;
				}
			}
			else
				result = parse_expr();
			m_return_whitespace = save_return_white_space;
			match(token_type::rbrace);
			break;

		default:
			throw zeep::exception("syntax error, unexpected token: " + m_token_string);
	}

	return result;
}

object interpreter::parse_primary_expr()
{
	object result;
	switch (m_lookahead)
	{
		case token_type::true_:
			result = true;
			match(m_lookahead);
			break;

		case token_type::false_:
			result = false;
			match(m_lookahead);
			break;

		case token_type::number_int:
			result = m_token_number_int;
			match(m_lookahead);
			break;

		case token_type::number_float:
			result = m_token_number_float;
			match(m_lookahead);
			break;

		case token_type::string:
			result = m_token_string;
			match(m_lookahead);
			break;

		case token_type::lparen:
			match(m_lookahead);
			result = parse_expr();
			match(token_type::rparen);
			break;

		case token_type::hash:
			result = parse_utility_expr();
			break;

		// a list
		case token_type::lbrace:
			match(m_lookahead);
			for (;;)
			{
				result.push_back(parse_expr());
				if (m_lookahead == token_type::comma)
				{
					match(m_lookahead);
					continue;
				}
				break;
			}
			match(token_type::rbrace);
			break;

		case token_type::bar:
			match(token_type::bar);
			result = parse_literal_substitution();
			match(token_type::bar);
			break;

		case token_type::object:
			result = m_scope.lookup(m_token_string);
			match(token_type::object);

			for (;;)
			{
				if (m_lookahead == token_type::dot)
				{
					match(m_lookahead);

					if (not result.is_null())
					{
						if (result.type() == object::value_type::array and (m_token_string == "count" or m_token_string == "length"))
							result = object((uint32_t)result.size());
						else if (m_token_string == "empty")
							result = result.empty();
						else if (result.type() == object::value_type::object)
							result = const_cast<const object &>(result)[m_token_string];
						else
							result = object::value_type::null;
					}
					match(token_type::object);
					continue;
				}

				if (m_lookahead == token_type::lbracket)
				{
					match(m_lookahead);

					object index = parse_expr();
					match(token_type::rbracket);

					if (not result.is_null())
					{
						if (result.type() == object::value_type::array and not (result.empty() or index.empty()))
							result = result[index.as<int>()];
						else if (result.type() == object::value_type::object and not (result.empty() or index.empty()))
							result = result[index.as<std::string>()];
						else
							result = object::value_type::null;
					}
					continue;
				}
				break;
			}
			break;

		case token_type::variable_template:
		case token_type::link_template:
		case token_type::selection_template:
			result = parse_template_expr();
			break;

		case token_type::fragment_template:
			match(token_type::fragment_template);
			result = parse_fragment_expr();
			match(token_type::rbrace);
			break;

		default:
			throw zeep::exception("syntax error, expected number, string or object");
	}

	return result;
}

object interpreter::parse_literal_substitution()
{
	std::string result;

	m_return_whitespace = true;

	while (m_lookahead != token_type::bar and m_lookahead != token_type::eof)
	{
		switch (m_lookahead)
		{
			case token_type::variable_template:
			case token_type::selection_template:
			case token_type::message_template:
				result += parse_template_expr().as<std::string>();
				break;
			
			default:
				result += m_token_string;
				match(m_lookahead);
				break;
		}
	}

	m_return_whitespace = false;

	return result;
}

// --------------------------------------------------------------------

object interpreter::parse_link_template_expr()
{
	std::string path;

	int braces = 0;

	// in case of a relative URL starting with a forward slash, we prefix the URL with the context_name of the server
	if (m_lookahead == token_type::div)
	{
		match(token_type::div);

		auto context = m_scope.get_context_name();
		if (not context.empty())
		{
			if (is_valid_uri(context) and not uri(context).get_scheme().empty())
				path = context + '/';
			else
				path = '/' + context + '/';
		}
		else
			path = "/";
	}

	while (m_lookahead != token_type::lparen and m_lookahead != token_type::eof)
	{
		if (m_lookahead == token_type::rbrace)
		{
			if (braces-- == 0)
				break;

			path += m_token_string;
			match(token_type::rbrace);
			continue;
		}

		switch (m_lookahead)
		{
			case token_type::variable_template:
			case token_type::selection_template:
				path += parse_template_expr().as<std::string>();
				break;
			
			case token_type::lbrace:
				path += m_token_string;
				match(token_type::lbrace);
				++braces;
				break;
			
			default:
				path += m_token_string;
				match(m_lookahead);
				break;
		}
	}

	if (m_lookahead == token_type::lparen)
	{
		match(token_type::lparen);

		std::map<std::string,std::string> parameters;

		for (;;)
		{
			std::string name = m_token_string;
			match(token_type::object);

			if (m_lookahead == token_type::assign)
			{
				match(token_type::assign);
				std::string value = parse_primary_expr().as<std::string>();

				// put into path directly, if found
				std::string::size_type p = path.find('{' + name + '}');
				if (p == std::string::npos)
					parameters[name] = value;
				else
				{
					do
					{
						path = path.substr(0, p) + value + path.substr(p + name.length() + 2);
						p += value.length();
					}
					while ((p = path.find('{' + name + '}', p)) != std::string::npos);
				}
			}
			else
				parameters[name] = "";

			if (m_lookahead == token_type::comma)
			{
				match(token_type::comma);
				continue;
			}

			break;
		}

		match(token_type::rparen);

		if (not parameters.empty())
		{
			path += '?';
			auto n = parameters.size();
			for (auto p: parameters)
			{
				path += encode_url(p.first);

				if (not p.second.empty())
					path += '=' + encode_url(p.second);

				if (--n > 0)
					path += '&';
			}
		}
	}

	return path;
}

// --------------------------------------------------------------------

object interpreter::parse_fragment_expr()
{
	object result{ {"fragment-spec", true } };

	if (m_lookahead == token_type::fragment_separator)
		result["template"] = "this";
	else if (m_lookahead == token_type::object)
	{
		result["template"] = m_token_string;
		match(m_lookahead);
	}
	else if (m_lookahead == token_type::rbrace)
	{
		result["template"] = "this";
		result["selector"] = {
			{ "xpath", "" }
		};
	}
	else
		result["template"] = parse_expr();

	if (m_lookahead == token_type::fragment_separator)
	{
		match(token_type::fragment_separator);

		result["selector"] = parse_selector();
	}

	return result;
}

// --------------------------------------------------------------------

object interpreter::parse_selector()
{
	object result;
	std::string xpath;

	while (m_lookahead == token_type::div or
		   m_lookahead == token_type::object or
		   m_lookahead == token_type::lbracket or
		   m_lookahead == token_type::dot or
		   m_lookahead == token_type::hash)
	{
		bool divided = false;

		if (m_lookahead == token_type::div)
		{
			divided = true;
			match(m_lookahead);
			if (m_lookahead == token_type::div)
			{
				match(m_lookahead);
				xpath += "//";
			}
			else
				xpath += "/";
		}
		else
			xpath += "//";
		
		if (m_lookahead == token_type::object)
		{
			std::string name = m_token_string;
			match(m_lookahead);

			if (m_lookahead == token_type::lparen and
				(name == "text" or
				 name == "comment" or
				 name == "processing-instruction" or
				 name == "node"))
			{
				match(m_lookahead);
				match(token_type::rparen);
				xpath += name + "()";
			}
			else
			{
				if (divided)
					xpath += name;
				else
					xpath +=
						"*[name()='"+name+"' or "
						"attribute::*[namespace-uri() = $ns and "
							"(local-name() = 'ref' or local-name() = 'fragment') and "
							"starts-with(string(), '" + name + "')]"
						"]";

				if (m_lookahead == token_type::lparen)
				{
					match(token_type::lparen);

					while (m_lookahead != token_type::rparen and m_lookahead != token_type::eof)
					{
						result["params"].push_back(parse_expr());
						if (m_lookahead == token_type::comma)
						{
							match(m_lookahead);
							continue;
						}
						break;
					}

					match(token_type::rparen);
					// break;	// what else?
				}
			}
		}
		else
			xpath += "*";
		
		while (m_lookahead == token_type::lbracket or
			   m_lookahead == token_type::dot or
			   m_lookahead == token_type::hash)
		{
			switch (m_lookahead)
			{
				case token_type::lbracket:
					do
					{
						xpath += m_token_string;
						match(m_lookahead);
					}
					while (m_lookahead != token_type::rbracket and m_lookahead != token_type::eof);
					xpath += m_token_string;
					match(token_type::rbracket);
					break;

				case token_type::dot:
					match(m_lookahead);
					xpath += "[@class='" + m_token_string + "']";
					match(token_type::object);
					break;

				case token_type::hash:
					xpath += "[@id='" + m_token_string.substr(1) + "']";
					result["by-id"] = true;
					match(m_lookahead);
					break;
				
				default:
					break;
			}
		}
	}

	result["xpath"] = xpath;

	return result;
}

// --------------------------------------------------------------------

object interpreter::parse_utility_expr()
{
	auto className = m_token_string;
	match(token_type::hash);
	match(token_type::dot);
	auto method = m_token_string;
	match(token_type::object);

	std::vector<object> params;
	if (m_lookahead == token_type::lparen)
	{
		match(token_type::lparen);
		while (m_lookahead != token_type::rparen)
		{
			params.push_back(parse_expr());
			
			if (m_lookahead == token_type::comma)
			{
				match(token_type::comma);
				continue;
			}

			break;
		}
		match(token_type::rparen);
	}

	return call_method(className, method, params);
}

object interpreter::call_method(const std::string& className, const std::string& method, std::vector<object>& params)
{
	object result;

	if (className[0] == '#')
		result = expression_utility_object_base::evaluate(m_scope, className.substr(1), method, params);

	// if (result.is_null())
	// 	throw runtime_error("Undefined class for utility object call: " + className);
	
	return result;
}

// --------------------------------------------------------------------
// some expression utility objects

expression_utility_object_base::instance* expression_utility_object_base::s_head;

class date_expr_util_object : public expression_utility_object<date_expr_util_object>
{
  public:
	static constexpr const char*	name()		{ return "dates"; }

	virtual object evaluate(const scope& scope, const std::string& method,
		const std::vector<object>& params) const
	{
		object result;

		if (method == "format")
		{
			if (params.size() == 2 and params[0].is_string())
			{
				tm t = {};
				std::istringstream ss(params[0].as<std::string>());

				ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
				if (ss.fail())	// hmmmm, lets try the ISO format then
					ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%SZ");
				
				std::wostringstream os;

				os.imbue(scope.get_request().get_locale());

				std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
				std::wstring time = myconv.from_bytes(params[1].as<std::string>());

				os << std::put_time(&t, time.c_str());

				result = os.str();
			}
		}

		return result;
	}
} s_date_instance;

class number_expr_util_object : public expression_utility_object<number_expr_util_object>
{
  public:
	static constexpr const char*	name()		{ return "numbers"; }

	virtual object evaluate(const scope& scope, const std::string& method,
		const std::vector<object>& params) const
	{
		object result;

		if (method == "formatDecimal")
		{
			if (params.size() >= 1 and params[0].is_number())
			{
				int intDigits = 1;
				if (params.size() >= 2 and params[1].is_number_int())
					intDigits = params[1].as<int>();
				
				int decimals = 0;
				if (params.size() >= 3 and params[2].is_number_int())
					decimals = params[2].as<int>();

				double d;
				if (params[0].is_number_int())
					d = static_cast<double>(params[0].as<int64_t>());
				else
					d = params[0].as<double>();

				return FormatDecimal(d, intDigits, decimals, scope.get_request().get_locale());
			}
		}
		else if (method == "formatDiskSize")
		{
			if (params.size() >= 1 and params[0].is_number())
			{
				double nr = params[0].as<double>();

				const char kBase[] = {'B', 'K', 'M', 'G', 'T', 'P', 'E'}; // whatever

				int base = 0;

				while (nr > 1024)
				{
					nr /= 1024;
					++base;
				}

				int decimals = 0;
				if (params.size() >= 2 and params[1].is_number_int())
					decimals = params[1].as<int>();

				return FormatDecimal(nr, 1, decimals, scope.get_request().get_locale()) + ' ' + kBase[base];
			}
		}

		return {};
	}
} s_number_instance;

class request_expr_util_object : public expression_utility_object<request_expr_util_object>
{
  public:
	static constexpr const char*	name()		{ return "request"; }

	virtual object evaluate(const scope& scope, const std::string& method,
		const std::vector<object>& params) const
	{
		object result;

		if (method == "getRequestURI")
			result = scope.get_request().get_uri();
		else if (method == "getRequestURL")
			result = scope.get_request().get_uri();
		else if ((method == "getParameter") and params.size() == 1)
			result = scope.get_request().get_parameter(params[0].as<std::string>().c_str());
		
		return result;
	}
} s_request_instance;

class security_expr_util_object : public expression_utility_object<security_expr_util_object>
{
  public:
	static constexpr const char*	name()		{ return "security"; }

	virtual object evaluate(const scope& scope, const std::string& method,
		const std::vector<object>& params) const
	{
		object result;

		if (method == "hasRole")
		{
			if (params.size() == 1 and params[0].is_string())
			{
				auto role = params[0].as<std::string>();
				auto roles = scope.get_credentials()["role"];
				result = roles.is_array() and roles.contains(role);
			}
		}
		else if (method == "authorized")
		{
			result = scope.get_credentials()["username"].is_string();
		}
		else if (method == "username")
		{
			result = scope.get_credentials()["username"];
		}

		return result;
	}
} s_security_instance;

// --------------------------------------------------------------------
// interpreter calls

bool process_el(const scope &scope, std::string &text)
{
	interpreter interpreter(scope);
	return interpreter.process(text);
}

std::string process_el_2(const scope& scope, const std::string& text)
{
	std::string s = text;

	interpreter interpreter(scope);
	return interpreter.process(s) ? s : text;
}

object evaluate_el(const scope &scope, const std::string &text)
{
	interpreter interpreter(scope);
	return interpreter.evaluate(text);
}

std::vector<std::pair<std::string,std::string>> evaluate_el_attr(const scope& scope, const std::string& text)
{
	interpreter interpreter(scope);
	return interpreter.evaluate_attr_expr(text);
}

bool evaluate_el_assert(const scope& scope, const std::string& text)
{
	interpreter interpreter(scope);
	return interpreter.evaluate_assert(text);
}

void evaluate_el_with(scope& scope, const std::string& text)
{
	interpreter interpreter(scope);
	interpreter.evaluate_with(scope, text);
}

object evaluate_el_link(const scope& scope, const std::string& text)
{
	interpreter interpreter(scope);
	return interpreter.evaluate_link(text);
}

// --------------------------------------------------------------------
// scope

std::ostream &operator<<(std::ostream &lhs, const scope &rhs)
{
	const scope *s = &rhs;
	while (s != nullptr)
	{
		for (scope::data_map::value_type e : s->m_data)
			lhs << e.first << " = " << e.second << std::endl;
		s = s->m_next;
	}
	return lhs;
}

scope::scope()
	: m_next(nullptr), m_depth(0), m_req(nullptr), m_server(nullptr)
{
}

scope::scope(const scope &next)
	: m_next(const_cast<scope *>(&next))
	, m_depth(next.m_depth + 1)
	, m_req(next.m_req)
	, m_server(next.m_server)
{
	if (m_depth > 1000)
		throw std::runtime_error("scope stack overflow");
}

scope::scope(const request &req)
	: m_next(nullptr), m_depth(0), m_req(&req), m_server(nullptr)
{
}

scope::scope(const basic_server& server, const request &req)
	: m_next(nullptr), m_depth(0), m_req(&req), m_server(&server)
{
}

object &scope::operator[](const std::string &name)
{
	return lookup(name);
}

const object& scope::lookup(const std::string &name, bool includeSelected) const
{
	const object* result = nullptr;

	auto i = m_data.find(name);
	if (i != m_data.end())
		result = &i->second;
	else if (includeSelected and m_selected.contains(name))
		result = &*m_selected.find(name);
	else if (m_next != nullptr)
		result = &m_next->lookup(name, includeSelected);

	if (result == nullptr)
	{
		static object s_null;
		result = &s_null;
	}
		
	return *result;
}

const object& scope::operator[](const std::string &name) const
{
	return lookup(name);
}

object& scope::lookup(const std::string &name)
{
	object* result = nullptr;

	auto i = m_data.find(name);
	if (i != m_data.end())
		result = &i->second;
	else if (m_next != nullptr)
		result = &m_next->lookup(name);

	if (result == nullptr)
	{
		m_data[name] = object();
		result = &m_data[name];
	}
		
	return *result;
}

const request& scope::get_request() const
{
	// if (m_next)
	// 	return m_next->get_request();
	if (m_req == nullptr)
		throw zeep::exception("Invalid scope, no request");
	return *m_req;
}

std::string scope::get_context_name() const
{
	if (not m_server)
		throw zeep::exception("Invalid scope, no server");
	return m_server->get_context_name();
}

json::element scope::get_credentials() const
{
	if (m_req == nullptr or m_server == nullptr)
		throw zeep::exception("Invalid scope, no request, no server");
	return m_req->get_credentials();
}

void scope::select_object(const object& o)
{
	m_selected = o;
}

auto scope::get_nodeset(const std::string& name) const -> node_set_type
{
	if (m_nodesets.count(name))
	{
		node_set_type result;

		// return a clone of the stored nodes.
		for (auto& n: m_nodesets.at(name))
			result.emplace_back(n->clone());

		return result;
	}
	
	if (m_next == nullptr)
		return {};

	return m_next->get_nodeset(name);
}

void scope::set_nodeset(const std::string& name, node_set_type&& nodes)
{
	m_nodesets.emplace(std::make_pair(name, std::move(nodes)));
}

std::string scope::get_csrf_token() const
{
	return get_request().get_cookie("csrf-token");
}

} // namespace zeep::http
