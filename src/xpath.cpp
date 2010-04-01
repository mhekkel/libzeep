//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <numeric>
#include <stack>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "zeep/exception.hpp"
#include "zeep/xml/node.hpp"
#include "zeep/xml/xpath.hpp"
#include "zeep/xml/document.hpp"
#include "zeep/xml/unicode_support.hpp"

#define nil NULL

extern int VERBOSE;

using namespace std;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

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
	xp_Name,
	
	xp_AxisName,
	xp_FunctionName,
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

	xp_Literal,
	xp_Number,
	xp_Variable,
	xp_Asterisk,
	xp_Colon
};

enum AxisType {
	ax_Ancestor,
	ax_AncestorOrSelf,
	ax_Attribute,
	ax_Child,
	ax_Descendant,
	ax_DescendantOrSelf,
	ax_Following,
	ax_FollowingSibling,
	ax_Namespace,
	ax_Parent,
	ax_Preceding,
	ax_PrecedingSibling,
	ax_Self,
	
	ax_Count
};

const char* kAxisNames[] = {
	"ancestor",
	"ancestor-or-self",
	"attribute",
	"child",
	"descendant",
	"descendant-or-self",
	"following",
	"following-sibling",
	"namespace",
	"parent",
	"preceding",
	"preceding-sibling",
	"self",
	nil
};

//	{ "and",					xp_OperatorAnd,	0 },
//	{ "or",						xp_OperatorOr,	0 },
//	{ "mod",					xp_OperatorMod,	0 },
//	{ "div",					xp_OperatorDiv,	0 },

const char* kCoreFunctionNames[] = {
	"last",
	"position",
	"count",
	"id",
	"local-name",
	"namespace-uri",
	"name",
	"string",
	"concat",
	"starts-with",
	"contains",
	"substring-before",
	"substring-after",
	"string-length",
	"normalize-space",
	"translate",
	"boolean",
	"not",
	"true"	,
	"false",
	"lang",
	"number",
	"sum",
	"floor",
	"ceiling",
	"round",
	"comment",
};

const int kCoreFunctionCount = sizeof(kCoreFunctionNames) / sizeof(const char*);

// the expressions implemented as interpreter objects

enum object_type
{
	ot_undef,
	ot_node_set,
	ot_boolean,
	ot_number,
	ot_string
};

class object
{
  public:
						object();
						object(node_set ns);
						object(bool b);
						object(double n);
						object(const string& s);
						object(const object& o);
	object&				operator=(const object& o);

	object_type			type() const;

	template<typename T>
	T&					as();
	
  private:
	object_type			m_type;
	node_set			m_node_set;
	bool				m_boolean;
	double				m_number;
	string				m_string;
};

template<>
node_set& object::as<node_set>()
{
	if (m_type != ot_node_set)
		throw exception("object is not of type node-set");
	return m_node_set;
}

// --------------------------------------------------------------------

template<typename PREDICATE>
void iterate_children(element* context, node_set& s, bool deep, PREDICATE pred)
{
	for (node* child = context->child(); child != nil; child = child->next())
	{
		element* e = dynamic_cast<element*>(child);
		if (e == nil)
			continue;

		if (s.count(child) > 0)
			continue;

		if (pred(e))
			s.push_back(child);

		if (deep)
			iterate_children(e, s, true, pred);
	}
}

template<typename PREDICATE>
void iterate_ancestor(element* e, node_set& s, PREDICATE pred)
{
	for (;;)
	{
		e = dynamic_cast<element*>(e->parent());
		
		if (e == nil)
			break;
		
		document* d = dynamic_cast<document*>(e);
		if (d != nil)
			break;

		if (pred(e))
			s.push_back(e);
	}
}

template<typename PREDICATE>
void iterate_preceding(node* n, node_set& s, bool sibling, PREDICATE pred)
{
	while (n != nil)
	{
		if (n->prev() == nil)
		{
			if (sibling)
				break;
			
			n = n->parent();
			continue;
		}
		
		n = n->prev();
		
		element* e = dynamic_cast<element*>(n);
		if (e == nil)
			continue;
		
		if (pred(e))
			s.push_back(e);
		
		if (sibling == false)
			iterate_children(e, s, true, pred);
	}
}

