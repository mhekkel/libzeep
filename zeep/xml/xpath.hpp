//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include "zeep/xml/node.hpp"

namespace zeep { namespace xml {

class document;

class xpath
{
  public:
							xpath(const std::string& path);
	virtual					~xpath();

	template<typename NODE_TYPE>
	std::list<NODE_TYPE*>	evaluate(const node& root) const;

  private:
	struct xpath_imp*		m_impl;
};

template<>
node_set xpath::evaluate<node>(const node& root) const;

template<>
element_set xpath::evaluate<element>(const node& root) const;
	
}
}
