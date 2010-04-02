//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>

namespace zeep { namespace xml {

class document;
class node;

class xpath
{
  public:
						xpath(const std::string& path);
	virtual				~xpath();

	node_set			evaluate(const node& root);

  private:
	struct xpath_imp*	m_impl;
};
	
}
}
