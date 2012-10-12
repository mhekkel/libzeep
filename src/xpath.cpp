//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <iostream>
#include <sstream>
#include <numeric>
#include <stack>
#include <cmath>
#include <map>

#include <boost/tr1/cmath.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <zeep/exception.hpp>
#include <zeep/xml/node.hpp>
#include <zeep/xml/xpath.hpp>
#include <zeep/xml/unicode_support.hpp>

#include <zeep/xml/writer.hpp>

//extern int VERBOSE;

using namespace std;
using namespace tr1;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

// debug code
ostream& operator<<(ostream& lhs, const node* rhs)
{
	lhs << *rhs;
	return lhs;
}

// --------------------------------------------------------------------

enum Token {
	xp_Undef				= 0,
	xp_EOF					= 256,
	xp_LeftParenthesis,
	xp_RightParenthesis,
	xp_LeftBracket,
	xp_RightBracket,
	xp_Slash,
	xp_DoubleSlash,
	xp_Comma,
	xp_Name,
	
	xp_AxisSpec,
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

	// next four operators are pseudo tokens, i.e. they are returned as xp_Name from get_next_token
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

enum AxisType
{
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
	
	ax_AxisTypeCount
};

const char* kAxisNames[ax_AxisTypeCount] =
{
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
	"self"
};

enum CoreFunction
{
	cf_Last,
	cf_Position,
	cf_Count,
	cf_Id,
	cf_LocalName,
	cf_NamespaceUri,
	cf_Name,
	cf_String,
	cf_Concat,
	cf_StartsWith,
	cf_Contains,
	cf_SubstringBefore,
	cf_SubstringAfter,
	cf_Substring,
	cf_StringLength,
	cf_NormalizeSpace,
	cf_Translate,
	cf_Boolean,
	cf_Not,
	cf_True,
	cf_False,
	cf_Lang,
	cf_Number,
	cf_Sum,
	cf_Floor,
	cf_Ceiling,
	cf_Round,
	cf_Comment,
	
	cf_CoreFunctionCount
};

struct CoreFunctionInfo
{
	const char*		name;
	int				arg_count;
};

const int kOptionalArgument = -100;

const CoreFunctionInfo kCoreFunctionInfo[cf_CoreFunctionCount] =
{
	{ "last",				0 },
	{ "position",			0 },
	{ "count",				1 },
	{ "id",					0 },
	{ "local-name",			kOptionalArgument },
	{ "namespace-uri",		kOptionalArgument },
	{ "name",				kOptionalArgument },
	{ "string",				kOptionalArgument },
	{ "concat",				-2 },
	{ "starts-with",		2 },
	{ "contains",			2 },
	{ "substring-before",	2 },
	{ "substring-after",	2 },
	{ "substring",			-2 },
	{ "string-length",		kOptionalArgument },
	{ "normalize-space",	kOptionalArgument },
	{ "translate",			3 },
	{ "boolean",			1 },
	{ "not",				1 },
	{ "true"	,			0 },
	{ "false",				0 },
	{ "lang",				0 },
	{ "number",				kOptionalArgument },
	{ "sum",				0 },
	{ "floor",				1 },
	{ "ceiling",			1 },
	{ "round",				1 },
	{ "comment",			1 },
};

// the expressions are implemented as interpreter objects
// they return 'objects' that can hold various data.

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

	bool				operator==(const object o);
	bool				operator<(const object o);

	object_type			type() const					{ return m_type; }

	template<typename T>
	const T				as() const;
	
  private:
	object_type			m_type;
	node_set			m_node_set;
	bool				m_boolean;
	double				m_number;
	string				m_string;
};

object operator%(const object& lhs, const object& rhs);
object operator/(const object& lhs, const object& rhs);
object operator+(const object& lhs, const object& rhs);
object operator-(const object& lhs, const object& rhs);

object::object()
	: m_type(ot_undef)
{
}

object::object(node_set ns)
	: m_type(ot_node_set)
	, m_node_set(ns)
{
}

object::object(bool b)
	: m_type(ot_boolean)
	, m_boolean(b)
{
}

