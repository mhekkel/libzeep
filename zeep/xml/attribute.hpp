//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_ATTRIBUTE_HPP
#define SOAP_XML_ATTRIBUTE_HPP

#include <list>
#include <boost/ptr_container/ptr_list.hpp>

namespace zeep { namespace xml {

// --------------------------------------------------------------------

class attribute
{
	friend class node;

  public:
//						attribute();
//						
						attribute(
							const std::string&	name,
							const std::string&	value)
							: m_name(name)
							, m_value(value) {}

	std::string			name() const							{ return m_name; }
//	void				name(const std::string& name)			{ m_name = name; }

	std::string			value() const							{ return m_value; }
	void				value(const std::string& value)			{ m_value = value; }

	bool				operator==(const attribute& a) const	{ return m_name == a.m_name and m_value == a.m_value; }
	bool				operator!=(const attribute& a) const	{ return m_name != a.m_name or m_value != a.m_value; }
	
  private:
	std::string			m_name;
	std::string			m_value;
};

typedef boost::ptr_list<attribute>		attribute_list;

}
}

#endif
