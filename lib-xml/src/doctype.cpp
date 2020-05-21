//  Copyright Maarten L. Hekkelman, Radboud University 2010-2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define _SCL_SECURE_NO_WARNINGS

#include <numeric>

#include <boost/algorithm/string.hpp>

#include <zeep/exception.hpp>
#include <zeep/xml/doctype.hpp>
#include <zeep/xml/character-classification.hpp>

namespace ba = boost::algorithm;

namespace zeep::xml::doctype
{

// --------------------------------------------------------------------
// validator code

// a refcounted state base class
struct state_base : std::enable_shared_from_this<state_base>
{
	state_base() : m_ref_count(1) {}

	virtual std::tuple<bool, bool> allow(const std::string& name) = 0;
	virtual bool allow_char_data() { return false; }
	virtual bool allow_empty() { return false; }
	virtual bool must_be_empty() { return false; }

	virtual void reset() {}

	void reference() { ++m_ref_count; }
	void release()
	{
		if (--m_ref_count == 0)
			delete this;
	}

protected:
	virtual ~state_base() { assert(m_ref_count == 0); }

private:
	int m_ref_count;
};

struct state_any : public state_base
{
	virtual std::tuple<bool, bool>
	allow(const std::string& name) { return std::make_tuple(true, true); }
	virtual bool allow_char_data() { return true; }
	virtual bool allow_empty() { return true; }
};

struct state_empty : public state_base
{
	virtual std::tuple<bool, bool>
	allow(const std::string& name) { return std::make_tuple(false, true); }
	virtual bool allow_empty() { return true; }
	virtual bool must_be_empty() { return true; }
};

struct state_element : public state_base
{
	state_element(const std::string& name)
		: m_name(name), m_done(false) {}

	virtual std::tuple<bool, bool>
	allow(const std::string& name)
	{
		bool result = false;
		if (not m_done and m_name == name)
			m_done = result = true;
		//									m_done = true;
		return std::make_tuple(result, m_done);
	}

	virtual void reset() { m_done = false; }

	std::string m_name;
	bool m_done;
};

struct state_repeated : public state_base
{
	state_repeated(content_spec_ptr sub)
		: m_sub(sub->create_state()), m_state(0) {}

	~state_repeated()
	{
		m_sub->release();
	}

	virtual void reset()
	{
		m_sub->reset();
		m_state = 0;
	}

	virtual bool allow_char_data() { return m_sub->allow_char_data(); }

	state_ptr m_sub;
	int m_state;
};

// repeat for ?

struct state_repeated_zero_or_once : public state_repeated
{
	state_repeated_zero_or_once(content_spec_ptr sub)
		: state_repeated(sub) {}

	std::tuple<bool, bool> allow(const std::string& name);

	virtual bool allow_empty() { return true; }
};

std::tuple<bool, bool> state_repeated_zero_or_once::allow(const std::string& name)
{
	// use a state machine
	enum State
	{
		state_Start = 0,
		state_Loop
	};

	bool result = false, done = false;

	switch (m_state)
	{
	case state_Start:
		std::tie(result, done) = m_sub->allow(name);
		if (result == true)
			m_state = state_Loop;
		else
			done = true;
		break;

	case state_Loop:
		std::tie(result, done) = m_sub->allow(name);
		if (result == false and done)
			done = true;
		break;
	}

	return std::make_tuple(result, done);
}

struct state_repeated_any : public state_repeated
{
	state_repeated_any(content_spec_ptr sub)
		: state_repeated(sub) {}

	std::tuple<bool, bool> allow(const std::string& name);

	virtual bool allow_empty() { return true; }
};

std::tuple<bool, bool> state_repeated_any::allow(const std::string& name)
{
	// use a state machine
	enum State
	{
		state_Start = 0,
		state_Loop
	};

	bool result = false, done = false;

	switch (m_state)
	{
	case state_Start:
		std::tie(result, done) = m_sub->allow(name);
		if (result == true)
			m_state = state_Loop;
		else
			done = true;
		break;

	case state_Loop:
		std::tie(result, done) = m_sub->allow(name);
		if (result == false and done)
		{
			m_sub->reset();
			std::tie(result, done) = m_sub->allow(name);
			if (result == false)
				done = true;
		}
		break;
	}

	return std::make_tuple(result, done);
}

struct state_repeated_at_least_once : public state_repeated
{
	state_repeated_at_least_once(content_spec_ptr sub)
		: state_repeated(sub) {}