object::object(double n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(const string& s)
	: m_type(ot_string)
	, m_string(s)
{
}

object::object(const object& o)
	: m_type(o.m_type)
{
	switch (m_type)
	{
		case ot_node_set:	m_node_set = o.m_node_set; break;
		case ot_boolean:	m_boolean = o.m_boolean; break;
		case ot_number:		m_number = o.m_number; break;
		case ot_string:		m_string = o.m_string; break;
		default: 			break;
	}
}

object& object::operator=(const object& o)
{
	m_type = o.m_type;
	switch (m_type)
	{
		case ot_node_set:	m_node_set = o.m_node_set; break;
		case ot_boolean:	m_boolean = o.m_boolean; break;
		case ot_number:		m_number = o.m_number; break;
		case ot_string:		m_string = o.m_string; break;
		default: 			break;
	}
	return *this;
}

template<>
const node_set& object::as<const node_set&>() const
{
	if (m_type != ot_node_set)
		throw exception("object is not of type node-set");
	return m_node_set;
}

template<>
const bool object::as<bool>() const
{
	bool result;
	switch (m_type)
	{
		case ot_number:		result = m_number != 0 and not tr1::isnan(m_number); break;
		case ot_node_set:	result = not m_node_set.empty(); break;
		case ot_string:		result = not m_string.empty(); break;
		case ot_boolean:	result = m_boolean; break;
		default:			result = false; break;
	}
	
	return result;
}

template<>
const double object::as<double>() const
{
	double result;
	switch (m_type)
	{
		case ot_number:		result = m_number; break;
		case ot_node_set:	result = boost::lexical_cast<double>(m_node_set.front()->str()); break;
		case ot_string:		result = boost::lexical_cast<double>(m_string); break;
		case ot_boolean:	result = m_boolean; break;
		default:			result = 0; break;
	}
	return result;
}

template<>
const int object::as<int>() const
{
	if (m_type != ot_number)
		throw exception("object is not of type number");
	return static_cast<int>(tr1::round(m_number));
}

template<>
const string& object::as<const string&>() const
{
	if (m_type != ot_string)
		throw exception("object is not of type string");
	return m_string;
}

template<>
const string object::as<string>() const
{
	string result;
	
	switch (m_type)
	{
		case ot_number:		result = boost::lexical_cast<string>(m_number); break;
		case ot_string:		result = m_string; break;
		case ot_boolean:	result = (m_boolean ? "true" : "false"); break;
		case ot_node_set:
			if (not m_node_set.empty())
				result = m_node_set.front()->str();
			break;
		default:			break;
	}

	return result;
}

bool object::operator==(const object o)
{
	bool result = false;
	
	if (m_type == o.m_type)
	{
		switch (m_type)
		{
			case ot_node_set:	result = m_node_set == o.m_node_set; break;
			case ot_boolean:	result = m_boolean == o.m_boolean; break;
			case ot_number:		result = m_number == o.m_number; break;
			case ot_string:		result = m_string == o.m_string; break;
			default: 			break;
		}
	}
	else
	{
		if (m_type == ot_number or o.m_type == ot_number)
			result = as<double>() == o.as<double>();
		else if (m_type == ot_string or o.m_type == ot_string)
			result = as<string>() == o.as<string>();
		else if (m_type == ot_boolean or o.m_type == ot_boolean)
			result = as<bool>() == o.as<bool>();
	}
	
	return result;
}

bool object::operator<(const object o)
{
	bool result = false;
	switch (m_type)
	{
		case ot_node_set:	result = m_node_set < o.m_node_set; break;
		case ot_boolean:	result = m_boolean < o.m_boolean; break;
		case ot_number:		result = m_number < o.m_number; break;
		case ot_string:		result = m_string < o.m_string; break;
		default: 			break;
	}
	return result;
}

ostream& operator<<(ostream& lhs, object& rhs)
{
	switch (rhs.type())
	{
		case ot_undef:		lhs << "undef()";						break;
		case ot_number:		lhs << "number(" << rhs.as<double>() << ')';	break;
		case ot_string:		lhs << "string(" << rhs.as<string>() << ')'; break;
		case ot_boolean:	lhs << "boolean(" << (rhs.as<bool>() ? "true" : "false") << ')'; break;
		case ot_node_set:	lhs << "node_set(#" << rhs.as<const node_set&>().size() << ')'; break;
	}
	
	return lhs;
}

// --------------------------------------------------------------------
// visiting (or better, collecting) other nodes in the hierarchy is done here.

template<typename PREDICATE>
void iterate_children(container* context, node_set& s, bool deep, PREDICATE pred)
{
	for (node* child = context->child(); child != nullptr; child = child->next())
	{
		container* e = dynamic_cast<container*>(child);
		if (e == nullptr)
			continue;

		if (find(s.begin(), s.end(), child) != s.end())
			continue;

		if (pred(e))
			s.push_back(child);

		if (deep)
			iterate_children(e, s, true, pred);
	}
}

template<typename PREDICATE>
void iterate_ancestor(container* e, node_set& s, PREDICATE pred)
{
	for (;;)
	{
		e = dynamic_cast<container*>(e->parent());
		
		if (e == nullptr)
			break;
		
		root_node* r = dynamic_cast<root_node*>(e);
		if (r != nullptr)
			break;

		if (pred(e))
			s.push_back(e);
	}
}

template<typename PREDICATE>
void iterate_preceding(node* n, node_set& s, bool sibling, PREDICATE pred)
{
	while (n != nullptr)
	{
		if (n->prev() == nullptr)
		{
			if (sibling)
				break;
			
			n = n->parent();
			continue;
		}
		
		n = n->prev();
		
		container* e = dynamic_cast<container*>(n);
		if (e == nullptr)
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
	while (n != nullptr)
	{
		if (n->next() == nullptr)
		{
			if (sibling)
				break;
			
			n = n->parent();
			continue;
		}
		
		n = n->next();
		
		container* e = dynamic_cast<container*>(n);
		if (e == nullptr)
			continue;
		
		if (pred(e))
			s.push_back(e);
		
		if (sibling == false)
			iterate_children(e, s, true, pred);
	}
}

template<typename PREDICATE>
void iterate_attributes(element* e, node_set& s, PREDICATE pred)
{
	foreach (attribute* a, e->attributes())
	{
		if (pred(a))
			s.push_back(a);
	}
}

template<typename PREDICATE>
void iterate_namespaces(element* e, node_set& s, PREDICATE pred)
{
	foreach (name_space* a, e->name_spaces())
	{
		if (pred(a))
			s.push_back(a);
	}
}

// --------------------------------------------------------------------
// context for the expressions
// Need to add support for external variables here.

struct context_imp
{
	virtual				~context_imp() {}
	
	virtual object&		get(const string& name)
						{
							return m_variables[name];
						}
						
	virtual void		set(const string& name, const object& value)
						{
							m_variables[name] = value;
						}
						
	map<string,object>	m_variables;
};

struct expression_context : public context_imp
{
						expression_context(context_imp& next, node* n, const node_set& s)
							: m_next(next), m_node(n), m_node_set(s) {}

	virtual object&		get(const string& name)
						{
							return m_next.get(name);
						}
						
	virtual void		set(const string& name, const object& value)
						{
							m_next.set(name, value);
						}

	void				dump();
	
	size_t				position() const;
	size_t				last() const;
	
	context_imp&		m_next;
	node*				m_node;
	const node_set&		m_node_set;
};

size_t expression_context::position() const
{
	size_t result = 0;
	foreach (const node* n, m_node_set)
	{
		++result;
		if (n == m_node)
			break;
	}

	if (result == 0)
		throw exception("invalid context for position");

	return result;
}

size_t expression_context::last() const
{
	return m_node_set.size();
}

void expression_context::dump()
{
	cout << "context node: " << *m_node << endl
		 << "context node-set: ";
	copy(m_node_set.begin(), m_node_set.end(), ostream_iterator<node*>(cout, ", "));
	cout << endl;
}

ostream& operator<<(ostream& lhs, expression_context& rhs)
{
	rhs.dump();
	return lhs;
}

void indent(int level)
{
	while (level-- > 0) cout << ' ';
}

// --------------------------------------------------------------------

class expression
{
  public:
	virtual				~expression() {}
	virtual object		evaluate(expression_context& context) = 0;

						// print exists only for debugging purposes
	virtual void		print(int level) = 0;
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
	object				evaluate(expression_context& context, T pred);

	AxisType			m_axis;
};

template<typename T>
object step_expression::evaluate(expression_context& context, T pred)
{
	node_set result;
	
	container* context_element = dynamic_cast<container*>(context.m_node);
	if (context_element != nullptr)
	{
		switch (m_axis)
		{
			case ax_Parent:
				if (context_element->parent() != nullptr)
				{
					container* e = static_cast<container*>(context_element->parent());
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
	
			case ax_Attribute:
				if (dynamic_cast<element*>(context_element) != nullptr)
					iterate_attributes(static_cast<element*>(context_element), result, pred);
				break;

			case ax_Namespace:
				if (dynamic_cast<element*>(context_element) != nullptr)
					iterate_namespaces(static_cast<element*>(context_element), result, pred);
				break;
				
			case ax_AxisTypeCount:
				;
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
							m_test = boost::bind(&name_test_step_expression::name_matches, this, _1);
						}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level) { indent(level); cout << "name test step " << m_name << endl; }

  protected:

	bool				name_matches(const node* n)
						{
							bool result = m_name == "*";
							if (result == false)
							{
								const element* e = dynamic_cast<const element*>(n);
								if (e != nullptr and e->name() == m_name)
									result = true;
							}

							if (result == false)
							{
								const attribute* a = dynamic_cast<const attribute*>(n);
								if (a != nullptr and a->name() == m_name)
									result = true;
							}
							
							return result;
						}

	string									m_name;
	boost::function<bool(const node*)>		m_test;
};

object name_test_step_expression::evaluate(expression_context& context)
{
	return step_expression::evaluate(context, m_test);
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

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level) { indent(level); cout << "node type step " << typeid(T).name() << endl; }

  private:
	static bool			test(const node* n)					{ return dynamic_cast<const T*>(n) != nullptr; }

	boost::function<bool(const node*)>		m_test;
};

template<typename T>
object node_type_expression<T>::evaluate(expression_context& context)
{
	return step_expression::evaluate(context, m_test);
}

// --------------------------------------------------------------------

class root_expression : public expression
{
  public:
	virtual object		evaluate(expression_context& context);

	virtual void		print(int level) { indent(level); cout << "root" << endl; }
};

object root_expression::evaluate(expression_context& context)
{
	node_set result;
	result.push_back(context.m_node->root());
	return result;
}

// --------------------------------------------------------------------

template<Token OP>
class operator_expression : public expression
{
  public:
						operator_expression(expression_ptr lhs, expression_ptr rhs)
							: m_lhs(lhs), m_rhs(rhs) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level)
						{
							indent(level);
							cout << "operator " << typeid(OP).name() << endl;
							m_lhs->print(level + 1);
							m_rhs->print(level + 1);
						}

  private:
	expression_ptr		m_lhs, m_rhs;
};

template<>
object operator_expression<xp_OperatorAdd>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1.as<double>() + v2.as<double>();
}