template<typename PREDICATE>
void iterate_following(node* n, node_set& s, bool sibling, PREDICATE pred)
{
	while (n != nil)
	{
		if (n->next() == nil)
		{
			if (sibling)
				break;
			
			n = n->parent();
			continue;
		}
		
		n = n->next();
		
		element* e = dynamic_cast<element*>(n);
		if (e == nil)
			continue;
		
		if (pred(e))
			s.push_back(e);
		
		if (sibling == false)
			iterate_children(e, s, true, pred);
	}
}

// --------------------------------------------------------------------

class expression
{
  public:
	virtual				~expression() {}
	virtual object		evaluate(object& context) = 0;
};

typedef boost::shared_ptr<expression>	expression_ptr;
typedef list<expression_ptr>			expression_list;

// --------------------------------------------------------------------

class step_expression : public expression
{
  public:
						step_expression(AxisType axis) : m_axis(axis) {}

  protected:

	template<typename T>
	object				evaluate(object& arg, T pred);

	AxisType			m_axis;
};

template<typename T>
object step_expression::evaluate(object& arg, T pred)
{
	node_set result;
	
	if (arg.type() != ot_node_set)
		throw exception("expected node-set in step expression");
	
	node_set context_set = arg.as<node_set>();

	foreach (node& context, context_set)
	{
		element* context_element = dynamic_cast<element*>(&context);
		if (context_element != nil)
		{
			switch (m_axis)
			{
				case ax_Parent:
					if (context_element->parent() != nil)
					{
						element* e = static_cast<element*>(context_element->parent());
						if (pred(e))
							result.push_back(context_element->parent());
					}
					break;
				
				case ax_Ancestor:
					iterate_ancestor(context_element, result, pred);
					break;

				case ax_AncestorOrSelf:
					if (pred(context_element))
						result.push_back(context_element);
					iterate_ancestor(context_element, result, pred);
					break;
				
				case ax_Self:
					if (pred(context_element))
						result.push_back(context_element);
					break;
				
				case ax_Child:
					iterate_children(context_element, result, false, pred);
					break;
	
				case ax_Descendant:
					iterate_children(context_element, result, true, pred);
					break;

				case ax_DescendantOrSelf:
					if (pred(context_element))
						result.push_back(context_element);
					iterate_children(context_element, result, true, pred);
					break;
				
				case ax_Following:
					iterate_following(context_element, result, false, pred);
					break;

				case ax_FollowingSibling:
					iterate_following(context_element, result, true, pred);
					break;
			
				case ax_Preceding:
					iterate_preceding(context_element, result, false, pred);
					break;

				case ax_PrecedingSibling:
					iterate_preceding(context_element, result, true, pred);
					break;
	
				default:
					throw exception("unimplemented axis");
			}
		}
	}

	return result;
}

// --------------------------------------------------------------------

class name_test_step_expression : public step_expression
{
  public:
						name_test_step_expression(AxisType axis, const string& name)
							: step_expression(axis)
							, m_name(name)
						{
							if (m_name == "*")
								m_test = boost::bind(&name_test_step_expression::asterisk, _1);
							else
								m_test = boost::bind(&element::name, _1) == m_name;
						}

	virtual object		evaluate(object& arg);

  protected:

	static bool			asterisk(const element*)					{ return true; }

	string									m_name;
	boost::function<bool(const element*)>	m_test;
};

object name_test_step_expression::evaluate(object& arg)
{
	return step_expression::evaluate(arg, m_test);
}

// --------------------------------------------------------------------

template<typename T>
class node_type_expression : public step_expression
{
  public:
						node_type_expression(AxisType axis)
							: step_expression(axis)
						{
							m_test = boost::bind(&node_type_expression::test, _1);
						}

	virtual object		evaluate(object& arg);

  private:
	static bool			test(const node* n)
						{
							return dynamic_cast<T*>(n) != nil;
						}

	boost::function<bool(const element*)>	m_test;
};

template<typename T>
object node_type_expression<T>::evaluate(object& arg)
{
	return step_expression::evaluate(arg, m_test);
}

