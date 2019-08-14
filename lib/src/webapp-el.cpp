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
#include <locale>
#include <codecvt>

#include <zeep/http/webapp.hpp>
#include <zeep/el/process.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/el/utils.hpp>

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

	vector<pair<string,string>> evaluate_attr_expr(const string& s);

	bool process(string &s);

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

	// --------------------------------------------------------------------
	// these are for inside ${} templates

	object parse_template_expr();					// or_expr ( '?' expr ':' expr )?
	object parse_template_or_expr();				// and_expr ( 'or' and_expr)*
	object parse_template_and_expr();				// equality_expr ( 'and' equality_expr)*
	object parse_template_equality_expr();			// relational_expr ( ('=='|'!=') relational_expr )?
	object parse_template_relational_expr();		// additive_expr ( ('<'|'<='|'>='|'>') additive_expr )*
	object parse_template_additive_expr();			// multiplicative_expr ( ('+'|'-') multiplicative_expr)*
	object parse_template_multiplicative_expr();	 // unary_expr (('%'|'/') unary_expr)*
	object parse_template_unary_expr();				// ('-')? primary_expr
	object parse_template_primary_expr();			// '(' expr ')' | number | string

	object parse_utility_expr();

	object call_method(const string& className, const string& method, vector<object>& params);

	const scope &m_scope;
	uint32_t m_lookahead;
	string m_token_string;
	int64_t m_token_number_int;
	double m_token_number_float;
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
	elt_number_int,
	elt_number_float,
	elt_string,
	elt_object,
	elt_and,
	elt_or,
	elt_not,
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
	elt_lbrace,
	elt_rbrace,
	elt_if,
	elt_elvis,
	elt_else,
	elt_dot,
	elt_hash,

	elt_true,
	elt_false,
	elt_in,
	elt_comma,

	elt_variable_template,
	elt_selection_template,
	elt_message_template,
	elt_link_template,
	elt_fragment_template
};

object interpreter::evaluate(const string &s)
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
	catch (const exception& e)
	{
		result = "Error parsing expression: "s + e.what();
	}

	return result;
}

vector<pair<string,string>> interpreter::evaluate_attr_expr(const string& s)
{
	vector<pair<string,string>> result;

	m_ptr = s.begin();
	m_end = s.end();
	get_next_token();

	for (;;)
	{
		string var = m_token_string;
		match(elt_object);

		match (elt_eq);

		auto value = parse_expr();

		result.emplace_back(var, value.as<string>());

		if (m_lookahead != elt_comma)
			break;
		
		match(elt_comma);
	}

	match(elt_eof);

	return result;
}