template<>
object operator_expression<xp_OperatorSubstract>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1.as<double>() - v2.as<double>();
}

template<>
object operator_expression<xp_OperatorEqual>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1 == v2;
}

template<>
object operator_expression<xp_OperatorNotEqual>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return not (v1 == v2);
}

template<>
object operator_expression<xp_OperatorLess>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1 < v2;
}

template<>
object operator_expression<xp_OperatorLessOrEqual>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1 < v2 or v1 == v2;
}

template<>
object operator_expression<xp_OperatorGreater>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v2 < v1;
}

template<>
object operator_expression<xp_OperatorGreaterOrEqual>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v2 < v1 or v1 == v2;
}

template<>
object operator_expression<xp_OperatorAnd>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1.as<bool>() and v2.as<bool>();
}

template<>
object operator_expression<xp_OperatorOr>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1.as<bool>() or v2.as<bool>();
}

template<>
object operator_expression<xp_OperatorMod>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return double(v1.as<int>() % v2.as<int>());
}

template<>
object operator_expression<xp_OperatorDiv>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1.as<double>() / v2.as<double>();
}

template<>
object operator_expression<xp_Asterisk>::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	return v1.as<double>() * v2.as<double>();
}

// --------------------------------------------------------------------

class negate_expression : public expression
{
  public:
						negate_expression(expression_ptr expr)
							: m_expr(expr) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level) { indent(level); cout << "negate" << endl; m_expr->print(level + 1); }

  private:
	expression_ptr		m_expr;
};

object negate_expression::evaluate(expression_context& context)
{
	object v = m_expr->evaluate(context);
	return -v.as<double>();
}

// --------------------------------------------------------------------

class path_expression : public expression
{
  public:
						path_expression(expression_ptr lhs, expression_ptr rhs)
							: m_lhs(lhs), m_rhs(rhs) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level)
						{
							indent(level);
							cout << "path" << endl;
							m_lhs->print(level + 1);
							m_rhs->print(level + 1);
						}

  private:
	expression_ptr		m_lhs, m_rhs;
};

object path_expression::evaluate(expression_context& context)
{
	object v = m_lhs->evaluate(context);
	if (v.type() != ot_node_set)
		throw exception("filter does not evaluate to a node-set");
	
	node_set result;
	foreach (node* n, v.as<const node_set&>())
	{
		expression_context ctxt(context, n, v.as<const node_set&>());
		
		node_set s = m_rhs->evaluate(ctxt).as<const node_set&>();

		copy(s.begin(), s.end(), back_inserter(result));
	}
	
	return result;
}

// --------------------------------------------------------------------

class predicate_expression : public expression
{
  public:
						predicate_expression(expression_ptr path, expression_ptr pred)
							: m_path(path), m_pred(pred) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level)
						{
							indent(level);
							cout << "predicate" << endl;
							m_path->print(level + 1);
							m_pred->print(level + 1);
						}

  private:
	expression_ptr		m_path, m_pred;
};

