//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/bind.hpp>

#include "zeep/xml/doctype.hpp"
#include "zeep/xml/unicode_support.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml { namespace doctype {

// --------------------------------------------------------------------

bool attribute::is_name(wstring& s)
{
	bool result = true;
	
	ba::trim(s);
	
	if (not s.empty())
	{
		wstring::iterator c = s.begin();
		
		if (c != s.end())
			result = is_name_start_char(*c);
		
		while (result and ++c != s.end())
			result = is_name_char(*c);
	}
	
	return result;
}

bool attribute::is_names(wstring& s)
{
	bool result = true;

	ba::trim(s);
	
	if (not s.empty())
	{
		wstring::iterator c = s.begin();
		wstring t;
		
		while (result and c != s.end())
		{
			result = is_name_start_char(*c);
			t += *c;
			++c;
			
			while (result and c != s.end() and is_name_char(*c))
			{
				t += *c;
				++c;
			}
			
			if (c == s.end())
				break;
			
			result = isspace(*c);
			++c;
			t += ' '; 
			
			while (isspace(*c))
				++c;
		}

		swap(s, t);
	}
	
	return result;
}

bool attribute::is_nmtoken(wstring& s)
{
	bool result = true;

	ba::trim(s);
	
	wstring::iterator c = s.begin();
	while (result and ++c != s.end())
		result = is_name_char(*c);
	
	return result;
}

bool attribute::is_nmtokens(wstring& s)
{
	bool result = true;

	// remove leading and trailing spaces
	ba::trim(s);
	
	wstring::iterator c = s.begin();
	wstring t;
	
	while (result and c != s.end())
	{
		result = false;
		
		do
		{
			if (not is_name_char(*c))
				break;
			result = true;
			t += *c;
			++c;
		}
		while (c != s.end());
		
		if (not result or c == s.end())
			break;
		
		result = false;
		do
		{
			if (not isspace(*c))
				break;
			result = true;
			++c;
		}
		while (isspace(*c));
		
		t += ' ';
	}
	
	if (result)
		swap(s, t);
	
	return result;
}

bool attribute::validate_value(wstring& value, const entity_list& entities)
{
	bool result = true;

	if (m_type == attTypeString)
		result = true;
	else if (m_type == attTypeTokenizedENTITY)
	{
		result = is_name(value);
		if (result)
			result = is_unparsed_entity(value, entities);
	}
	else if (m_type == attTypeTokenizedID or m_type == attTypeTokenizedIDREF)
		result = is_name(value);
	else if (m_type == attTypeTokenizedENTITIES)
	{
		result = is_names(value);
		if (result)
		{
			vector<wstring> values;
			ba::split(values, value, ba::is_any_of(" "));
			foreach (const wstring& v, values)
			{
				if (not is_unparsed_entity(v, entities))
				{
					result = false;
					break;
				}
			}
		}
	}
	else if (m_type == attTypeTokenizedIDREFS)
		result = is_names(value);
	else if (m_type == attTypeTokenizedNMTOKEN)
		result = is_nmtoken(value);
	else if (m_type == attTypeTokenizedNMTOKENS)
		result = is_nmtokens(value);
	else if (m_type == attTypeEnumerated or m_type == attTypeNotation)
	{
		ba::trim(value);
		result = find(m_enum.begin(), m_enum.end(), value) != m_enum.end();
	}
	
	if (result and m_default == attDefFixed and value != m_default_value)
		result = false;
	
	return result;
}

bool attribute::is_unparsed_entity(const wstring& s, const entity_list& l)
{
	bool result = false;
	
	entity_list::const_iterator i = find_if(l.begin(), l.end(), boost::bind(&entity::name, _1) == s);
	if (i != l.end())
		result = i->parsed() == false;
	
	return result;
}

// --------------------------------------------------------------------

element::~element()
{
}

void element::add_attribute(auto_ptr<attribute> attr)
{
	if (find_if(m_attlist.begin(), m_attlist.end(), boost::bind(&attribute::name, _1) == attr->name()) == m_attlist.end())
		m_attlist.push_back(attr.release());
}

attribute* element::get_attribute(const wstring& name)
{
	attribute_list::iterator dta =
		find_if(m_attlist.begin(), m_attlist.end(), boost::bind(&attribute::name, _1) == name);
	
	attribute* result = NULL;
	
	if (dta != m_attlist.end())
		result = &(*dta);
	
	return result;
}

}
}
}
