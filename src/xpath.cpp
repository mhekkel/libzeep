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