object predicate_expression::evaluate(expression_context& context)
{
	object v = m_path->evaluate(context);
	
	node_set result;
	
	foreach (node* n, v.as<const node_set&>())
	{
		expression_context ctxt(context, n, v.as<const node_set&>());
		
		object test = m_pred->evaluate(ctxt);

		if (test.type() == ot_number)
		{
			if (ctxt.position() == test.as<double>())
				result.push_back(n);
		}
		else if (test.as<bool>())
			result.push_back(n);
	}
	
	return result;
}

// --------------------------------------------------------------------

class variable_expression : public expression
{
  public:
						variable_expression(const string& name)
							: m_var(name) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level) { indent(level); cout << "variable " << m_var << endl; }

  private:
	string				m_var;
};

object variable_expression::evaluate(expression_context& context)
{
	return context.get(m_var);
}

// --------------------------------------------------------------------

class literal_expression : public expression
{
  public:
						literal_expression(const string& lit)
							: m_lit(lit) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level) { indent(level); cout << "literal " << m_lit << endl; }

  private:
	string				m_lit;
};

object literal_expression::evaluate(expression_context& context)
{
	return object(m_lit);
}

// --------------------------------------------------------------------

class number_expression : public expression
{
  public:
						number_expression(double number)
							: m_number(number) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level) { indent(level); cout << "number " << m_number << endl; }

  private:
	double				m_number;
};

object number_expression::evaluate(expression_context& context)
{
	return object(m_number);
}

// --------------------------------------------------------------------

template<CoreFunction CF>
class core_function_expression : public expression
{
  public:
						core_function_expression(expression_list& arguments)
							: m_args(arguments) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level)
						{
							indent(level);
							cout << "function call " << typeid(CF).name() << endl;
							for_each(m_args.begin(), m_args.end(),
								boost::bind(&expression::print, _1, level + 1));
						}

  private:
	expression_list		m_args;
};

template<CoreFunction CF>
object core_function_expression<CF>::evaluate(expression_context& context)
{
	throw exception("unimplemented function ");
	return object();
}

template<>
object core_function_expression<cf_Position>::evaluate(expression_context& context)
{
	return object(double(context.position()));
}

template<>
object core_function_expression<cf_Last>::evaluate(expression_context& context)
{
	return object(double(context.last()));
}

template<>
object core_function_expression<cf_Count>::evaluate(expression_context& context)
{
	object v = m_args.front()->evaluate(context);
	size_t result = v.as<const node_set&>().size();

	return object(double(result));
}

template<>
object core_function_expression<cf_Id>::evaluate(expression_context& context)
{
	element* e = nullptr;
	
	if (m_args.empty())
		e = dynamic_cast<element*>(context.m_node);
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set&>().empty())
			e = dynamic_cast<element*>(v.as<const node_set&>().front());
	}
		
	if (e == nullptr)
		throw exception("argument is not an element in function 'id()'");

	return e->id();
}

template<>
object core_function_expression<cf_LocalName>::evaluate(expression_context& context)
{
	node* n = nullptr;
	
	if (m_args.empty())
		n = context.m_node;
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set&>().empty())
			n = v.as<const node_set&>().front();
	}
		
	if (n == nullptr)
		throw exception("argument is not an element in function 'local-name'");

	return n->name();
}

template<>
object core_function_expression<cf_NamespaceUri>::evaluate(expression_context& context)
{
	node* n = nullptr;
	
	if (m_args.empty())
		n = context.m_node;
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set&>().empty())
			n = v.as<const node_set&>().front();
	}
		
	if (n == nullptr)
		throw exception("argument is not an element in function 'namespace-uri'");

	return n->ns();
}

template<>
object core_function_expression<cf_Name>::evaluate(expression_context& context)
{
	node* n = nullptr;
	
	if (m_args.empty())
		n = context.m_node;
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set&>().empty())
			n = v.as<const node_set&>().front();
	}
		
	if (n == nullptr)
		throw exception("argument is not an element in function 'name'");

	return n->qname();
}

template<>
object core_function_expression<cf_String>::evaluate(expression_context& context)
{
	string result;
	
	if (m_args.empty())
		result = context.m_node->str();
	else
	{
		object v = m_args.front()->evaluate(context);
		result = v.as<string>();
	}
		
	return result;
}

template<>
object core_function_expression<cf_Concat>::evaluate(expression_context& context)
{
	string result;
	foreach (expression_ptr& e, m_args)
	{
		object v = e->evaluate(context);
		result += v.as<string>();
	}
	return result;
}

template<>
object core_function_expression<cf_StringLength>::evaluate(expression_context& context)
{
	string result;
	
	if (m_args.empty())
		result = context.m_node->str();
	else
	{
		object v = m_args.front()->evaluate(context);
		result = v.as<string>();
	}

	return double(result.length());
}

template<>
object core_function_expression<cf_StartsWith>::evaluate(expression_context& context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);
	
	if (v1.type() != ot_string or v2.type() != ot_string)
		throw exception("expected two strings as argument for starts-with");
	
	return v2.as<string>().empty() or
		ba::starts_with(v1.as<string>(), v2.as<string>());
}

template<>
object core_function_expression<cf_Contains>::evaluate(expression_context& context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);
	
	if (v1.type() != ot_string or v2.type() != ot_string)
		throw exception("expected two strings as argument for contains");
	
	return v2.as<string>().empty() or
		v1.as<string>().find(v2.as<string>()) != string::npos;
}

template<>
object core_function_expression<cf_SubstringBefore>::evaluate(expression_context& context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);
	
	if (v1.type() != ot_string or v2.type() != ot_string)
		throw exception("expected two strings as argument for substring-before");
	
	string result;
	if (not v2.as<string>().empty())
	{
		string::size_type p = v1.as<string>().find(v2.as<string>());
		if (p != string::npos)
			result = v1.as<string>().substr(0, p);
	}
	
	return result;
}