bool interpreter::process(string &s)
{
	bool result = false;

	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();

		object obj;

		if (m_lookahead != elt_eof)
			obj = parse_expr();
		match(elt_eof);

		s = obj.as<string>();

		result = true;
	}
	catch (exception &e)
	{
		//		if (VERBOSE)
		//			cerr << e.what() << endl;

		s = "error in el expression: ";
		s += e.what();
	}

	return result;
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
		els_Question,
		els_Number,
		els_NumberFraction,
		els_Name,
		els_Literal,

		els_Hash,

		els_TemplateStart
	} state = els_Start;

	token_type token = elt_undef;
	double fraction = 1.0;

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
			case '{':
				token = elt_lbrace;
				break;
			case '}':
				token = elt_rbrace;
				break;
			case ':':
				token = elt_else;
				break;
			case '?':
				state = els_Question;
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

			case '*':
			case '$':
			case '#':
			case '@':
			case '~':
				state = els_TemplateStart;
				break;

			case ' ':
			case '\n':
			case '\r':
			case '\t':
				m_token_string.clear();
				break;
			case '\'':
				state = els_Literal;
				break;

			default:
				if (ch >= '0' and ch <= '9')
				{
					m_token_number_int = ch - '0';
					state = els_Number;
				}
				else if (zeep::xml::is_name_start_char(ch))
					state = els_Name;
				else
					throw zeep::exception("invalid character (" + xml::to_hex(ch) + ") in expression");
			}
			break;

		case els_TemplateStart:
			if (ch == '{')
			{
				switch (m_token_string[0])
				{
					case '$':	token = elt_variable_template; break;
					case '*':	token = elt_selection_template; break;
					case '#':	token = elt_message_template; break;
					case '@':	token = elt_link_template; break;
					case '~':	token = elt_fragment_template; break;
					default: assert(false);
				}
			}
			else
			{
				retract();
				if (m_token_string[0] == '*')
					token = elt_mult;
				else if (m_token_string[0] == '#')
					state = els_Hash;
				else
					throw zeep::exception("invalid character (" + string{static_cast<char>(isprint(ch) ? ch : ' ')} + '/' + xml::to_hex(ch) + ") in expression");
			}
			break;

		case els_Equals:
			if (ch != '=')
				retract();
			token = elt_eq;
			break;

		case els_Question:
			if (ch == ':')
				token = elt_elvis;
			else
			{
				retract();
				token = elt_if;
			}
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
				m_token_number_int = 10 * m_token_number_int + (ch - '0');
			else if (ch == '.')
			{
				m_token_number_float = m_token_number_int;
				fraction = 0.1;
				state = els_NumberFraction;
			}
			else
			{
				retract();
				token = elt_number_int;
			}
			break;

		case els_NumberFraction:
			if (ch >= '0' and ch <= '9')
			{
				m_token_number_float += fraction * (ch - '0');
				fraction /= 10;
			}
			else
			{
				retract();
				token = elt_number_float;
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
			else if (ch == '\'')
			{
				token = elt_string;
				m_token_string = m_token_string.substr(1, m_token_string.length() - 2);
			}
			break;
		
		case els_Hash:
			if (ch == '.' or not zeep::xml::is_name_char(ch))
			{
				retract();
				token = elt_hash;
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

	if (m_lookahead == elt_if)
	{
		match(m_lookahead);
		object a = parse_expr();

		if (m_lookahead == elt_else)
		{
			match(elt_else);
			object b = parse_expr();
			if (result)
				result = a;
			else
				result = b;
		}
		else if (result)
			result = a;
	}
	else if (m_lookahead == elt_elvis)
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
		case elt_variable_template:
			match(elt_variable_template);
			result = parse_template_expr();
			match(elt_rbrace);
			break;
		
		case elt_selection_template:
			match(elt_selection_template);
			if (m_lookahead == elt_object)
			{
				result = m_scope.lookup(m_token_string, true);
				match(elt_object);
			}
			else
				result = parse_template_expr();
			match(elt_rbrace);
			break;

		case elt_true:
			result = true;
			match(m_lookahead);
			break;
		case elt_false:
			result = false;
			match(m_lookahead);
			break;
		case elt_number_int:
			result = m_token_number_int;
			match(m_lookahead);
			break;
		case elt_number_float:
			result = m_token_number_float;
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

		default:
			throw zeep::exception("syntax error, expected number, string or object");
	}
	return result;
}

// --------------------------------------------------------------------

object interpreter::parse_template_expr()
{
	object result;
	
	result = parse_template_or_expr();

	if (m_lookahead == elt_if)
	{
		match(m_lookahead);
		object a = parse_template_expr();

		if (m_lookahead == elt_else)
		{
			match(elt_else);
			object b = parse_template_expr();
			if (result)
				result = a;
			else
				result = b;
		}
		else if (result)
			result = a;
	}
	else if (m_lookahead == elt_elvis)
	{
		match(m_lookahead);
		object a = parse_template_expr();

		if (not result)
			result = a;
	}

	return result;
}

object interpreter::parse_template_or_expr()
{
	object result = parse_template_and_expr();
	while (m_lookahead == elt_or)
	{
		match(m_lookahead);
		bool b1 = result.as<bool>();
		bool b2 = parse_template_and_expr().as<bool>();
		result = b1 or b2;
	}
	return result;
}

object interpreter::parse_template_and_expr()
{
	object result = parse_template_equality_expr();
	while (m_lookahead == elt_and)
	{
		match(m_lookahead);
		bool b1 = result.as<bool>();
		bool b2 = parse_template_equality_expr().as<bool>();
		result = b1 and b2;
	}
	return result;
}

object interpreter::parse_template_equality_expr()
{
	object result = parse_template_relational_expr();
	if (m_lookahead == elt_eq)
	{
		match(m_lookahead);
		result = (result == parse_template_relational_expr());
	}
	else if (m_lookahead == elt_ne)
	{
		match(m_lookahead);
		result = not(result == parse_template_relational_expr());
	}
	return result;
}

object interpreter::parse_template_relational_expr()
{
	object result = parse_template_additive_expr();
	switch (m_lookahead)
	{
		case elt_lt:
			match(m_lookahead);
			result = (result < parse_template_additive_expr());
			break;
		case elt_le:
			match(m_lookahead);
			result = (result <= parse_template_additive_expr());
			break;
		case elt_ge:
			match(m_lookahead);
			result = (parse_template_additive_expr() <= result);
			break;
		case elt_gt:
			match(m_lookahead);
			result = (parse_template_additive_expr() < result);
			break;
		case elt_not:
		{
			match(elt_not);
			match(elt_in);

			object list = parse_template_additive_expr();

			result = not list.contains(result);
			break;
		}
		case elt_in:
		{
			match(m_lookahead);
			object list = parse_template_additive_expr();

			result = list.contains(result);
			break;
		}
		default:
			break;
	}
	return result;
}

