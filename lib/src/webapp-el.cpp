// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp::el is a set of classes used to implement an 'expression language'
// A script language used in the XHTML templates used by the zeep webapp.
//

#include <zeep/config.hpp>

#include <iomanip>
#include <cmath>

#include <zeep/http/webapp.hpp>
#include <zeep/el/process.hpp>
#include <zeep/xml/unicode_support.hpp>

using namespace std;

namespace zeep
{
namespace el
{

using request = http::request;
using object = el::element;

// --------------------------------------------------------------------
// interpreter for expression language

struct interpreter
{
	typedef uint32_t unicode;

	interpreter(const scope &scope)
		: m_scope(scope) {}

	template <class OutputIterator, class Match>
	OutputIterator operator()(Match &m, OutputIterator out, boost::regex_constants::match_flag_type);

	object evaluate(const string &s);

	void process(string &s);

	void match(uint32_t t);

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

	const scope &m_scope;
	uint32_t m_lookahead;
	string m_token_string;
	double m_token_number;
	string::const_iterator m_ptr, m_end;
};

template <class OutputIterator, class Match>
inline OutputIterator interpreter::operator()(Match &m, OutputIterator out, boost::regex_constants::match_flag_type)
{
	string s(m[1]);

	process(s);

	copy(s.begin(), s.end(), out);

	return out;
}

enum token_type
{
	elt_undef,
	elt_eof,
	elt_number,
	elt_string,
	elt_object,
	elt_and,
	elt_or,
	elt_not,
	elt_empty,
	elt_eq,
	elt_ne,
	elt_lt,
	elt_le,
	elt_ge,
	elt_gt,
	elt_plus,
	elt_minus,
	elt_div,
	elt_mod,
	elt_mult,
	elt_lparen,
	elt_rparen,
	elt_lbracket,
	elt_rbracket,
	elt_if,
	elt_else,
	elt_dot,

	elt_true,
	elt_false,
	elt_in,
	elt_comma
};

object interpreter::evaluate(
	const string &s)
{
	object result;

	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();

		if (m_lookahead != elt_eof)
			result = parse_expr();
		match(elt_eof);
	}
	catch (exception & /* e */)
	{
		//		if (VERBOSE)
		//			cerr << e.what() << endl;
	}

	return result;
}

void interpreter::process(string &s)
{
	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();

		object result;

		if (m_lookahead != elt_eof)
			result = parse_expr();
		match(elt_eof);

		s = result.as<string>();
	}
	catch (exception &e)
	{
		//		if (VERBOSE)
		//			cerr << e.what() << endl;

		s = "error in el expression: ";
		s += e.what();
	}
}