template<>
object core_function_expression<cf_SubstringAfter>::evaluate(expression_context& context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);
	
	if (v1.type() != ot_string or v2.type() != ot_string)
		throw exception("expected two strings as argument for substring-after");
	
	string result;
	if (v2.as<string>().empty())
		result = v1.as<string>();
	else
	{
		string::size_type p = v1.as<string>().find(v2.as<string>());
		if (p != string::npos and p + v2.as<string>().length() < v1.as<string>().length())
			result = v1.as<string>().substr(p + v2.as<string>().length());
	}
	
	return result;
}

template<>
object core_function_expression<cf_Substring>::evaluate(expression_context& context)
{
	expression_list::iterator a = m_args.begin();
	
	object v1 = (*a)->evaluate(context);	++a;
	object v2 = (*a)->evaluate(context);	++a;
	object v3 = (*a)->evaluate(context);
	
	if (v1.type() != ot_string or v2.type() != ot_number or v3.type() != ot_number)
		throw exception("expected one string and two numbers as argument for substring");
	
	return v1.as<string>().substr(v2.as<int>() - 1, v3.as<int>());
}

template<>
object core_function_expression<cf_NormalizeSpace>::evaluate(expression_context& context)
{
	string s;
	
	if (m_args.empty())
		s = context.m_node->str();
	else
	{
		object v = m_args.front()->evaluate(context);
		s = v.as<string>();
	}
	
	string result;
	bool space = true;
	
	foreach (char c, s)
	{
		if (isspace(c))
		{
			if (not space)
				result += ' ';
			space = true;
		}
		else
		{
			result += c;
			space = false;
		}
	}
	
	if (not result.empty() and space)
		result.erase(result.end() - 1);
	
	return result;
}

template<>
object core_function_expression<cf_Translate>::evaluate(expression_context& context)
{
	expression_list::iterator a = m_args.begin();
	
	object v1 = (*a)->evaluate(context);	++a;
	object v2 = (*a)->evaluate(context);	++a;
	object v3 = (*a)->evaluate(context);
	
	if (v1.type() != ot_string or v2.type() != ot_string or v3.type() != ot_string)
		throw exception("expected three strings as arguments for translate");
	
	const string& f = v2.as<const string&>();
	const string& r = v3.as<const string&>();
	
	string result;
	result.reserve(v1.as<string>().length());
	foreach (char c, v1.as<string>())
	{
		string::size_type fi = f.find(c);
		if (fi == string::npos)
			result += c;
		else if (fi < r.length())
			result += r[fi];
	}

	return result;
}

template<>
object core_function_expression<cf_Boolean>::evaluate(expression_context& context)
{
	object v = m_args.front()->evaluate(context);
	return v.as<bool>();
}

template<>
object core_function_expression<cf_Not>::evaluate(expression_context& context)
{
	object v = m_args.front()->evaluate(context);
	return not v.as<bool>();
}

template<>
object core_function_expression<cf_True>::evaluate(expression_context& context)
{
	return true;
}

template<>
object core_function_expression<cf_False>::evaluate(expression_context& context)
{
	return false;
}

template<>
object core_function_expression<cf_Lang>::evaluate(expression_context& context)
{
	object v = m_args.front()->evaluate(context);
	
	string test = v.as<string>();
	ba::to_lower(test);
	
	string lang = context.m_node->lang();
	ba::to_lower(lang);
	
	bool result = test == lang;
	
	string::size_type s;
	
	if (result == false and (s = lang.find('-')) != string::npos)
		result = test == lang.substr(0, s);
	
	return result;
}

template<>
object core_function_expression<cf_Number>::evaluate(expression_context& context)
{
	object v;
	
	if (m_args.size() == 1)
		v = m_args.front()->evaluate(context);
	else
		v = boost::lexical_cast<double>(context.m_node->str());

	return v.as<double>();
}

template<>
object core_function_expression<cf_Floor>::evaluate(expression_context& context)
{
	object v = m_args.front()->evaluate(context);
	return floor(v.as<double>());
}

template<>
object core_function_expression<cf_Ceiling>::evaluate(expression_context& context)
{
	object v = m_args.front()->evaluate(context);
	return ceil(v.as<double>());
}

template<>
object core_function_expression<cf_Round>::evaluate(expression_context& context)
{
	object v = m_args.front()->evaluate(context);
	return tr1::round(v.as<double>());
}

// --------------------------------------------------------------------

class union_expression : public expression
{
  public:
						union_expression(expression_ptr lhs, expression_ptr rhs)
							: m_lhs(lhs), m_rhs(rhs) {}

	virtual object		evaluate(expression_context& context);

	virtual void		print(int level)
						{
							indent(level);
							cout << "union" << endl;
							m_lhs->print(level + 1);
							m_rhs->print(level + 1);
						}

  private:
	expression_ptr		m_lhs, m_rhs;
};

object union_expression::evaluate(expression_context& context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);
	
	if (v1.type() != ot_node_set or v2.type() != ot_node_set)
		throw exception("union operator works only on node sets");
	
	node_set s1 = v1.as<const node_set&>();
	node_set s2 = v2.as<const node_set&>();
	
	copy(s2.begin(), s2.end(), back_inserter(s1));
	
	return s1;
}

// --------------------------------------------------------------------

struct xpath_imp
{
						xpath_imp();

	void				reference();
	void				release();
	
	node_set			evaluate(node& root, context_imp& context);

	void				parse(const string& path);
	
	void				preprocess(const string& path);
	
	unsigned char		next_byte();
	unicode				get_next_char();
	void				retract();
	Token				get_next_token();
	string				describe_token(Token token);
	void				match(Token token);
	
	expression_ptr		location_path();
	expression_ptr		absolute_location_path();
	expression_ptr		relative_location_path();
	expression_ptr		step();
	expression_ptr		node_test(AxisType axis);
	
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

	// abbreviated steps are expanded like macros by the scanner
	string				m_path;
	string::const_iterator
						m_begin, m_next, m_end;
	Token				m_lookahead;
	string				m_token_string;
	double				m_token_number;
	AxisType			m_token_axis;
	CoreFunction		m_token_function;

	// the generated expression
	expression_ptr		m_expr;

  private:
						~xpath_imp();

	long				m_refcount;
};

// --------------------------------------------------------------------

