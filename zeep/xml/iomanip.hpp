//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ZEEP_XML_IOMANIP
#define ZEEP_XML_IOMANIP

#include <iostream>

namespace zeep { namespace xml {

class node;
class document;

class pretty : public std::ostream
{
  public:
					pretty(int indent, bool empty, bool wrap = true, bool trim = false);
	virtual			~pretty();
	
	pretty&			operator<<(const document& doc);
	pretty&			operator<<(const node& doc);

  private:
	
	friend std::ostream& operator<<(std::ostream& lhs, pretty& rhs);

	void			set_base(std::ostream& os);
	
	std::ostream*	m_base;
	int				m_indent;
	bool			m_empty;
	bool			m_wrap;
	bool			m_trim;
};

std::ostream& operator<<(std::ostream& lhs, pretty& rhs);

}
}

#endif
