//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <tr1/tuple>
#include <typeinfo>

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

#define nil NULL

extern int VERBOSE;

namespace zeep { namespace xml { namespace doctype {

// --------------------------------------------------------------------
// validator code

state_ptr kSentinel;

struct state_base : boost::enable_shared_from_this<state_base>
{
						state_base(bool done)
							: m_done(done), m_done_at_start(done), m_count(0) {}

	virtual bool		allow(const wstring& name) = 0;

	virtual bool		done()									{ return m_done; }

	virtual void		reset()
						{
							if (m_sub)
								m_sub->reset();
							m_done = m_done_at_start;
							m_count = 0;
						}

	virtual void		print() = 0;
	
	bool				m_done, m_done_at_start;
	int					m_count;
	state_ptr			m_sub;
};

template<typename T>
struct state : public state_base
{
	typedef	T			allowed_type;
		
						state(const allowed_type& allowed = allowed_type())
							: state_base(allowed_type::may_be_empty)
							, m_allowed(allowed)
						{
						}

	virtual bool		allow(const wstring& name);

	virtual void		reset()									{ state_base::reset(); }

	virtual void		print();

	const allowed_type&	m_allowed;
};

// allow any (everything is allowed here)
template<>
bool state<allowed_any>::allow(const wstring& name)
{
	return true;
}

template<>
void state<allowed_any>::print()
{
	cout << "ANY";
}

// allow empty (nothing is allowed here)
template<>
bool state<allowed_empty>::allow(const wstring& name)
{
	return false;
}

template<>
void state<allowed_empty>::print()
{
	cout << "EMPTY";
}

// allow element, expect just one specific element
template<>
bool state<allowed_element>::allow(const wstring& name)
{
	m_done = true;
	return m_allowed.m_name == name;
}

template<>
void state<allowed_element>::print()
{
	cout << wstring_to_string(m_allowed.m_name);
}

// allow zero or more ('*')

template<>
state<allowed_zero_or_more>::state(const allowed_zero_or_more& allowed)
	: state_base(true)
	, m_allowed(allowed)
{
	m_sub = allowed.m_allowed->create_state();
}

template<>
bool state<allowed_zero_or_more>::allow(const wstring& name)
{
	bool result = m_sub->allow(name);
	
	if (result == false and m_sub->done())
	{
		++m_count;
		
		m_sub->reset();

		result = m_sub->allow(name);
	}

	return result;
}

template<>
void state<allowed_zero_or_more>::print()
{
	m_sub->print();
	cout << '*';
}

// allow one or more ('+')

template<>
state<allowed_one_or_more>::state(const allowed_one_or_more& allowed)
	: state_base(false)
	, m_allowed(allowed)
{
	m_sub = allowed.m_allowed->create_state();
}

template<>
bool state<allowed_one_or_more>::allow(const wstring& name)
{
	if (m_done)
	{
		m_sub->reset();
		m_done = false;
	}
	
	bool result = m_sub->allow(name);
	m_done = m_sub->done();
	
	if (result == false)
	{
		m_done = m_sub->done();
		if (m_done)
		{
			m_sub->reset();
			result = m_sub->allow(name);
		}
	}
	
	if (result)
		++m_count;

	return result;
}

template<>
void state<allowed_one_or_more>::print()
{
	m_sub->print();
	cout << '+';
}

// allow zero or one ('?')

template<>
state<allowed_zero_or_one>::state(const allowed_zero_or_one& allowed)
	: state_base(true)
	, m_allowed(allowed)
{
	m_sub = allowed.m_allowed->create_state();
}

template<>
bool state<allowed_zero_or_one>::allow(const wstring& name)
{
	bool result = false;
	if (m_count == 0)
	{
		result = m_sub->allow(name);
		if (m_sub->done())
			++m_count;
	}
	m_done = true;
	return result;
}

template<>
void state<allowed_zero_or_one>::print()
{
	m_sub->print();
	cout << '?';
}

// allow mixed

template<>
bool state<allowed_mixed>::allow(const wstring& name)
{
	return true;
}

template<>
void state<allowed_mixed>::print()
{
	cout << "#MIXED";
}

// allow a sequence

template<>
struct state<allowed_seq> : public state_base
{
					state(const allowed_seq& allowed)
						: state_base(allowed.m_allowed.empty())
					{
						foreach (allowed_ptr a, allowed.m_allowed)
							m_states.push_back(a->create_state());
						
						m_next = m_states.begin();
					}