object interpreter::parse_template_additive_expr()
{
	object result = parse_template_multiplicative_expr();
	while (m_lookahead == elt_plus or m_lookahead == elt_minus)
	{
		if (m_lookahead == elt_plus)
		{
			match(m_lookahead);
			result = (result + parse_template_multiplicative_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result - parse_template_multiplicative_expr());
		}
	}
	return result;
}

object interpreter::parse_template_multiplicative_expr()
{
	object result = parse_template_unary_expr();
	while (m_lookahead == elt_div or m_lookahead == elt_mod or m_lookahead == elt_mult)
	{
		if (m_lookahead == elt_mult)
		{
			match(m_lookahead);
			result = (result * parse_template_unary_expr());
		}
		else if (m_lookahead == elt_div)
		{
			match(m_lookahead);
			result = (result / parse_template_unary_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result % parse_template_unary_expr());
		}
	}
	return result;
}

object interpreter::parse_template_unary_expr()
{
	object result;
	if (m_lookahead == elt_minus)
	{
		match(m_lookahead);
		result = -(parse_template_primary_expr());
	}
	else if (m_lookahead == elt_not)
	{
		match(m_lookahead);
		result = not parse_template_primary_expr().as<bool>();
	}
	else
		result = parse_template_primary_expr();
	return result;
}

object interpreter::parse_template_primary_expr()
{
	object result;
	switch (m_lookahead)
	{
		case elt_true:
			result = true;
			match(m_lookahead);
			break;
		case elt_false:
			result = false;
			match(m_lookahead);
			break;
		case elt_number_int:
			result = m_token_number_int;
			match(m_lookahead);
			break;
		case elt_number_float:
			result = m_token_number_float;
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

		case elt_hash:
			result = parse_utility_expr();
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
					else if (m_token_string == "empty")
						result = result.empty();
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

					object index = parse_template_expr();
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

		default:
			throw zeep::exception("syntax error, expected number, string or object");
	}
	return result;
}

object interpreter::parse_utility_expr()
{
	auto className = m_token_string;
	match(elt_hash);
	match(elt_dot);
	auto method = m_token_string;
	match(elt_object);

	vector<object> params;
	if (m_lookahead == elt_lparen)
	{
		match(elt_lparen);
		while (m_lookahead != elt_rparen)
		{
			params.push_back(parse_template_expr());
			
			if (m_lookahead == elt_comma)
			{
				match(elt_comma);
				continue;
			}

			break;
		}
		match(elt_rparen);
	}

	return call_method(className, method, params);
}

object interpreter::call_method(const string& className, const string& method, vector<object>& params)
{
	object result;
	if (className == "#dates")
	{
		if (method == "format")
		{
			if (params.size() == 2 and params[0].is_string())
			{
				tm t = {};
				istringstream ss(params[0].as<string>());
				// ss.imbue(std::locale("de_DE.utf-8"));

				ss >> get_time(&t, "%Y-%m-%d %H:%M:%S");
				if (ss.fail())
					throw runtime_error("Invalid date");
				
				wostringstream os;

				os.imbue(m_scope.get_request().get_locale());

				std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
				wstring time = myconv.from_bytes(params[1].as<string>());

				os << put_time(&t, time.c_str());

				result = os.str();
			}
		}
		else
			throw runtime_error("Undefined method " + method + " for utility object " + className);	
	}
	else if (className == "#numbers")
	{
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
					d = params[0].as<int64_t>();
				else
					d = params[0].as<double>();

				return FormatDecimal(d, intDigits, decimals, m_scope.get_request().get_locale());
			}
		}
		else
			throw runtime_error("Undefined method " + method + " for utility object " + className);	
	}
	else
		throw runtime_error("Undefined class for utility object call: " + className);
	
	return result;
}

// --------------------------------------------------------------------
// interpreter calls

bool process_el(const scope &scope, string &text)
{
	interpreter interpreter(scope);
	return interpreter.process(text);
}

object evaluate_el(const scope &scope, const string &text)
{
	interpreter interpreter(scope);
	return interpreter.evaluate(text);
}

vector<pair<string,string>> evaluate_el_attr(const scope& scope, const string& text)
{
	interpreter interpreter(scope);
	return interpreter.evaluate_attr_expr(text);
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

const object& scope::lookup(const string &name, bool includeSelected) const
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

const object& scope::operator[](const string &name) const
{
	return lookup(name);
}

object& scope::lookup(const string &name)
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

const request &scope::get_request() const
{
	if (m_next)
		return m_next->get_request();
	if (m_req == nullptr)
		throw zeep::exception("Invalid scope, no request");
	return *m_req;
}

void scope::select_object(const object& o)
{
	m_selected = o;
}

} // namespace el
} // namespace zeep
