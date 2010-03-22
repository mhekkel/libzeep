//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <tr1/tuple>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "zeep/xml/doctype.hpp"
#include "zeep/xml/unicode_support.hpp"

using namespace std;
using namespace tr1;
namespace ba = boost::algorithm;

namespace zeep { namespace xml { namespace doctype {

// --------------------------------------------------------------------
// validator code

struct state_base;
typedef boost::shared_ptr<state_base>	state_ptr;

state_ptr kSentinel;

struct state_base
{
					state_base() : m_done(false) {}

	virtual tuple<bool,state_ptr>
					allow(const wstring& name) = 0;

	virtual bool	done()						{ return m_done; }
	
	bool			m_done;
};

template<typename T>
struct state : public state_base
{
	typedef	T			allowed_type;
		
						state(const allowed_type& allowed)
							: m_allowed(allowed) {}

	virtual tuple<bool,state_ptr>
						allow(const wstring& name);
	
	const allowed_type&	m_allowed;
};

template<>
tuple<bool,state_ptr> state<allowed_any>::allow(const wstring& name)
{
	return make_tuple(true, this);
}

template<>
tuple<bool,state_ptr> state<allowed_empty>::allow(const wstring& name)
{
	return make_tuple(false, this);
}

template<>
tuple<bool,state_ptr> state<allowed_element>::allow(const wstring& name)
{
	return make_tuple(m_allowed.m_name == name, kSentinel);
}

template<>
struct state<allowed_zero_or_one> : public state_base
{
				state(const allowed_zero_or_one& allowed)
					: state_base()
					, m_next(allowed.m_allowed->create_validator())
					, m_seen(false)
				{
					m_done = true;
				}

	virtual tuple<bool,state_ptr>
				allow(const wstring& name)
				{
					bool result = false;
					
					if (not m_seen and m_next.allow(name))
					{
						result = true;
						m_seen = true;
					}
					
					return make_tuple(result, kSentinel);
				}

	validator	m_next;
	bool		m_seen;
};

template<>
struct state<allowed_one_or_more> : public state_base
{
				state(const allowed_one_or_more& allowed)
					: state_base()
					, m_next(allowed.m_allowed->create_validator())
				{
				}

	virtual tuple<bool,state_ptr>
				allow(const wstring& name)
				{
					bool result = m_next.allow(name);
					
					if (result)
						m_done = true;
					
					return make_tuple(result, this);
				}

	validator	m_next;
};

template<>
struct state<allowed_zero_or_more> : public state_base
{
				state(const allowed_zero_or_more& allowed)
					: state_base()
					, m_next(allowed.m_allowed->create_validator())
				{
					m_done = true;
				}

	virtual tuple<bool,state_ptr>
				allow(const wstring& name)
				{
					bool result = m_next.allow(name);
					return make_tuple(result, this);
				}

	validator	m_next;
};

template<>
struct state<allowed_mixed> : public state_base
{
				state(const allowed_mixed& allowed)
					: state_base()
				{
					m_done = true;
				}

	virtual tuple<bool,state_ptr>
				allow(const wstring& name)
				{
					return make_tuple(true, this);
				}
};

template<>
struct state<allowed_seq> : public state_base
{
				state(const allowed_seq& allowed)
					: state_base()
					, m_seq(allowed)
					, m_next(m_seq.begin())
				{
				}

	virtual tuple<bool,state_ptr>
				allow(const wstring& name)
				{
					bool result = false;
					state_ptr next = kSentinel;
					
					if (not m_done and m_next != m_seq.end())
					{
						tie(result, next) = m_
						
						
					}
					
					return make_tuple(true, this);
				}

	allowed_seq			m_seq;
	allowed_seq::iter	m_next;
};

template<>
struct state<allowed_choice> : public state_base
{
				state(const allowed_choice& allowed)
					: state_base()
				{
					m_done = true;
				}

	virtual tuple<bool,state_ptr>
				allow(const wstring& name)
				{
					return make_tuple(true, this);
				}
};

class validator_imp
{
  public:
					validator_imp()
						: m_state(NULL) {}

	template<typename A>
					validator_imp(const A& allowed, bool done = false);

	bool			allow(const wstring& name)
					{
						bool result = false;
						if (m_state != NULL)
							tie(result, m_state) = m_state->allow(name);
						return result;
					}

	state_ptr		m_state;
};

template<typename A>
validator_imp::validator_imp(const A& allowed, bool done)
{
	m_state = new state<A>(allowed);
	m_state->m_done = done;
}

// --------------------------------------------------------------------

validator::validator()
	: m_impl(new validator_imp())
{
	allowed_any any;
	m_impl->m_state = new state<allowed_any>(any);
}

template<typename A>
validator::validator(const A& allowed)
	: m_impl(new validator_imp(allowed))
{
}

validator::validator(const validator& other)
	: m_impl(other.m_impl)
{
}

validator& validator::operator=(const validator& other)
{
	m_impl = other.m_impl;
	return *this;
}

void validator::reset()
{
	assert(false);
}

bool validator::allow(const wstring& name)
{
	return m_impl->allow(name);
}

bool validator::done()
{
	return m_impl->m_state == NULL;
}

// --------------------------------------------------------------------

validator allowed_any::create_validator() const
{
	return validator(*this);
}

validator allowed_empty::create_validator() const
{
	return validator(*this);
}

validator allowed_element::create_validator() const
{
	return validator(*this);
}

validator allowed_zero_or_one::create_validator() const
{
	return validator(*this);
}

validator allowed_one_or_more::create_validator() const
{
	return validator(*this);
}

validator allowed_zero_or_more::create_validator() const
{
	return validator(*this);
}

validator allowed_seq::create_validator() const
{
	return validator(*this);
}

validator allowed_choice::create_validator() const
{
	return validator(*this);
}

validator allowed_mixed::create_validator() const
{
	return validator(*this);
}

// --------------------------------------------------------------------

bool attribute::is_name(wstring& s) const
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

bool attribute::is_names(wstring& s) const
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

bool attribute::is_nmtoken(wstring& s) const
{
	bool result = true;

	ba::trim(s);
	
	wstring::iterator c = s.begin();
	while (result and ++c != s.end())
		result = is_name_char(*c);
	
	return result;
}

bool attribute::is_nmtokens(wstring& s) const
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

bool attribute::validate_value(wstring& value, const entity_list& entities) const
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

bool attribute::is_unparsed_entity(const wstring& s, const entity_list& l) const
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

const attribute* element::get_attribute(const wstring& name) const
{
	attribute_list::const_iterator dta =
		find_if(m_attlist.begin(), m_attlist.end(), boost::bind(&attribute::name, _1) == name);
	
	const attribute* result = NULL;
	
	if (dta != m_attlist.end())
		result = &(*dta);
	
	return result;
}

validator element::get_validator() const
{
	validator valid;
	if (m_allowed)
		valid = m_allowed->create_validator();
	return valid;
}

}
}
}