// --------------------------------------------------------------------

class document_expression : public expression
{
  public:
	virtual object		evaluate(object& arg);
};

object document_expression::evaluate(object& arg)
{
	assert(arg.type() == ot_node_set);
	assert(arg.as<node_set>().size() == 1);
	
	node_set result;
	result.push_back(arg.as<node_set>().front().doc());

	return result;
}

// --------------------------------------------------------------------

class or_expression : public expression
{
  public:
						or_expression(expression_ptr lhs, expression_ptr rhs)
							: m_lhs(lhs), m_rhs(rhs) {}

	virtual object		evaluate(object& arg);

  private:
	expression_ptr		m_lhs, m_rhs;
};

object or_expression::evaluate(object& arg)
{
	object v1 = m_lhs->evaluate(arg);
	object v2 = m_rhs->evaluate(arg);
	
	return v1.as<bool>() and v2.as<bool>();
}

// --------------------------------------------------------------------

class path_expression : public expression
{
  public:
						path_expression(expression_ptr lhs, expression_ptr rhs)
							: m_lhs(lhs), m_rhs(rhs) {}

	virtual object		evaluate(object& arg);

  private:
	expression_ptr		m_lhs, m_rhs;
};

object path_expression::evaluate(object& arg)
{
	object v = m_lhs->evaluate(arg);
	if (v.type() != ot_node_set)
		throw exception("filter does not evaluate to a node-set");
	
	return m_rhs->evaluate(v);
}

// --------------------------------------------------------------------

class variable_expression : public expression
{
  public:
						variable_expression(const string& name)
							: m_var(name) {}

	virtual object		evaluate(object& arg);

  private:
	string				m_var;
};

object variable_expression::evaluate(object& arg)
{
	throw exception("variables are not supported yet");
	return object();
}

// --------------------------------------------------------------------

class literal_expression : public expression
{
  public:
						literal_expression(const string& lit)
							: m_lit(lit) {}

	virtual object		evaluate(object& arg);

  private:
	string				m_lit;
};

object literal_expression::evaluate(object& arg)
{
	return object(m_lit);
}

// --------------------------------------------------------------------

class number_expression : public expression
{
  public:
						number_expression(double number)
							: m_number(number) {}

	virtual object		evaluate(object& arg);

  private:
	double				m_number;
};

object number_expression::evaluate(object& arg)
{
	return object(m_number);
}

// --------------------------------------------------------------------

class core_function_expression : public expression
{
  public:
						core_function_expression(int function_nr, expression_list& arguments)
							: m_function_nr(function_nr), m_args(arguments) {}

	virtual object		evaluate(object& arg);

  private:
	int					m_function_nr;
	expression_list		m_args;
};

object core_function_expression::evaluate(object& arg)
{
	object result;
	
	switch (m_function_nr)
	{
		
		default: throw exception("unimplemented function ");
	}
	
	return result;
}

// --------------------------------------------------------------------

class union_expression : public expression
{
  public:
						union_expression(expression_ptr lhs, expression_ptr rhs)
							: m_lhs(lhs), m_rhs(rhs) {}

	virtual object		evaluate(object& arg);

  private:
	expression_ptr		m_lhs, m_rhs;
};

object union_expression::evaluate(object& arg)
{
	object v1 = m_lhs->evaluate(arg);
	object v2 = m_rhs->evaluate(arg);
	
	if (v1.type() != ot_node_set or v2.type() != ot_node_set)
		throw exception("union operator works only on node sets");
	
	node_set s1 = v1.as<node_set>();
	node_set s2 = v2.as<node_set>();
	
	copy(s2.begin(), s2.end(), back_inserter(s1));
	
	return s1;
}

// --------------------------------------------------------------------

struct xpath_imp
{
						xpath_imp(const string& path);
						~xpath_imp();
	
	node_set			evaluate(node& root);

	void				parse(const string& path);
	
	unsigned char		next_byte();
	wchar_t				get_next_char();
	void				retract();
	Token				get_next_token();
	string				describe_token(Token token);
	void				match(Token token);
	
