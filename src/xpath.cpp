//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include "zeep/xml/node.hpp"
#include "zeep/xml/xpath.hpp"

using namespace std;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct xpath_imp
{
						xpath_imp(const string& path);
						~xpath_imp();
	
	node_list			evaluate(node& root);

	enum Token {
		xp_EOF,
		xp_LeftParenthesis,
		xp_RightParenthesis,
		xp_LeftBracket,
		xp_RightBracket,
		xp_Dot,
		xp_DoubleDot,
		xp_At,
		xp_Comma,
		xp_DoubleColon,
		xp_NameTest,
		xp_NodeType,
		xp_Operator,
		xp_FunctionName,
		xp_AxisName,
		xp_Literal,
		xp_Number,
		xp_Variable
	};

	enum State {
		xps_Start		= 0,
		xps_Number		= 100,
		xps_Name		= 200
	};

	void				parse(const string& path);
	
	wchar_t				get_next_char();
	void				restart(State& start, State& state);
	Token				get_next_token();
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
	
};




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