xpath_imp::xpath_imp()
	: m_refcount(1)
{
}

xpath_imp::~xpath_imp()
{
}

void xpath_imp::reference()
{
	++m_refcount;
}

void xpath_imp::release()
{
	if (--m_refcount <= 0)
		delete this;
}

void xpath_imp::parse(const string& path)
{
	// start by expanding the abbreviations in the path
	preprocess(path);

	m_begin = m_next = m_path.begin();
	m_end = m_path.end();
	
	m_lookahead = get_next_token();
	m_expr = location_path();
	
	while (m_lookahead == xp_OperatorUnion)
	{
		match(xp_OperatorUnion);
		m_expr.reset(new union_expression(m_expr, location_path()));
	}

//	if (VERBOSE)
//		m_expr->print(0);

	match(xp_EOF);
}

void xpath_imp::preprocess(const string& path)
{
	// preprocessing consists of expanding abbreviations
	// replacements are:
	// @  => replaced by 'attribute::'
	// // => replaced by '/descendant-or-self::node()/'
	// . (if at a step location) => 'self::node()'
	// ..  (if at a step location) => 'parent::node()'
	
	m_path.clear();
	
	enum State
	{
		pp_Step,
		pp_Data,
		pp_Dot,
		pp_Slash,
		pp_String
	} state;
	
	state = pp_Step;
	unicode quoteChar = 0;
	
	for (string::const_iterator ch = path.begin(); ch != path.end(); ++ch)
	{
		switch (state)
		{
			case pp_Step:
				state = pp_Data;
				switch (*ch)
				{
					case '@': m_path += "attribute::"; break;
					case '.': state = pp_Dot;	break;
					case '/': state = pp_Slash;	break;
					case '\'':
					case '\"':
						m_path += *ch;
						quoteChar = *ch;
						state = pp_String;
						break;
					default: m_path += *ch; break;
				}
				break;
			
			case pp_Data:
				switch (*ch)
				{
					case '@': m_path += "attribute::"; break;
					case '/': state = pp_Slash; break;
					case '[': m_path += '['; state = pp_Step; break;
					case '\'':
					case '\"':
						m_path += *ch;
						quoteChar = *ch;
						state = pp_String;
						break;
					default: m_path += *ch; break;
				}
				break;
			
			case pp_Dot:
				if (*ch == '.')
					m_path += "parent::node()";
				else
				{
					--ch;
					m_path += "self::node()";
				}
				state = pp_Step;
				break;
			
			case pp_Slash:
				if (*ch == '/')
					m_path += "/descendant-or-self::node()/";
				else
				{
					--ch;
					m_path += '/';
				}
				state = pp_Step;
				break;
			
			case pp_String:
				m_path += *ch;
				if (static_cast<unsigned char>(*ch) == quoteChar)
						state = pp_Data;
				break;
		}
	}
}

unsigned char xpath_imp::next_byte()
{
	char result = 0;
	
	if (m_next < m_end)
	{
		result = *m_next;
		++m_next;
	}

	m_token_string += result;

	return static_cast<unsigned char>(result);
}