	expression_ptr		location_path();
	expression_ptr		absolute_location_path();
	expression_ptr		relative_location_path();
	expression_ptr		step();
	AxisType			axis_specifier();
	expression_ptr		node_test(AxisType axis);
//	expression_ptr		predicate();
//	expression_ptr		predicate_expr();
	
//	expression_ptr		abbr_absolute_location_path();
//	expression_ptr		abbr_relative_location_path();
//	expression_ptr		abbr_step();
//	expression_ptr		abbr_axis_specifier();
	
	expression_ptr		expr();
	expression_ptr		primary_expr();
	expression_ptr		function_call();
	expression_ptr		argument();
	
	expression_ptr		union_expr();
	expression_ptr		path_expr();
	expression_ptr		filter_expr();

	expression_ptr		or_expr();
	expression_ptr		and_expr();
	expression_ptr		equality_expr();
	expression_ptr		relational_expr();
	
	expression_ptr		additive_expr();
	expression_ptr		multiplicative_expr();
	expression_ptr		unary_expr();

	// 
	struct state {
		string::const_iterator	begin, next, end;
	};
	
	void				push_state(const string& replacement);
	void				pop_state();

	static const string	kDoubleSlash;

	string				m_path;
	string::const_iterator
						m_begin, m_next, m_end;
	Token				m_lookahead;
	string				m_token_string;
	double				m_token_number;
	AxisType			m_token_axis;
	int					m_token_function;

	
	// the generated expression
	expression_ptr		m_expr;
	stack<state>		m_state;
};

const string xpath_imp::kDoubleSlash("descendant-or-self::node()/");

// --------------------------------------------------------------------

xpath_imp::xpath_imp(const string& path)
	: m_path(path)
	, m_begin(m_path.begin())
	, m_next(m_begin)
	, m_end(m_path.end())
{
}

xpath_imp::~xpath_imp()
{
}	

void xpath_imp::parse(const string& path)
{
	m_begin = m_next = path.begin();
	m_end = path.end();
	
	m_lookahead = get_next_token();
	location_path();

	match(xp_EOF);
}

void xpath_imp::push_state(const string& replacement)
{
	state s = { m_begin, m_next, m_end };
	m_state.push(s);
	m_begin = m_next = replacement.begin();
	m_end = replacement.end();
}

void xpath_imp::pop_state()
{
	m_begin = m_state.top().begin;
	m_next = m_state.top().next;
	m_end = m_state.top().end;
	
	m_state.pop();
}

unsigned char xpath_imp::next_byte()
{
	char result = 0;
	
	while (m_next == m_end and not m_state.empty())
		pop_state();

	if (m_next < m_end)
		result = *m_next;

	++m_next;
	m_token_string += result;

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
		case xp_Name:				result << "name"; break;
		case xp_AxisName:			result << "axis specification"; break;
		case xp_FunctionName:		result << "function name"; break;
		case xp_NodeType:			result << "node type specification"; break;
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

		case xp_Literal:			result << "literal"; break;
		case xp_Number:				result << "number"; break;
		case xp_Variable:			result << "variable"; break;
		case xp_Asterisk:			result << "asterisk"; break;
		case xp_Colon:				result << "colon"; break;
	}

	if (token != xp_EOF and token != xp_Undef)
		result << " (\"" << m_token_string << "\")";
	return result.str();
}

