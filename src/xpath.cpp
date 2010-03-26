//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>

#include "zeep/exception.hpp"
#include "zeep/xml/node.hpp"
#include "zeep/xml/xpath.hpp"
#include "zeep/xml/unicode_support.hpp"

using namespace std;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct xpath_imp
{
						xpath_imp(const string& path);
						~xpath_imp();
	
	node_list			evaluate(node& root);

	enum Token {
		xp_Undef,
		xp_EOF,
		xp_LeftParenthesis,
		xp_RightParenthesis,
		xp_LeftBracket,
		xp_RightBracket,
		xp_Dot,
		xp_DoubleDot,
		xp_Slash,
		xp_DoubleSlash,
		xp_At,
		xp_Comma,
		xp_DoubleColon,
		xp_NameTest,
		xp_NodeType,

		xp_OperatorUnion,
		xp_OperatorAdd,
		xp_OperatorSubstract,
		xp_OperatorEqual,
		xp_OperatorNotEqual,
		xp_OperatorLess,
		xp_OperatorLessOrEqual,
		xp_OperatorGreater,
		xp_OperatorGreaterOrEqual,
		xp_OperatorAnd,
		xp_OperatorOr,
		xp_OperatorMod,
		xp_OperatorDiv,

		xp_FunctionName,
		xp_AxisName,
		xp_Literal,
		xp_Number,
		xp_Variable,
		xp_Asterisk,
		xp_Colon
	};

	void				parse(const string& path);
	
	unsigned char		next_byte();
	wchar_t				get_next_char();
	void				retract();
	Token				get_next_token();
	string				describe_token(Token token);
	void				match(Token token);
	
	void				location_path();
	void				absolute_location_path();
	void				relative_location_path();
	void				step();
	void				axis_specifier();
	void				axis_name();
	void				node_test();
	void				predicate();
	void				predicate_expr();
	
	void				abbr_absolute_location_path();
	void				abbr_relative_location_path();
	void				abbr_step();
	void				abbr_axis_specifier();
	
	void				expr();
	void				primary_expr();
	void				function_call();
	void				argument();
	
	void				union_expr();
	void				path_expr();
	void				filter_expr();

	void				or_expr();
	void				and_expr();
	void				equality_expr();
	void				relational_expr();
	
	void				additive_expr();
	void				multiplicative_expr();
	void				unary_expr();

	// 

	string::const_iterator
						m_begin, m_next, m_end;
	Token				m_lookahead;
	string				m_token_string;
	double				m_token_number;
	
	
};

void xpath_imp::parse(const string& path)
{
	m_begin = m_next = path.begin();
	m_end = path.end();
	
	m_lookahead = get_next_token();
	location_path();
}

unsigned char xpath_imp::next_byte()
{
	char result = 0;
	if (m_next != m_end)
	{
		result = *m_next;
		m_token_string += result;
		++m_next;
	}
	return static_cast<unsigned char>(result);
}