// We assume all paths are in valid UTF-8 encoding
unicode xpath_imp::get_next_char()
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
			throw exception("Invalid utf-8");
		result = ((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F);
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = ((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = ((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F);
	}

	if (result > 0x10ffff)
		throw exception("invalid utf-8 character (out of range)");
	
	return result;
}

void xpath_imp::retract()
{
	string::iterator c = m_token_string.end();

	// skip one valid character back in the input buffer
	// since we've arrived here, we can safely assume input
	// is valid UTF-8
	do --c; while ((*c & 0x0c0) == 0x080);
	
	if (m_next != m_end or *c != 0)
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
		case xp_Slash:				result << "forward slash"; break;
		case xp_DoubleSlash:		result << "double forward slash"; break;
		case xp_Comma:				result << "comma"; break;
		case xp_Name:				result << "name"; break;
		case xp_AxisSpec:			result << "axis specification"; break;
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
		case xp_Asterisk:			result << "asterisk (or multiply)"; break;
		case xp_Colon:				result << "colon"; break;
	}

	result << " {" << m_token_string << '}';
	return result.str();
}

Token xpath_imp::get_next_token()
{
	enum State {
		xps_Start,
		xps_VariableStart,
		xps_ExclamationMark,
		xps_LessThan,
		xps_GreaterThan,
		xps_Number,
		xps_NumberFraction,
		xps_Name,
		xps_QName,
		xps_QName2,
		xps_Literal
	} start, state;
	start = state = xps_Start;

	Token token = xp_Undef;
	bool variable = false;
	double fraction = 1.0;
	unicode quoteChar;

	m_token_string.clear();
	
	while (token == xp_Undef)
	{
		unicode ch = get_next_char();
		
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
					case ',':	token = xp_Comma; break;
					case ':':	token = xp_Colon; break;
					case '$':	state = xps_VariableStart; break;
					case '*':	token = xp_Asterisk; break;
					case '/':	token = xp_Slash; break;
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
					case '\'':	quoteChar = ch; state = xps_Literal; break;
					case '"':	quoteChar = ch; state = xps_Literal; break;

					case '@':
						token = xp_AxisSpec;
						m_token_axis = ax_Attribute;
						break;

					default:
						if (ch == '.')
						{
							m_token_number = 0;
							state = xps_NumberFraction;
						}
						else if (ch >= '0' and ch <= '9')
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
				break;
			
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
			
			case xps_Literal:
				if (ch == 0)
					throw exception("run-away string, missing quote character?");
				else if (ch == quoteChar)
				{
					token = xp_Literal;
					m_token_string = m_token_string.substr(1, m_token_string.length() - 2);
				}
				break;
		}
	}

	if (token == xp_Name)		// we've scanned a name, but it might as well be a function, nodetype or axis
	{
		// look forward and see what's ahead
		for (string::const_iterator c = m_next; c != m_end; ++c)
		{
			if (isspace(*c))
				continue;
			
			if (*c == ':' and *(c + 1) == ':')		// it must be an axis specifier
			{
				token = xp_AxisSpec;
				
				const int kAxisNameCount = sizeof(kAxisNames) / sizeof(const char*);
				const char** a = find(kAxisNames, kAxisNames + kAxisNameCount, m_token_string);
				if (*a != nullptr)
					m_token_axis = AxisType(a - kAxisNames);
				else
					throw exception("invalid axis specification %s", m_token_string.c_str());

				// skip over the double colon
				m_next = c + 2;
			}
			else if (*c == '(')
			{
				if (m_token_string == "comment" or m_token_string == "text" or
					m_token_string == "processing-instruction" or m_token_string == "node")
				{
					token = xp_NodeType;
					
					// set input pointer after the parenthesis
					m_next = c + 1;
					while (m_next != m_end and isspace(*m_next))
						++m_next;
					if (*m_next != ')')
						throw exception("expected '()' after a node type specifier");
					++m_next;
				}
				else
				{
					for (int i = 0; i < cf_CoreFunctionCount; ++i)
					{
						if (m_token_string == kCoreFunctionInfo[i].name)
						{
							token = xp_FunctionName;
							m_token_function = CoreFunction(i);
							break;
						}
					}

					if (token != xp_FunctionName)
						throw exception("invalid function %s", m_token_string.c_str());
				}
			}
			
			break;
		}
	}

//	if (VERBOSE)
//		cout << "get_next_token: " << describe_token(token) << endl;
	
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
		
		if (m_lookahead != xp_EOF and m_lookahead != xp_Undef)
		{
			found += " (\"";
			found += m_token_string;
			found += "\")";
		}
		
		string expected = describe_token(token);
		
		stringstream s;
		s << "syntax error in xpath, expected " << expected << " but found " << found;
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
		result.reset(new path_expression(expression_ptr(new root_expression()), result));

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
	
	AxisType axis = ax_Child;
	if (m_lookahead == xp_AxisSpec)
	{
		axis = m_token_axis;
		match(xp_AxisSpec);
	}

	result = node_test(axis);
	
	while (m_lookahead == xp_LeftBracket)
	{
		match(xp_LeftBracket);
		result.reset(new predicate_expression(result, expr()));
		match(xp_RightBracket);
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
	
	return result;
}

expression_ptr xpath_imp::expr()
{
	expression_ptr result(and_expr());

	while (m_lookahead == xp_Name and m_token_string == "or")
	{
		match(xp_Name);
		result.reset(new operator_expression<xp_OperatorOr>(result, and_expr()));
	}
	
	return result;
}

expression_ptr xpath_imp::primary_expr()
{
	expression_ptr result;
	
	switch (m_lookahead)
	{
		case xp_Variable:
			result.reset(new variable_expression(m_token_string.substr(1)));
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
	CoreFunction function = m_token_function;
	
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
	
	expression_ptr result;
	
	int expected_arg_count = kCoreFunctionInfo[int(function)].arg_count;
	if (expected_arg_count > 0)
	{
		if (int(arguments.size()) != expected_arg_count)
			throw exception("invalid number of arguments for function %s", kCoreFunctionInfo[int(function)].name);
	}
	else if (expected_arg_count == kOptionalArgument)
	{
		if (arguments.size() > 1)
			throw exception("incorrect number of arguments for function %s", kCoreFunctionInfo[int(function)].name);
	}
	else if (expected_arg_count < 0 and int(arguments.size()) < -expected_arg_count)
		throw exception("insufficient number of arguments for function %s", kCoreFunctionInfo[int(function)].name);
	
	switch (function)
	{
		case cf_Last:			result.reset(new core_function_expression<cf_Last>(arguments)); break;
		case cf_Position:		result.reset(new core_function_expression<cf_Position>(arguments)); break;
		case cf_Count:			result.reset(new core_function_expression<cf_Count>(arguments)); break;
		case cf_Id:				result.reset(new core_function_expression<cf_Id>(arguments)); break;
		case cf_LocalName:		result.reset(new core_function_expression<cf_LocalName>(arguments)); break;
		case cf_NamespaceUri:	result.reset(new core_function_expression<cf_NamespaceUri>(arguments)); break;
		case cf_Name:			result.reset(new core_function_expression<cf_Name>(arguments)); break;
		case cf_String:			result.reset(new core_function_expression<cf_String>(arguments)); break;
		case cf_Concat:			result.reset(new core_function_expression<cf_Concat>(arguments)); break;
		case cf_StartsWith:		result.reset(new core_function_expression<cf_StartsWith>(arguments)); break;
		case cf_Contains:		result.reset(new core_function_expression<cf_Contains>(arguments)); break;
		case cf_SubstringBefore:result.reset(new core_function_expression<cf_SubstringBefore>(arguments)); break;
		case cf_SubstringAfter:	result.reset(new core_function_expression<cf_SubstringAfter>(arguments)); break;
		case cf_StringLength:	result.reset(new core_function_expression<cf_StringLength>(arguments)); break;
		case cf_NormalizeSpace:	result.reset(new core_function_expression<cf_NormalizeSpace>(arguments)); break;
		case cf_Translate:		result.reset(new core_function_expression<cf_Translate>(arguments)); break;
		case cf_Boolean:		result.reset(new core_function_expression<cf_Boolean>(arguments)); break;
		case cf_Not:			result.reset(new core_function_expression<cf_Not>(arguments)); break;
		case cf_True:			result.reset(new core_function_expression<cf_True>(arguments)); break;
		case cf_False:			result.reset(new core_function_expression<cf_False>(arguments)); break;
		case cf_Lang:			result.reset(new core_function_expression<cf_Lang>(arguments)); break;
		case cf_Number:			result.reset(new core_function_expression<cf_Number>(arguments)); break;
		case cf_Sum:			result.reset(new core_function_expression<cf_Sum>(arguments)); break;
		case cf_Floor:			result.reset(new core_function_expression<cf_Floor>(arguments)); break;
		case cf_Ceiling:		result.reset(new core_function_expression<cf_Ceiling>(arguments)); break;
		case cf_Round:			result.reset(new core_function_expression<cf_Round>(arguments)); break;
		case cf_Comment:		result.reset(new core_function_expression<cf_Comment>(arguments)); break;
		default:				break;
	}
	
	return result;
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
		result.reset(new predicate_expression(result, expr()));
		match(xp_RightBracket);
	}

	return result;
}

expression_ptr xpath_imp::and_expr()
{
	expression_ptr result(equality_expr());
	
	while (m_lookahead == xp_Name and m_token_string == "and")
	{
		match(xp_Name);
		result.reset(new operator_expression<xp_OperatorAnd>(result, equality_expr()));
	}

	return result;
}

expression_ptr xpath_imp::equality_expr()
{
	expression_ptr result(relational_expr());

	while (m_lookahead == xp_OperatorEqual or m_lookahead == xp_OperatorNotEqual)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		if (op == xp_OperatorEqual)
			result.reset(new operator_expression<xp_OperatorEqual>(result, relational_expr()));
		else
			result.reset(new operator_expression<xp_OperatorNotEqual>(result, relational_expr()));
	}
	
	return result;
}