	std::tuple<bool, bool> allow(const std::string& name);

	virtual bool allow_empty() { return m_sub->allow_empty(); }
};

std::tuple<bool, bool> state_repeated_at_least_once::allow(const std::string& name)
{
	// use a state machine
	enum State
	{
		state_Start = 0,
		state_FirstLoop,
		state_NextLoop
	};

	bool result = false, done = false;

	switch (m_state)
	{
	case state_Start:
		std::tie(result, done) = m_sub->allow(name);
		if (result == true)
			m_state = state_FirstLoop;
		break;

	case state_FirstLoop:
		std::tie(result, done) = m_sub->allow(name);
		if (result == false and done)
		{
			m_sub->reset();
			std::tie(result, done) = m_sub->allow(name);
			if (result == true)
				m_state = state_NextLoop;
		}
		break;

	case state_NextLoop:
		std::tie(result, done) = m_sub->allow(name);
		if (result == false and done)
		{
			m_sub->reset();
			std::tie(result, done) = m_sub->allow(name);
			if (result == false)
				done = true;
		}
		break;
	}

	return std::make_tuple(result, done);
}

// allow a sequence

struct state_seq : public state_base
{
	state_seq(const content_spec_list& allowed)
		: m_state(0)
	{
		for (content_spec_ptr a : allowed)
			m_states.push_back(a->create_state());
	}

	~state_seq()
	{
		for (state_ptr s : m_states)
			s->release();
	}

	virtual std::tuple<bool, bool>
	allow(const std::string& name);

	virtual void reset()
	{
		m_state = 0;
		std::for_each(m_states.begin(), m_states.end(), [](auto state) { state->reset(); });
	}

	virtual bool allow_char_data()
	{
		bool result = false;
		for (state_ptr s : m_states)
		{
			if (s->allow_char_data())
			{
				result = true;
				break;
			}
		}

		return result;
	}

	virtual bool allow_empty();

	std::list<state_ptr> m_states;
	std::list<state_ptr>::iterator m_next;
	int m_state;
};

std::tuple<bool, bool> state_seq::allow(const std::string& name)
{
	bool result = false, done = false;

	enum State
	{
		state_Start,
		state_Element
	};

	switch (m_state)
	{
	case state_Start:
		m_next = m_states.begin();
		if (m_next == m_states.end())
		{
			done = true;
			break;
		}
		m_state = state_Element;
		// fall through

	case state_Element:
		std::tie(result, done) = (*m_next)->allow(name);
		while (result == false and done)
		{
			++m_next;

			if (m_next == m_states.end())
			{
				done = true;
				break;
			}

			std::tie(result, done) = (*m_next)->allow(name);
		}
		break;
	}

	return std::make_tuple(result, done);
}

bool state_seq::allow_empty()
{
	bool result;

	if (m_states.empty())
		result = true;
	else
	{
		result = accumulate(m_states.begin(), m_states.end(), true,
							[](bool flag, auto& state) { return state->allow_empty() and flag; });
	}

	return result;
}

// allow one of a list

struct state_choice : public state_base
{
	state_choice(const content_spec_list& allowed, bool mixed)
		: m_mixed(mixed), m_state(0)
	{
		for (content_spec_ptr a : allowed)
			m_states.push_back(a->create_state());
	}

	~state_choice()
	{
		for (state_ptr s : m_states)
			s->release();
	}

	virtual std::tuple<bool, bool>
	allow(const std::string& name);

	virtual void reset()
	{
		m_state = 0;
		std::for_each(m_states.begin(), m_states.end(), [](auto& state) { state->reset(); });
	}

	virtual bool allow_char_data() { return m_mixed; }

	virtual bool allow_empty();

