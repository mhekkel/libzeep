//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include "zeep/xml/document.hpp"
#include "zeep/xml/iomanip.hpp"

using namespace std;

namespace zeep { namespace xml {

pretty::pretty(int indent, bool empty, bool wrap, bool trim)
	: m_base(NULL), m_indent(indent), m_empty(empty), m_wrap(wrap), m_trim(trim)
{
}

pretty::~pretty()
{
}
	
pretty& pretty::operator<<(const document& doc)
{
//	*m_base << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	return *this;
}

pretty& pretty::operator<<(const node& n)
{
	n.write(*m_base, 0, m_indent, m_empty, m_wrap, m_trim);
	return *this;
}

void pretty::set_base(std::ostream& os)
{
	m_base = &os;
}

ostream& operator<<(std::ostream& lhs, pretty& rhs)
{
	rhs.set_base(lhs);
	return rhs;
}
	
}
}
