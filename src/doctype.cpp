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

struct state_base : boost::enable_shared_from_this<state_base>
{
						state_base()
							: m_done(false) {}

	virtual bool		allow(const wstring& name) = 0;
	
	virtual void		reset()
						{
							m_done = false;
						}

	bool				m_done;
};

struct state_any : public state_base
{
	virtual bool		allow(const wstring& name)			{ return true; }
};

struct state_empty : public state_base
{
	virtual bool		allow(const wstring& name)			{ return false; }
};

struct state_element : public state_base
{
						state_element(const wstring& name)
							: m_name(name) {}

	virtual bool		allow(const wstring& name)
						{
							bool result = false;
							if (not m_done and m_name == name)
								m_done = result = true;
							return result;
						}

	wstring				m_name;
};

struct state_repeated : public state_base
{
						state_repeated(allowed_ptr sub, char repetition)
							: m_sub(sub->create_state()), m_repetition(repetition), m_state(0) {}

	virtual bool		allow(const wstring& name)
						{
							switch (m_repetition)
							{
								case '?':	return allow_once(name);
								case '*':	return allow_any(name);
								case '+':	return allow_at_least_once(name);
								default:	return false;
							}
						}
	
	bool				allow_once(const wstring& name);
	bool				allow_any(const wstring& name);
	bool				allow_at_least_once(const wstring& name);

	virtual void		reset()								{ state_base::reset(); m_sub->reset(); m_state = 0; }

	state_ptr			m_sub;
	char				m_repetition;
	int					m_state;
};

bool state_repeated::allow_any(const wstring& name)
{
	// use a state machine
	enum State {
		state_Start			= 0,
		state_Loop,
		state_Done
	};
	
	bool result = false;
	
	switch (m_state)
	{
		case state_Start:
			result = m_sub->allow(name);
			if (result == true)
				m_state = state_Loop;
			else
				m_state = state_Done;
			break;
		
		case state_Loop:
			result = m_sub->allow(name);
			if (result == false)
			{
				if (m_sub->m_done)
				{
					m_sub->reset();
					result = m_sub->allow(name);
					if (result == false)
						m_state = state_Done;
				}
			}
			break;

		case state_Done:
			break;
	}
	
	m_done = (m_state == state_Done);
	
	return result;
}

bool state_repeated::allow_at_least_once(const wstring& name)
{
	// use a state machine
	enum State {
		state_Start			= 0,
		state_FirstLoop,
		state_NextLoop,
		state_Done
	};
	
	bool result = false;
	
	switch (m_state)
	{
		case state_Start:
			result = m_sub->allow(name);
			if (result == true)
				m_state = state_FirstLoop;
			break;
		
		case state_FirstLoop:
			result = m_sub->allow(name);
			if (result == false)
			{
				if (m_sub->m_done)
				{
					m_sub->reset();
					result = m_sub->allow(name);
					if (result == true)
						m_state = state_NextLoop;
				}
			}
			break;

		case state_NextLoop:
			result = m_sub->allow(name);
			if (result == false)
			{
				if (m_sub->m_done)
				{
					m_sub->reset();
					result = m_sub->allow(name);
					if (result == false)
						m_state = state_Done;
				}
			}

		case state_Done:
			break;
	}
	
	m_done = (m_state == state_Done);
	
	return result;
}


// allow zero or one ('?')
bool state_repeated::allow_once(const wstring& name)
{
	// use a state machine
	enum State {
		state_Start			= 0,
		state_Loop,
		state_Done
	};
	
	bool result = false;
	
	switch (m_state)
	{
		case state_Start:
			result = m_sub->allow(name);
			if (result == true)
				m_state = state_Loop;
			else
				m_state = state_Done;
			break;
		
		case state_Loop:
			result = m_sub->allow(name);
			if (result == false)
			{
				if (m_sub->m_done)
					m_state = state_Done;
			}
			break;

		case state_Done:
			break;
	}
	
	m_done = (m_state == state_Done);
	
	return result;
}

// allow a sequence

struct state_seq : public state_base
{
					state_seq(const allowed_list& allowed)
						: m_state(0)
					{
						foreach (allowed_ptr a, allowed)
							m_states.push_back(a->create_state());
					}

	virtual bool	allow(const wstring& name);

	virtual void	reset()
					{
						state_base::reset();
						m_state = 0;
						for_each(m_states.begin(), m_states.end(), boost::bind(&state_base::reset, _1));
					}
	
	list<state_ptr>				m_states;
	list<state_ptr>::iterator	m_next;
	int							m_state;
};

bool state_seq::allow(const wstring& name)
{
	bool result = false;
	
	enum State {
		state_Start,
		state_Element,
		state_Done
	};
	
	switch (m_state)
	{
		case state_Start:
			m_next = m_states.begin();
			if (m_next == m_states.end())
			{
				m_state = state_Done;
				break;
			}
			m_state = state_Element;
			// fall through
		
		case state_Element:
			result = (*m_next)->allow(name);
			while (result == false and (*m_next)->m_done)
			{
				++m_next;
				
				if (m_next == m_states.end())
				{
					m_state = state_Done;
					break;
				}
				else
					result = (*m_next)->allow(name);
			}
			break;
			
		case state_Done:
			break;
			
	}
	
	m_done = (m_state == state_Done);
	
	return result;
}

// allow one of a list