	std::list<state_ptr> m_states;
	bool m_mixed;
	int m_state;
	state_ptr m_sub;
};

std::tuple<bool, bool> state_choice::allow(const std::string& name)
{
	bool result = false, done = false;

	enum State
	{
		state_Start,
		state_Choice
	};

	switch (m_state)
	{
	case state_Start:
		for (auto choice = m_states.begin(); choice != m_states.end(); ++choice)
		{
			std::tie(result, done) = (*choice)->allow(name);
			if (result == true)
			{
				m_sub = *choice;
				m_state = state_Choice;
				break;
			}
		}
		break;

	case state_Choice:
		std::tie(result, done) = m_sub->allow(name);
		break;
	}

	return std::make_tuple(result, done);
}

bool state_choice::allow_empty()
{
	return m_mixed or
		   std::find_if(m_states.begin(), m_states.end(), std::bind(&state_base::allow_empty, std::placeholders::_1)) != m_states.end();
}

// --------------------------------------------------------------------

validator::validator(content_spec_ptr allowed)
	: m_state(allowed->create_state()), m_allowed(allowed), m_done(m_state->allow_empty())
{
}

validator::validator(const element_* e)
	: m_allowed(e ? e->get_allowed() : nullptr)
{
	if (m_allowed == nullptr)
	{
		m_state = new state_any();
		m_done = true;
	}
	else
	{
		m_state = m_allowed->create_state();
		m_done = m_state->allow_empty();
	}
}

validator::~validator()
{
	m_state->release();
}

bool validator::allow(const std::string& name)
{
	bool result;
	std::tie(result, m_done) = m_state->allow(name);
	return result;
}

bool validator::done()
{
	return m_done;
}

ContentSpecType validator::get_content_spec() const
{
	return m_allowed ? m_allowed->get_content_spec() : ContentSpecType::Any;
}

// --------------------------------------------------------------------

state_ptr content_spec_any::create_state() const
{
	return new state_any();
}

// --------------------------------------------------------------------

state_ptr content_spec_empty::create_state() const
{
	return new state_empty();
}

// --------------------------------------------------------------------

state_ptr content_spec_element::create_state() const
{
	return new state_element(m_name);
}

// --------------------------------------------------------------------

content_spec_repeated::~content_spec_repeated()
{
	delete m_allowed;
}

state_ptr content_spec_repeated::create_state() const
{
	switch (m_repetition)
	{
	case '?':
		return new state_repeated_zero_or_once(m_allowed);
	case '*':
		return new state_repeated_any(m_allowed);
	case '+':
		return new state_repeated_at_least_once(m_allowed);
	default:
		assert(false);
		throw zeep::exception("illegal repetition character");
	}
}

bool content_spec_repeated::element_content() const
{
	return m_allowed->element_content();
}

// --------------------------------------------------------------------

content_spec_seq::~content_spec_seq()
{
	for (content_spec_ptr a : m_allowed)
		delete a;
}

void content_spec_seq::add(content_spec_ptr a)
{
	m_allowed.push_back(a);
}

state_ptr content_spec_seq::create_state() const
{
	return new state_seq(m_allowed);
}

bool content_spec_seq::element_content() const
{
	bool result = true;
	for (content_spec_ptr a : m_allowed)
	{
		if (not a->element_content())
		{
			result = false;
			break;
		}
	}
	return result;
}

// --------------------------------------------------------------------

content_spec_choice::~content_spec_choice()
{
	for (content_spec_ptr a : m_allowed)
		delete a;
}

void content_spec_choice::add(content_spec_ptr a)
{
	m_allowed.push_back(a);
}

state_ptr content_spec_choice::create_state() const
{
	return new state_choice(m_allowed, m_mixed);
}

bool content_spec_choice::element_content() const
{
	bool result = true;
	if (m_mixed)
		result = false;
	else
	{
		for (content_spec_ptr a : m_allowed)
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

bool attribute_::is_name(std::string& s) const
{
	bool result = true;

	ba::trim(s);

	if (not s.empty())
	{
		std::string::iterator c = s.begin();

		if (c != s.end())
			result = is_name_start_char(*c);

		while (result and ++c != s.end())
			result = is_name_char(*c);
	}

	return result;
}

bool attribute_::is_names(std::string& s) const
{
	bool result = true;

	ba::trim(s);

	if (not s.empty())
	{
		std::string::iterator c = s.begin();
		std::string t;

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

			result = isspace(*c) != 0;
			++c;
			t += ' ';

			while (c != s.end() and isspace(*c))
				++c;
		}

		swap(s, t);
	}

	return result;
}

bool attribute_::is_nmtoken(std::string& s) const
{
	ba::trim(s);

	bool result = not s.empty();

	std::string::iterator c = s.begin();
	while (result and ++c != s.end())
		result = is_name_char(*c);

	return result;
}

bool attribute_::is_nmtokens(std::string& s) const
{
	// remove leading and trailing spaces
	ba::trim(s);

	bool result = not s.empty();

	std::string::iterator c = s.begin();
	std::string t;

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
		} while (c != s.end());

		if (not result or c == s.end())
			break;

		result = false;
		do
		{
			if (*c != ' ')
				break;
			result = true;
			++c;
		} while (c != s.end() and *c == ' ');

		t += ' ';
	}