expression_ptr xpath_imp::relational_expr()
{
	expression_ptr result(additive_expr());

	while (m_lookahead == xp_OperatorLess or m_lookahead == xp_OperatorLessOrEqual or
		   m_lookahead == xp_OperatorGreater or m_lookahead == xp_OperatorGreaterOrEqual)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		
		expression_ptr rhs = additive_expr();
		switch (op)
		{
			case xp_OperatorLess:
				result.reset(new operator_expression<xp_OperatorLess>(result, rhs));
				break;

			case xp_OperatorLessOrEqual:
				result.reset(new operator_expression<xp_OperatorLessOrEqual>(result, rhs));
				break;

			case xp_OperatorGreater:
				result.reset(new operator_expression<xp_OperatorGreater>(result, rhs));
				break;

			case xp_OperatorGreaterOrEqual:
				result.reset(new operator_expression<xp_OperatorGreaterOrEqual>(result, rhs));
				break;
			
			default:
				break;
		}
	}
	
	return result;
}

expression_ptr xpath_imp::additive_expr()
{
	expression_ptr result(multiplicative_expr());
	
	while (m_lookahead == xp_OperatorAdd or m_lookahead == xp_OperatorSubstract)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		if (op == xp_OperatorAdd)
			result.reset(new operator_expression<xp_OperatorAdd>(result, multiplicative_expr()));
		else
			result.reset(new operator_expression<xp_OperatorSubstract>(result, multiplicative_expr()));
	}
	
	return result;
}

expression_ptr xpath_imp::multiplicative_expr()
{
	expression_ptr result(unary_expr());

	for (;;)
	{
		if (m_lookahead == xp_Asterisk)
		{
			match(m_lookahead);
			result.reset(new operator_expression<xp_Asterisk>(result, unary_expr()));
			continue;
		}

		if (m_lookahead == xp_Name and m_token_string == "mod")
		{
			match(m_lookahead);
			result.reset(new operator_expression<xp_OperatorMod>(result, unary_expr()));
			continue;
		}
			
		if (m_lookahead == xp_Name and m_token_string == "div")
		{
			match(m_lookahead);
			result.reset(new operator_expression<xp_OperatorDiv>(result, unary_expr()));
			continue;
		}

		break;
	}
	
	return result;
}

expression_ptr xpath_imp::unary_expr()
{
	expression_ptr result;
	
	if (m_lookahead == xp_OperatorSubstract)
	{
		match(xp_OperatorSubstract);
		result.reset(new negate_expression(unary_expr()));
	}
	else
		result = union_expr();
	
	return result;
}

// --------------------------------------------------------------------

node_set xpath_imp::evaluate(node& root, context_imp& ctxt)
{
	node_set empty;
	expression_context context(ctxt, &root, empty);
	return m_expr->evaluate(context).as<const node_set&>();
}

// --------------------------------------------------------------------

context::context()
	: m_impl(new context_imp)
{
}

context::~context()
{
	delete m_impl;
}

template<>
void context::set<double>(const string& name, const double& value)
{
	m_impl->set(name, value);
}

template<>
double context::get<double>(const string& name)
{
	return m_impl->get(name).as<double>();
}

template<>
void context::set<string>(const string& name, const string& value)
{
	m_impl->set(name, value);
}

template<>
string context::get<string>(const string& name)
{
	return m_impl->get(name).as<string>();
}

// --------------------------------------------------------------------

xpath::xpath(const string& path)
	: m_impl(new xpath_imp())
{
	m_impl->parse(path);
}

xpath::xpath(const xpath& rhs)
	: m_impl(rhs.m_impl)
{
	m_impl->reference();
}

xpath& xpath::operator=(const xpath& rhs)
{
	if (this != &rhs)
	{
		m_impl->release();
		m_impl = rhs.m_impl;
		m_impl->reference();
	}
	
	return *this;
}

xpath::~xpath()
{
	m_impl->release();
}

template<>
node_set xpath::evaluate<node>(const node& root) const
{
	context ctxt;
	return evaluate<node>(root, ctxt);
}

template<>
node_set xpath::evaluate<node>(const node& root, context& ctxt) const
{
	return m_impl->evaluate(const_cast<node&>(root), *ctxt.m_impl);
}

template<>
element_set xpath::evaluate<element>(const node& root) const
{
	context ctxt;
	return evaluate<element>(root, ctxt);
}

template<>
element_set xpath::evaluate<element>(const node& root, context& ctxt) const
{
	element_set result;
	
	object s(m_impl->evaluate(const_cast<node&>(root), *ctxt.m_impl));
	foreach (node* n, s.as<const node_set&>())
	{
		element* e = dynamic_cast<element*>(n);
		if (e != nullptr)
			result.push_back(e);
	}
	
	return result;
}

bool xpath::matches(const node* n) const
{
	bool result = false;
	if (n != nullptr)
	{
		const node* root = n->root();
	
		context ctxt;
		object s(m_impl->evaluate(const_cast<node&>(*root), *ctxt.m_impl));
		foreach (node* e, s.as<const node_set&>())
		{
			if (e == n)
			{
				result = true;
				break;
			}
		}
	}
	
	return result;
}

}
}