Token xpath_imp::get_next_token()
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
				{
//					token = xp_DoubleSlash;
					push_state(kDoubleSlash);
					token = xp_Slash;
				}
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
					else
						token = xp_Name;
				}
			
			case xps_QName:
				if (ch != ':' and is_name_start_char(ch))
					state = xps_QName2;
				else
				{
					retract();	// ch
					retract();	// ':'
					if (variable)
						token = xp_Variable;
					else
						token = xp_Name;
				}
				break;
			
			case xps_QName2:
				if (ch == ':' or not is_name_char(ch))
				{
					retract();
					if (variable)
						token = xp_Variable;
					else
						token = xp_Name;
				}
				break;
		}
	}

	if (token == xp_Name)		// we've scanned a name, but it might as well be a function, nodetype or axis
	{
		// look forward and see what's ahead
		
		for (string::const_iterator c = m_next; c != m_end; ++c)
		{
			if (*c == ' ' or *c == '\t' or *c == '\n' or *c == '\r')
				continue;
			
			if (*c == ':' and *(c + 1) == ':')		// it must be an axis specifier
			{
				token = xp_AxisName;
				
				const char** a = find(kAxisNames, kAxisNames + ax_Count, m_token_string);
				if (*a != nil)
					m_token_axis = AxisType(a - kAxisNames);
				else
					throw exception("invalid axis specification %s", m_token_string.c_str());
			}
			else if (*c == '(')
			{
				if (m_token_string == "comment" or m_token_string == "text" or
					m_token_string == "processing-instruction" or m_token_string == "node")
				{
					token = xp_NodeType;
				}
				else
				{
					token = xp_FunctionName;

					const char** a = find(kCoreFunctionNames, kCoreFunctionNames + kCoreFunctionCount, m_token_string);
					if (a != kCoreFunctionNames + kCoreFunctionCount)
						m_token_function = (a - kCoreFunctionNames);
					else
						throw exception("invalid function %s", m_token_string.c_str());
				}
			}
			
			break;
		}
	}

	if (VERBOSE)
		cout << "get_next_token: " << describe_token(token) << endl;
	
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

expression_ptr xpath_imp::location_path()
{
	bool absolute = false;
	if (m_lookahead == xp_Slash)
	{
		absolute = true;
		match(xp_Slash);
	}
	
	expression_ptr result(relative_location_path());
	
	if (absolute)
		result.reset(new path_expression(expression_ptr(new document_expression()), result));

	return result;
}

expression_ptr xpath_imp::relative_location_path()
{
	expression_ptr result(step());
	
	while (m_lookahead == xp_Slash)
	{
		match(xp_Slash);
		result.reset(new path_expression(result, step()));
	}
	
	return result;
}

expression_ptr xpath_imp::step()
{
	expression_ptr result;
	
	// abbreviated steps
	if (m_lookahead == xp_Dot)
		;
	else if (m_lookahead == xp_DoubleDot)
		;
	else
	{
		AxisType axis = axis_specifier();
		result = node_test(axis);
		step_expression* step = static_cast<step_expression*>(result.get());
		
		while (m_lookahead == xp_LeftBracket)
		{
			match(xp_LeftBracket);
			step->add_predicate(expr());
			match(xp_RightBracket);
		}
	}
	
	return result;
}

AxisType xpath_imp::axis_specifier()
{
	AxisType result = ax_Child;
	
	if (m_lookahead == xp_At)
	{
		result = ax_Attribute;
		match(xp_At);
	}
	else if (m_lookahead == xp_AxisName)
	{
		result = m_token_axis;
		match(xp_AxisName);
		match(xp_DoubleColon);
	}
	
	return result;
}

expression_ptr xpath_imp::node_test(AxisType axis)
{
	expression_ptr result;
	
	if (m_lookahead == xp_Asterisk)
	{
		result.reset(new name_test_step_expression(axis, m_token_string));
		match(xp_Asterisk);
	}
	else if (m_lookahead == xp_NodeType)
	{
		// see if the name is followed by a parenthesis, if so, it must be a nodetype function
		string name = m_token_string;
		match(xp_NodeType);
		
		if (name == "comment")
			result.reset(new node_type_expression<comment>(axis));
		else if (name == "text")
			result.reset(new node_type_expression<text>(axis));
		else if (name == "processing-instruction")
			result.reset(new node_type_expression<processing_instruction>(axis));
		else if (name == "node")
			result.reset(new node_type_expression<node>(axis));
		else
			throw exception("invalid node type specified: %s", name.c_str());
	}
	else
	{
		result.reset(new name_test_step_expression(axis, m_token_string));
		match(xp_Name);
	}
}

expression_ptr xpath_imp::expr()
{
	expression_ptr result(and_expr());

	while (m_lookahead == xp_OperatorOr)
	{
		match(xp_OperatorOr);
		result.reset(new or_expression(result, and_expr()));
	}
	
	return result;
}