void interpreter::match(uint32_t t)
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
	string::iterator c = m_token_string.end();

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
	enum State
	{
		els_Start,
		els_Equals,
		els_ExclamationMark,
		els_LessThan,
		els_GreaterThan,
		els_Number,
		els_NumberFraction,
		els_Name,
		els_Literal
	} state = els_Start;

	token_type token = elt_undef;
	double fraction = 1.0;
	unicode quoteChar = 0;

	m_token_string.clear();

	while (token == elt_undef)
	{
		unicode ch = get_next_char();

		switch (state)
		{
		case els_Start:
			switch (ch)
			{
			case 0:
				token = elt_eof;
				break;
			case '(':
				token = elt_lparen;
				break;
			case ')':
				token = elt_rparen;
				break;
			case '[':
				token = elt_lbracket;
				break;
			case ']':
				token = elt_rbracket;
				break;
			case ':':
				token = elt_else;
				break;
			case '?':
				token = elt_if;
				break;
			case '*':
				token = elt_mult;
				break;
			case '/':
				token = elt_div;
				break;
			case '+':
				token = elt_plus;
				break;
			case '-':
				token = elt_minus;
				break;
			case '.':
				token = elt_dot;
				break;
			case ',':
				token = elt_comma;
				break;
			case '=':
				state = els_Equals;
				break;
			case '!':
				state = els_ExclamationMark;
				break;
			case '<':
				state = els_LessThan;
				break;
			case '>':
				state = els_GreaterThan;
				break;
			case ' ':
			case '\n':
			case '\r':
			case '\t':
				m_token_string.clear();
				break;
			case '\'':
				quoteChar = ch;
				state = els_Literal;
				break;
			case '"':
				quoteChar = ch;
				state = els_Literal;
				break;

			default:
				if (ch >= '0' and ch <= '9')
				{
					m_token_number = ch - '0';
					state = els_Number;
				}
				else if (zeep::xml::is_name_start_char(ch))
					state = els_Name;
				else
					throw zeep::exception("invalid character (" + xml::to_hex(ch) + ") in expression");
			}
			break;

		case els_Equals:
			if (ch != '=')
				retract();
			token = elt_eq;
			break;

		case els_ExclamationMark:
			if (ch != '=')
			{
				retract();
				throw zeep::exception("unexpected character ('!') in expression");
			}
			token = elt_ne;
			break;

		case els_LessThan:
			if (ch == '=')
				token = elt_le;
			else
			{
				retract();
				token = elt_lt;
			}
			break;

		case els_GreaterThan:
			if (ch == '=')
				token = elt_ge;
			else
			{
				retract();
				token = elt_gt;
			}
			break;

		case els_Number:
			if (ch >= '0' and ch <= '9')
				m_token_number = 10 * m_token_number + (ch - '0');
			else if (ch == '.')
			{
				fraction = 0.1;
				state = els_NumberFraction;
			}
			else
			{
				retract();
				token = elt_number;
			}
			break;

		case els_NumberFraction:
			if (ch >= '0' and ch <= '9')
			{
				m_token_number += fraction * (ch - '0');
				fraction /= 10;
			}
			else
			{
				retract();
				token = elt_number;
			}
			break;

		case els_Name:
			if (ch == '.' or not zeep::xml::is_name_char(ch))
			{
				retract();
				if (m_token_string == "div")
					token = elt_div;
				else if (m_token_string == "mod")
					token = elt_mod;
				else if (m_token_string == "and")
					token = elt_and;
				else if (m_token_string == "or")
					token = elt_or;
				else if (m_token_string == "not")
					token = elt_not;
				else if (m_token_string == "empty")
					token = elt_empty;
				else if (m_token_string == "lt")
					token = elt_lt;
				else if (m_token_string == "le")
					token = elt_le;
				else if (m_token_string == "ge")
					token = elt_ge;
				else if (m_token_string == "gt")
					token = elt_gt;
				else if (m_token_string == "ne")
					token = elt_ne;
				else if (m_token_string == "eq")
					token = elt_eq;
				else if (m_token_string == "true")
					token = elt_true;
				else if (m_token_string == "false")
					token = elt_false;
				else if (m_token_string == "in")
					token = elt_in;
				else
					token = elt_object;
			}
			break;

		case els_Literal:
			if (ch == 0)
				throw zeep::exception("run-away string, missing quote character?");
			else if (ch == quoteChar)
			{
				token = elt_string;
				m_token_string = m_token_string.substr(1, m_token_string.length() - 2);
			}
			break;
		}
	}

	m_lookahead = token;
}

object interpreter::parse_expr()
{
	object result;
	
	for (;;)
	{
		result = parse_or_expr();

		if (m_lookahead == elt_if)
		{
			match(m_lookahead);
			object a = parse_expr();
			match(elt_else);
			object b = parse_expr();
			if (result.as<bool>())
				result = a;
			else
				result = b;
		}

		if (m_lookahead != elt_comma)
			break;

		match(elt_comma);
	}

	return result;
}