	virtual bool	allow(const wstring& name)
					{
						bool result = false;
						while (result == false and m_next != m_states.end())
						{
							result = (*m_next)->allow(name);
							if ((*m_next)->done())
								++m_next;
						}
						
						if (m_next == m_states.end())
							m_done = true;
						
						return result;
					}

	virtual void	reset()
					{
						state_base::reset();
						for_each(m_states.begin(), m_states.end(), boost::bind(&state_base::reset, _1));
						m_next = m_states.begin();
					}

	virtual void	print()
					{
						cout << '(';
						for (list<state_ptr>::iterator s = m_states.begin(); s != m_states.end(); ++s)
						{
							(*s)->print();
							if (next(s) != m_states.end())
								cout << ", ";
						}
						cout << ')';
					}

	
	list<state_ptr>				m_states;
	list<state_ptr>::iterator	m_next;
};

// allow one of a list

template<>
struct state<allowed_choice> : public state_base
{
					state(const allowed_choice& allowed)
						: state_base(false)
					{
						foreach (allowed_ptr a, allowed.m_allowed)
							m_states.push_back(a->create_state());
						m_cur = m_states.end();
					}

	virtual bool	allow(const wstring& name)
					{
						bool result = false;

						if (m_cur != m_states.end())
							result = (*m_cur)->allow(name);
						else
						{
							for (m_cur = m_states.begin(); result == false and m_cur != m_states.end(); ++m_cur)
								result = (*m_cur)->allow(name);
						}
						
						if (m_cur == m_states.end() or (*m_cur)->done())
							m_done = true;
						
						return result;
					}

	virtual void	print()
					{
						cout << '(';
						for (list<state_ptr>::iterator s = m_states.begin(); s != m_states.end(); ++s)
						{
							(*s)->print();
							if (next(s) != m_states.end())
								cout << "|";
						}
						cout << ')';
					}

	virtual void	reset()
					{
						state_base::reset();
						for_each(m_states.begin(), m_states.end(), boost::bind(&state_base::reset, _1));
						m_cur = m_states.end();
					}
	
	list<state_ptr>				m_states;
	list<state_ptr>::iterator	m_cur;
};

// --------------------------------------------------------------------

validator::validator()
	: m_start(new state<allowed_any>())
{
	m_state = m_start;
}

validator::validator(state_ptr state)
	: m_start(state)
	, m_state(state)
{
}

validator::validator(const validator& other)
	: m_start(other.m_start)
	, m_state(other.m_state)
{
}

validator& validator::operator=(const validator& other)
{
	m_start = other.m_start;
	m_state = other.m_state;
	return *this;
}

void validator::reset()
{
	m_state = m_start;
}

bool validator::allow(const wstring& name)
{
//	bool result = m_state->allow(name);
//	if (not result and VERBOSE)
//	{
//		cout << "state machine failed for " << wstring_to_string(name) << endl;
//		m_state->print();
//		cout << endl;
//	}
//	return result;

	bool result = m_state->allow(name);
	
	if (VERBOSE)
	{
		cout << "state machine " << (result ? "succeeded" : "failed") <<  " for " << wstring_to_string(name) << endl;
		m_state->print();
		cout << endl << endl;
	}
	return result;
}

bool validator::done()
{
	return m_state->done();
}

// --------------------------------------------------------------------

state_ptr allowed_any::create_state() const
{
	return state_ptr(new state<allowed_any>(*this));
}

state_ptr allowed_empty::create_state() const
{
	return state_ptr(new state<allowed_empty>(*this));
}

state_ptr allowed_element::create_state() const
{
	return state_ptr(new state<allowed_element>(*this));
}

state_ptr allowed_zero_or_one::create_state() const
{
	return state_ptr(new state<allowed_zero_or_one>(*this));
}

state_ptr allowed_one_or_more::create_state() const
{
	return state_ptr(new state<allowed_one_or_more>(*this));
}

state_ptr allowed_zero_or_more::create_state() const
{
	return state_ptr(new state<allowed_zero_or_more>(*this));
}

state_ptr allowed_seq::create_state() const
{
	return state_ptr(new state<allowed_seq>(*this));
}

state_ptr allowed_choice::create_state() const
{
	return state_ptr(new state<allowed_choice>(*this));
}

state_ptr allowed_mixed::create_state() const
{
	return state_ptr(new state<allowed_mixed>(*this));
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
	
	const attribute* result = nil;
	
	if (dta != m_attlist.end())
		result = &(*dta);
	
	return result;
}

validator element::get_validator() const
{
	validator valid;
	if (m_allowed)
		valid = validator(m_allowed->create_state());
	return valid;
}

}
}
}