expression_ptr xpath_imp::primary_expr()
{
	expression_ptr result;
	
	switch (m_lookahead)
	{
		case xp_Variable:
			result.reset(new variable_expression(m_token_string));
			match(xp_Variable);
			break;
		
		case xp_LeftParenthesis:
			match(xp_LeftParenthesis);
			result = expr();
			match(xp_RightParenthesis);
			break;
			
		case xp_Literal:
			result.reset(new literal_expression(m_token_string));
			match(xp_Literal);
			break;
		
		case xp_Number:
			result.reset(new number_expression(m_token_number));
			match(xp_Number);
			break;
		
		case xp_FunctionName:
			result = function_call();
			break;
		
		default:
			throw exception("invalid primary expression in xpath");
	}
	
	return result;
}

expression_ptr xpath_imp::function_call()
{
	match(xp_FunctionName);
	match(xp_LeftParenthesis);
	
	expression_list arguments;
	
	if (m_lookahead != xp_RightParenthesis)
	{
		for (;;)
		{
			arguments.push_back(expr());
			if (m_lookahead == xp_Comma)
				match(xp_Comma);
			else
				break;
		}
	}
	match(xp_RightParenthesis);
	
	return expression_ptr(new core_function_expression(m_token_function, arguments));
}

expression_ptr xpath_imp::union_expr()
{
	expression_ptr result(path_expr());
	
	while (m_lookahead == xp_OperatorUnion)
	{
		match(m_lookahead);
		result.reset(new union_expression(result, path_expr()));
	}
	
	return result;
}

expression_ptr xpath_imp::path_expr()
{
	expression_ptr result;
	
	if (m_lookahead == xp_Variable or m_lookahead == xp_LeftParenthesis or
		m_lookahead == xp_Literal or m_lookahead == xp_Number or m_lookahead == xp_FunctionName)
	{
		result = filter_expr();
		
		if (m_lookahead == xp_Slash)
		{
			match(xp_Slash);
			result.reset(new path_expression(result, relative_location_path()));
		}
	}
	else
		result = location_path();
	
	return result;
}

expression_ptr xpath_imp::filter_expr()
{
	expression_ptr result(primary_expr());

	while (m_lookahead == xp_LeftBracket)
	{
		match(xp_LeftBracket);
		result.reset(new path_expression(result, expr()));
		match(xp_RightBracket);
	}

	return result;
}

expression_ptr xpath_imp::and_expr()
{
	expression_ptr result(equality_expr());
	
	while (m_lookahead == xp_OperatorAnd)
	{
		match(xp_OperatorAnd);
		result.reset(new operator_expression(result, relational_expr(), xp_OperatorAnd));
	}

	return result;
}

void xpath_imp::equality_expr()
{
	expression_ptr result(relational_expr());

	while (m_lookahead == xp_OperatorEqual or m_lookahead == xp_OperatorNotEqual)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		result.reset(new operator_expression(result, relational_expr(), op));
	}
	
	return result;
}

void xpath_imp::relational_expr()
{
	expression_ptr result(additive_expr());

	while (m_lookahead == xp_OperatorLess or m_lookahead == xp_OperatorLessOrEqual or
		   m_lookahead == xp_OperatorGreater or m_lookahead == xp_OperatorGreaterOrEqual)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		result.reset(new operator_expression(result, additive_expr(), op));
	}
	
	return result;
}

void xpath_imp::additive_expr()
{
	expression_ptr result(multiplicative_expr());
	
	while (m_lookahead == xp_OperatorAdd or m_lookahead == xp_OperatorSubstract)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		result.reset(new operator_expression(result, multiplicative_expr(), op));
	}
	
	return result;
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

node_set xpath_imp::evaluate(node& root)
{
	node_set result;
	result.push_back(&root);
	
	foreach (expression* expr, m_steps)
		result = expr->evaluate(result);
	
	return result;
}

// --------------------------------------------------------------------

xpath::xpath(const string& path)
	: m_impl(new xpath_imp(path))
{
	m_impl->parse(path);
}

xpath::~xpath()
{
	delete m_impl;
}

node_set xpath::evaluate(node& root)
{
	return m_impl->evaluate(root);
}

}
}