	if (result)
		swap(s, t);

	return result;
}

bool attribute_::validate_value(std::string& value, const entity_list& entities) const
{
	bool result = true;

	if (m_type == AttributeType::CDATA)
		result = true;
	else if (m_type == AttributeType::ENTITY)
	{
		result = is_name(value);
		if (result)
			result = is_unparsed_entity(value, entities);
	}
	else if (m_type == AttributeType::ID or m_type == AttributeType::IDREF)
		result = is_name(value);
	else if (m_type == AttributeType::ENTITIES)
	{
		result = is_names(value);
		if (result)
		{
			std::vector<std::string> values;
			ba::split(values, value, ba::is_any_of(" "));
			for (const std::string& v : values)
			{
				if (not is_unparsed_entity(v, entities))
				{
					result = false;
					break;
				}
			}
		}
	}
	else if (m_type == AttributeType::IDREFS)
		result = is_names(value);
	else if (m_type == AttributeType::NMTOKEN)
		result = is_nmtoken(value);
	else if (m_type == AttributeType::NMTOKENS)
		result = is_nmtokens(value);
	else if (m_type == AttributeType::Enumerated or m_type == AttributeType::Notation)
	{
		ba::trim(value);
		result = find(m_enum.begin(), m_enum.end(), value) != m_enum.end();
	}

	if (result and m_default == AttributeDefault::Fixed and value != m_default_value)
		result = false;

	return result;
}

bool attribute_::is_unparsed_entity(const std::string& s, const entity_list& l) const
{
	bool result = false;

	entity_list::const_iterator i = std::find_if(l.begin(), l.end(), [s](auto e) { return e->name() == s; });
	if (i != l.end())
		result = (*i)->is_parsed() == false;

	return result;
}

// --------------------------------------------------------------------

element_::~element_()
{
	for (attribute_* attr : m_attlist)
		delete attr;
	delete m_allowed;
}

void element_::set_allowed(content_spec_ptr allowed)
{
	if (allowed != m_allowed)
	{
		delete m_allowed;
		m_allowed = allowed;
	}
}

void element_::add_attribute(attribute_* attrib)
{
	std::unique_ptr<attribute_> attr(attrib);
	if (find_if(m_attlist.begin(), m_attlist.end(), [attrib](auto a) { return a->name() == attrib->name(); }) == m_attlist.end())
		m_attlist.push_back(attr.release());
}

const attribute_* element_::get_attribute(const std::string& name) const
{
	attribute_list::const_iterator dta =
		find_if(m_attlist.begin(), m_attlist.end(), [name](auto a) { return a->name() == name; });

	const attribute_* result = nullptr;

	if (dta != m_attlist.end())
		result = *dta;

	return result;
}

// validator element::get_validator() const
// {
// 	// validator valid;
// 	// if (m_allowed)
// 	// 	valid = validator(m_allowed);
// 	return { m_allowed };
// }

bool element_::empty() const
{
	return dynamic_cast<content_spec_empty *>(m_allowed) != nullptr;
}

} // namespace zeep::xml::doctype