struct state_choice : public state_base
{
					state_choice(const allowed_list& allowed, bool mixed)
						: m_mixed(mixed)
						, m_state(0)
					{
						foreach (allowed_ptr a, allowed)
							m_states.push_back(a->create_state());
					}

	virtual bool	allow(const wstring& name);

	virtual void	reset()
					{
						state_base::reset();
						m_state = 0;
						for_each(m_states.begin(), m_states.end(), boost::bind(&state_base::reset, _1));
					}
	
	list<state_ptr>	m_states;
	bool			m_mixed;
	int				m_state;
	state_ptr		m_sub;
};

bool state_choice::allow(const wstring& name)
{
	bool result = false;
	
	enum State {
		state_Start,
		state_Choice,
		state_Done
	};
	
	switch (m_state)
	{
		case state_Start:
			m_state = state_Done;
			for (list<state_ptr>::iterator choice = m_states.begin(); choice != m_states.end(); ++choice)
			{
				result = (*choice)->allow(name);
				if (result == true)
				{
					m_sub = *choice;
					m_state = state_Choice;
					break;
				}
			}
			break;

		case state_Choice:
			result = m_sub->allow(name);
			if (result == false and m_sub->m_done)
				m_state = state_Done;
			break;
		
		case state_Done:
			break;
	}
	
	m_done = (m_state == state_Done);
	
	return result;
}

// --------------------------------------------------------------------

int validator::s_next_nr = 1;

validator::validator()
	: m_state(new state_any())
	, m_nr(0)
{
}

validator::validator(allowed_ptr allowed)
	: m_state(allowed->create_state())
	, m_allowed(allowed)
	, m_nr(s_next_nr++)
{
}

validator::validator(const validator& other)
	: m_state(other.m_state)
	, m_allowed(other.m_allowed)
	, m_nr(other.m_nr)
{
}

validator& validator::operator=(const validator& other)
{
	m_nr = other.m_nr;
	m_state = other.m_state;
	m_allowed = other.m_allowed;
	return *this;
}

void validator::reset()
{
	m_state->reset();
}

bool validator::allow(const wstring& name)
{
	bool result = m_state->allow(name);
	
	if (VERBOSE and m_allowed)
	{
		cout << "state machine " << m_nr << ' ' << (result ? "succeeded" : "failed") <<  " for " << wstring_to_string(name) << endl;
		m_allowed->print(cout);
		cout << endl << endl;
	}

	return result;
}

bool validator::done()
{
//	(void)m_state->allow(L"");
//	return m_state->done();
	return true;
}

std::ostream& operator<<(std::ostream& lhs, validator& rhs)
{
	rhs.m_allowed->print(lhs);
	return lhs;
}

// --------------------------------------------------------------------

state_ptr allowed_any::create_state() const
{
	return state_ptr(new state_any());
}

void allowed_any::print(ostream& os)
{
	os << "ANY";
}

state_ptr allowed_empty::create_state() const
{
	return state_ptr(new state_empty());
}

void allowed_empty::print(ostream& os)
{
	os << "EMPTY";
}

state_ptr allowed_element::create_state() const
{
	return state_ptr(new state_element(m_name));
}

void allowed_element::print(ostream& os)
{
	os << wstring_to_string(m_name);
}

state_ptr allowed_repeated::create_state() const
{
	return state_ptr(new state_repeated(m_allowed, m_repetition));
}

void allowed_repeated::print(ostream& os)
{
	m_allowed->print(os); os << m_repetition;
}

bool allowed_repeated::element_content() const
{
	return m_allowed->element_content();
}

state_ptr allowed_seq::create_state() const
{
	return state_ptr(new state_seq(m_allowed));
}

void allowed_seq::print(ostream& os)
{
	os << '(';
	for (allowed_list::iterator s = m_allowed.begin(); s != m_allowed.end(); ++s)
	{
		(*s)->print(os);
		if (next(s) != m_allowed.end())
			os << ", ";
	}
	os << ')';
}

bool allowed_seq::element_content() const
{
	bool result = true;
	foreach (allowed_ptr a, m_allowed)
	{
		if (not a->element_content())
		{
			result = false;
			break;
		}
	}
	return result;
}

state_ptr allowed_choice::create_state() const
{
	return state_ptr(new state_choice(m_allowed, m_mixed));
}

void allowed_choice::print(ostream& os)
{
	os << '(';

	if (m_mixed)
	{
		os << "#PCDATA";
		if (not m_allowed.empty())
			os << "|";
	}

	for (allowed_list::iterator s = m_allowed.begin(); s != m_allowed.end(); ++s)
	{
		(*s)->print(os);
		if (next(s) != m_allowed.end())
			os << "|";
	}
	os << ')';
}

bool allowed_choice::element_content() const
{
	bool result = true;
	if (m_mixed)
		result = false;
	else
	{
		foreach (allowed_ptr a, m_allowed)
		{
			if (not a->element_content())
			{
				result = false;
				break;
			}
		}
	}
	return result;
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
	{
		valid = validator(m_allowed);
//		cout << "created validator for " << wstring_to_string(name()) << endl
//			 << " >> " << valid << endl;
	}
	return valid;
}

bool element::empty() const
{
	return dynamic_cast<allowed_empty*>(m_allowed.get()) != nil;
}

bool element::element_content() const
{
	return m_allowed != nil and m_allowed->element_content();
}

}
}
}