object interpreter::parse_or_expr()
{
	object result = parse_and_expr();
	while (m_lookahead == elt_or)
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
	while (m_lookahead == elt_and)
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
	if (m_lookahead == elt_eq)
	{
		match(m_lookahead);
		result = (result == parse_relational_expr());
	}
	else if (m_lookahead == elt_ne)
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
	case elt_lt:
		match(m_lookahead);
		result = (result < parse_additive_expr());
		break;
	case elt_le:
		match(m_lookahead);
		result = (result <= parse_additive_expr());
		break;
	case elt_ge:
		match(m_lookahead);
		result = (parse_additive_expr() <= result);
		break;
	case elt_gt:
		match(m_lookahead);
		result = (parse_additive_expr() < result);
		break;
	case elt_not:
	{
		match(elt_not);
		match(elt_in);

		object list = parse_additive_expr();

		result = not list.contains(result);
		break;
	}
	case elt_in:
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
	while (m_lookahead == elt_plus or m_lookahead == elt_minus)
	{
		if (m_lookahead == elt_plus)
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
	while (m_lookahead == elt_div or m_lookahead == elt_mod or m_lookahead == elt_mult)
	{
		if (m_lookahead == elt_mult)
		{
			match(m_lookahead);
			result = (result * parse_unary_expr());
		}
		else if (m_lookahead == elt_div)
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
	if (m_lookahead == elt_minus)
	{
		match(m_lookahead);
		result = -(parse_primary_expr());
	}
	else if (m_lookahead == elt_not)
	{
		match(m_lookahead);
		result = not parse_primary_expr().as<bool>();
	}
	else
		result = parse_primary_expr();
	return result;
}

object interpreter::parse_primary_expr()
{
	object result;
	switch (m_lookahead)
	{
	case elt_true:
		result = true;
		break;
	case elt_false:
		result = false;
		break;
	case elt_number:
		result = m_token_number;
		match(m_lookahead);
		break;
	case elt_string:
		result = m_token_string;
		match(m_lookahead);
		break;
	case elt_lparen:
		match(m_lookahead);
		result = parse_expr();
		match(elt_rparen);
		break;
	case elt_object:
		result = m_scope.lookup(m_token_string);
		match(elt_object);
		for (;;)
		{
			if (m_lookahead == elt_dot)
			{
				match(m_lookahead);
				if (result.type() == object::value_type::array and (m_token_string == "count" or m_token_string == "length"))
					result = object((uint32_t)result.size());
				else if (result.type() == object::value_type::object)
					result = const_cast<const object &>(result)[m_token_string];
				else
					result = object::value_type::null;
				match(elt_object);
				continue;
			}

			if (m_lookahead == elt_lbracket)
			{
				match(m_lookahead);

				object index = parse_expr();
				match(elt_rbracket);

				if (index.empty() or (result.type() != object::value_type::array and result.type() != object::value_type::object))
					result = object();
				else if (result.type() == object::value_type::array)
					result = result[index.as<int>()];
				else if (result.type() == object::value_type::object)
					result = result[index.as<string>()];
				else
					result = object::value_type::null;
				continue;
			}

			break;
		}
		break;
	case elt_empty:
		match(m_lookahead);
		if (m_lookahead != elt_object)
			throw zeep::exception("syntax error, expected an object after operator 'empty'");
		result = parse_primary_expr().empty();
		break;
	default:
		throw zeep::exception("syntax error, expected number, string or object");
	}
	return result;
}

// --------------------------------------------------------------------
// interpreter calls

bool process_el(const el::scope &scope, string &text)
{
	static const boost::regex re("\\$\\{([^}]+)\\}");

	el::interpreter interpreter(scope);

	ostringstream os;
	ostream_iterator<char, char> out(os);
	boost::regex_replace(out, text.begin(), text.end(), re, interpreter,
						 boost::match_default | boost::format_all);

	bool result = false;
	if (os.str() != text)
	{
		//cerr << "processed \"" << text << "\" => \"" << os.str() << '"' << endl;
		text = os.str();
		result = true;
	}

	return result;
}

void evaluate_el(const el::scope &scope, const string &text, el::object &result)
{
	static const boost::regex re("^\\$\\{([^}]+)\\}$");

	boost::smatch m;
	if (boost::regex_match(text, m, re))
	{
		el::interpreter interpreter(scope);
		result = interpreter.evaluate(m[1]);
		//cerr << "evaluated \"" << text << "\" => \"" << result << '"' << endl;
	}
	else
		result = text;
}

bool evaluate_el(const el::scope &scope, const string &text)
{
	el::object result;
	evaluate_el(scope, text, result);
	return result.as<bool>();
}

// --------------------------------------------------------------------
// scope

ostream &operator<<(ostream &lhs, const scope &rhs)
{
	const scope *s = &rhs;
	while (s != nullptr)
	{
		for (scope::data_map::value_type e : s->m_data)
			lhs << e.first << " = " << e.second << endl;
		s = s->m_next;
	}
	return lhs;
}

scope::scope(const scope &next)
	: m_next(const_cast<scope *>(&next)), m_req(nullptr)
{
}

scope::scope(const request &req)
	: m_next(nullptr), m_req(&req)
{
}

object &scope::operator[](const string &name)
{
	return lookup(name);
}

const object &scope::lookup(const string &name) const
{
	map<string, object>::const_iterator i = m_data.find(name);
	if (i != m_data.end())
		return i->second;
	else if (m_next != nullptr)
		return m_next->lookup(name);

	static object s_null;
	return s_null;
}

const object &scope::operator[](const string &name) const
{
	return lookup(name);
}

object &scope::lookup(const string &name)
{
	object *result = nullptr;

	map<string, object>::iterator i = m_data.find(name);
	if (i != m_data.end())
		result = &i->second;
	else if (m_next != nullptr)
		result = &m_next->lookup(name);
	else
	{
		m_data[name] = object();
		result = &m_data[name];
	}

	return *result;
}

const request &scope::get_request() const
{
	if (m_next)
		return m_next->get_request();
	if (m_req == nullptr)
		throw zeep::exception("Invalid scope, no request");
	return *m_req;
}

} // namespace el
} // namespace zeep