// We assume all paths are in valid UTF-8 encoding
wchar_t xpath_imp::get_next_char()
{
	unsigned long result = 0;
	unsigned char ch[5];
	
	ch[0] = next_byte();
	
	if ((ch[0] & 0x080) == 0)
		result = ch[0];
	else if ((ch[0] & 0x0E0) == 0x0C0)
	{
		ch[1] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<unsigned long>(((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F));
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<unsigned long>(((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F));
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<unsigned long>(((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F));
	}

	if (result > 0x10ffff)
		throw exception("invalid utf-8 character (out of range)");
	
	if (m_next == m_end)
		result = 0;
	
	return static_cast<wchar_t>(result);
}

void xpath_imp::retract()
{
	string::iterator c = m_token_string.end();

	// skip one valid character back in the input buffer
	// since we've arrived here, we can safely assume input
	// is valid UTF-8
	do --c; while ((*c & 0x0c0) == 0x080);
	
	m_next -= m_token_string.end() - c;
	m_token_string.erase(c, m_token_string.end());
}

string xpath_imp::describe_token(Token token)
{
	stringstream result;
	switch (token)
	{
		case xp_Undef: 				result << "undefined"; break;
		case xp_EOF:				result << "end of expression"; break;
		case xp_LeftParenthesis:	result << "left parenthesis"; break;
		case xp_RightParenthesis:	result << "right parenthesis"; break;
		case xp_LeftBracket:		result << "left bracket"; break;
		case xp_RightBracket:		result << "right bracket"; break;
		case xp_Dot:				result << "dot"; break;
		case xp_DoubleDot:			result << "double dot"; break;
		case xp_Slash:				result << "forward slash"; break;
		case xp_DoubleSlash:		result << "double forward slash"; break;
		case xp_At:					result << "at sign"; break;
		case xp_Comma:				result << "comma"; break;
		case xp_DoubleColon:		result << "double colon"; break;
		case xp_NameTest:			result << "name test"; break;
		case xp_NodeType:			result << "node type"; break;
//		case xp_Operator:			result << "operator"; break;
		case xp_OperatorUnion:		result << "union operator"; break;
		case xp_OperatorAdd:		result << "addition operator"; break;
		case xp_OperatorSubstract:	result << "substraction operator"; break;
		case xp_OperatorEqual:		result << "equals operator"; break;
		case xp_OperatorNotEqual:	result << "not-equals operator"; break;
		case xp_OperatorLess:		result << "less operator"; break;
		case xp_OperatorLessOrEqual:result << "less-or-equal operator"; break;
		case xp_OperatorGreater:	result << "greater operator"; break;
		case xp_OperatorGreaterOrEqual:
									result << "greater-or-equal operator"; break;
		case xp_OperatorAnd:		result << "logical-and operator"; break;
		case xp_OperatorOr:			result << "logical-or operator"; break;
		case xp_OperatorMod:		result << "modulus operator"; break;
		case xp_OperatorDiv:		result << "division operator"; break;

		case xp_FunctionName:		result << "function name"; break;
		case xp_AxisName:			result << "axis name"; break;
		case xp_Literal:			result << "literal"; break;
		case xp_Number:				result << "number"; break;
		case xp_Variable:			result << "variable"; break;
		case xp_Asterisk:			result << "asterisk"; break;
		case xp_Colon:				result << "colon"; break;
	}

	if (token != xp_EOF and token != xp_Undef)
		result << " (\"" << m_token_string << "\"";
	return result.str();
}

xpath_imp::Token xpath_imp::get_next_token()
{
	enum State {
		xps_Start,
		xps_FirstDot,
		xps_FirstColon,
		xps_VariableStart,
		xps_FirstSlash,
		xps_ExclamationMark,
		xps_LessThan,
		xps_GreaterThan,
		xps_Number,
		xps_NumberFraction,
		xps_Name,
		xps_QName,
		xps_QName2
	} start, state;
	start = state = xps_Start;

	Token token = xp_Undef;
	bool variable = false;
	double fraction;

	m_token_string.clear();
	
	while (token == xp_Undef)
	{
		wchar_t ch = get_next_char();
		
		switch (state)
		{
			case xps_Start:
				switch (ch)
				{
					case 0:		token = xp_EOF; break;
					case '(':	token = xp_LeftParenthesis; break;
					case ')':	token = xp_RightParenthesis; break;
					case '[':	token = xp_LeftBracket; break;
					case ']':	token = xp_RightBracket; break;
					case '.':	state = xps_FirstDot; break;
					case '@':	token = xp_At; break;
					case ',':	token = xp_Comma; break;
					case ':':	state = xps_FirstColon; break;
					case '$':	state = xps_VariableStart; break;
					case '*':	token = xp_Asterisk; break;
					case '/':	state = xps_FirstSlash; break;
					case '|':	token = xp_OperatorUnion; break;
					case '+':	token = xp_OperatorAdd; break;
					case '-':	token = xp_OperatorSubstract; break;
					case '=':	token = xp_OperatorEqual; break;
					case '!':	state = xps_ExclamationMark; break;
					case '<':	state = xps_LessThan; break;
					case '>':	state = xps_GreaterThan; break;
					case ' ':
					case '\n':
					case '\r':
					case '\t':
						m_token_string.clear();
						break;
					default:
						if (ch >= '0' and ch <= '9')
						{
							m_token_number = ch - '0';
							state = xps_Number;
						}
						else if (is_name_start_char(ch))
							state = xps_Name;
						else
							throw exception("invalid character in xpath");
				}
				break;
			
			case xps_FirstDot:
				if (ch == '.')
					token = xp_DoubleDot;
				else
				{
					retract();
					token = xp_Dot;
				}
				break;
			
			case xps_FirstSlash:
				if (ch != '/')
				{
					retract();
					token = xp_Slash;
				}
				else
					token = xp_DoubleSlash;
				break;
			
			case xps_FirstColon:
				if (ch == ':')
					token = xp_DoubleColon;
				else
				{
					retract();
					token = xp_Colon;
				}
				break;
			
			case xps_ExclamationMark:
				if (ch != '=')
				{
					retract();
					throw exception("unexpected character ('!') in xpath");
				}
				token = xp_OperatorNotEqual;
				break;
			
			case xps_LessThan:
				if (ch == '=')
					token = xp_OperatorLessOrEqual;
				else
				{
					retract();
					token = xp_OperatorLess;
				}
				break;

			case xps_GreaterThan:
				if (ch == '=')
					token = xp_OperatorGreaterOrEqual;
				else
				{
					retract();
					token = xp_OperatorGreater;
				}
				break;
			
			case xps_Number:
				if (ch >= '0' and ch <= '9')
					m_token_number = 10 * m_token_number + (ch - '0');
				else if (ch == '.')
				{
					fraction = 0.1;
					state = xps_NumberFraction;
				}
				else
				{
					retract();
					token = xp_Number;
				}
				break;
			
			case xps_NumberFraction:
				if (ch >= '0' and ch <= '9')
				{
					m_token_number += fraction * (ch - '0');
					fraction /= 10;
				}
				else
				{
					retract();
					token = xp_Number;
				}
				break;
			
			case xps_VariableStart:
				if (is_name_start_char(ch))
				{
					variable = true;
					state = xps_Name;
				}
				else
					throw exception("invalid variable name or lone dollar character");
				break;
			
			case xps_Name:
				if (ch == ':')
					state = xps_QName;
				else if (not is_name_char(ch))
				{
					retract();
					if (variable)
						token = xp_Variable;
					else if (m_token_string == "comment" or
							 m_token_string == "text" or
							 m_token_string == "processing-instruction" or
							 m_token_string == "node")
						token = xp_NodeType;
					else if (m_token_string == "and")
						token = xp_OperatorAnd;
					else if (m_token_string == "or")
						token = xp_OperatorOr;
					else if (m_token_string == "mod")
						token = xp_OperatorMod;
					else if (m_token_string == "div")
						token = xp_OperatorDiv;
					else if (m_token_string == "last" or
							 m_token_string == "position" or
							 m_token_string == "count" or
							 m_token_string == "id" or
							 m_token_string == "local-name" or
							 m_token_string == "namespace-uri" or
							 m_token_string == "name" or
							 m_token_string == "string" or
							 m_token_string == "concat" or
							 m_token_string == "starts-with" or
							 m_token_string == "contains" or
							 m_token_string == "substring-before" or
							 m_token_string == "substring-after" or
							 m_token_string == "string-length" or
							 m_token_string == "normalize-space" or
							 m_token_string == "translate" or
							 m_token_string == "boolean" or
							 m_token_string == "not" or
							 m_token_string == "true" or
							 m_token_string == "false" or
							 m_token_string == "lang" or
							 m_token_string == "number" or
							 m_token_string == "sum" or
							 m_token_string == "floor" or
							 m_token_string == "ceiling" or
							 m_token_string == "round" or
							 m_token_string == "comment")
						token = xp_FunctionName;
					else
						token = xp_NameTest;
				}
			
			case xps_QName:
				if (is_name_start_char(ch))
					state = xps_QName2;
				else
				{
					retract();	// ch
					retract();	// ':'
					if (variable)
						token = xp_Variable;
					else
						token = xp_NameTest;
				}
				break;
			
			case xps_QName2:
				if (not is_name_char(ch))
				{
					retract();
					if (variable)
						token = xp_Variable;
					else
						token = xp_NameTest;
				}
				break;
		}
	}
	
	return token;
}

void xpath_imp::match(Token token)
{
	if (m_lookahead == token)
		m_lookahead = get_next_token();
	else
	{
		// aargh... syntax error
		
		string found = describe_token(m_lookahead);
		string expected = describe_token(token);
		
		stringstream s;
		s << "syntax error in xpath, expected '" << expected << "' but found '" << found << "'";
		throw exception(s.str());
	}
}

void xpath_imp::location_path()
{
	bool absolute = false;
	bool abbreviated = false;
	
	if (m_lookahead == xp_Slash)
	{
		absolute = true;
		match(xp_Slash);
	}
	else if (m_lookahead == xp_DoubleSlash)
	{
		absolute = abbreviated = true;
		match(xp_DoubleSlash);
	}
	
	for (;;)
	{
		step();
		
		if (m_lookahead == xp_Slash)
			match(xp_Slash);
		else if (m_lookahead == xp_DoubleSlash)
			match(xp_DoubleSlash);
		else
			break;
	}
	
	match(xp_EOF);
}

void xpath_imp::step()
{
	// abbreviated steps
	if (m_lookahead == xp_Dot)
		;
	else if (m_lookahead == xp_DoubleDot)
		;
	else
	{
		axis_specifier();
		node_test();
		
		while (m_lookahead == xp_LeftBracket)
		{
			match(xp_LeftBracket);
			expr();
			match(xp_RightBracket);
		}
	}
}

void xpath_imp::axis_specifier()
{
	if (m_lookahead == xp_At)
		match(xp_At);
	else if (m_lookahead == xp_AxisName)
	{
		match(xp_AxisName);
		match(xp_DoubleColon);
	}
}

void xpath_imp::node_test()
{
	if (m_lookahead == xp_NodeType)
	{
		string nodeType = m_token_string;
		
		match(xp_NodeType);
		match(xp_LeftParenthesis);
		
		if (nodeType == "processing-instruction")
			match(xp_Literal);
		
		match(xp_RightParenthesis);
	}
	else
	{
		match(xp_NameTest);
	}
}

void xpath_imp::expr()
{
	for (;;)
	{
		and_expr();
		if (m_lookahead == xp_OperatorOr)
			match(xp_OperatorOr);
		else
			break;
	}
}

void xpath_imp::primary_expr()
{
	switch (m_lookahead)
	{
		case xp_Variable:
			match(xp_Variable);
			break;
		
		case xp_LeftParenthesis:
			match(xp_LeftParenthesis);
			expr();
			match(xp_RightParenthesis);
			break;
			
		case xp_Literal:
			match(xp_Literal);
			break;
		
		case xp_Number:
			match(xp_Number);
			break;
		
		case xp_FunctionName:
			function_call();
			break;
		
		default:
			throw exception("invalid primary expression in xpath");
	}
}

void xpath_imp::function_call()
{
	match(xp_FunctionName);
	match(xp_LeftParenthesis);
	if (m_lookahead != xp_RightParenthesis)
	{
		for (;;)
		{
			expr();
			if (m_lookahead == xp_Comma)
				match(xp_Comma);
			else
				break;
		}
	}
	match(xp_RightParenthesis);
}

void xpath_imp::union_expr()
{
	for (;;)
	{
		path_expr();
		if (m_lookahead == xp_OperatorUnion)
			match(m_lookahead);
		else
			break;
	}
}

void xpath_imp::path_expr()
{
	if (m_lookahead == xp_Variable or m_lookahead == xp_LeftParenthesis or
		m_lookahead == xp_Literal or m_lookahead == xp_Number or m_lookahead == xp_FunctionName)
	{
		for (;;)
		{
			filter_expr();
			if (m_lookahead == xp_Slash)
			{
				match(xp_Slash);
				relative_location_path();
			}
			else if (m_lookahead == xp_DoubleSlash)
			{
				match(xp_DoubleSlash);
				relative_location_path();
			}
			else
				break;
		}
	}
	else
		location_path();
}

void xpath_imp::filter_expr()
{
	for (;;)
	{
		primary_expr();
		while (m_lookahead == xp_LeftBracket)
		{
			match(xp_LeftBracket);
			expr();
			match(xp_RightBracket);
		}
	}	
}

void xpath_imp::and_expr()
{
	for (;;)
	{
		equality_expr();
		if (m_lookahead == xp_OperatorAnd)
			match(xp_OperatorAnd);
		else
			break;
	}
}

void xpath_imp::equality_expr()
{
	for (;;)
	{
		relational_expr();
		if (m_lookahead == xp_OperatorEqual)
			match(xp_OperatorEqual);
		else if (m_lookahead == xp_OperatorNotEqual)
			match(xp_OperatorNotEqual);
		else
			break;
	}
}

void xpath_imp::relational_expr()
{
	for (;;)
	{
		additive_expr();
		if (m_lookahead == xp_OperatorLess or m_lookahead == xp_OperatorLessOrEqual or
			m_lookahead == xp_OperatorGreater or m_lookahead == xp_OperatorGreaterOrEqual)
		{
			match(m_lookahead);
		}
		else
			break;
	}
}

void xpath_imp::additive_expr()
{
	for (;;)
	{
		multiplicative_expr();
		if (m_lookahead == xp_OperatorAdd or m_lookahead == xp_OperatorSubstract)
			match(m_lookahead);
		else
			break;
	}
}

void xpath_imp::multiplicative_expr()
{
	for (;;)
	{
		unary_expr();
		if (m_lookahead == xp_OperatorDiv or m_lookahead == xp_OperatorMod)
			match(m_lookahead);
		else
			break;
	}
}

void xpath_imp::unary_expr()
{
	if (m_lookahead == xp_OperatorSubstract)
	{
		match(xp_OperatorSubstract);
		unary_expr();
	}
	else
		union_expr();
}

// --------------------------------------------------------------------

xpath::xpath(const string& path)
	: m_impl(new xpath_imp(path))
{
}

xpath::~xpath()
{
	delete m_impl;
}

node_list xpath::evaluate(node& root)
{
	return m_impl->evaluate(root);
}

}
}
