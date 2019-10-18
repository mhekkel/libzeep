// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ZEEP_XML_DOCTYPE_HPP
#define ZEEP_XML_DOCTYPE_HPP

#include <zeep/config.hpp>

#include <set>
#include <vector>
#include <list>

namespace zeep
{
namespace xml
{
namespace doctype
{

// --------------------------------------------------------------------
// doctype support with full validation.

class element;
class attlist;
class entity;
class attribute;

typedef std::vector<entity *> entity_list;
typedef std::vector<element *> element_list;
typedef std::vector<attribute *> attribute_list;

// --------------------------------------------------------------------

enum class ContentSpecType { Empty, Any, Mixed, Children };

// --------------------------------------------------------------------
// validation of elements is done by the validator classes

struct content_spec_base;
typedef content_spec_base* content_spec_ptr;
typedef std::list<content_spec_ptr> content_spec_list;

struct state_base;
typedef state_base* state_ptr;

class validator
{
public:
	validator(content_spec_base* allowed);
	validator(const element* e);

	validator(const validator& other) = delete;
	validator& operator=(const validator& other) = delete;

	~validator();

	bool allow(const std::string& name);
	ContentSpecType content_spec() const;
	bool done();

	bool operator()(const std::string& name) { return allow(name); }

private:
	friend std::ostream& operator<<(std::ostream& lhs, validator& rhs);

	state_ptr m_state;
	content_spec_ptr m_allowed;
	int m_nr;
	static int s_next_nr;
	bool m_done;
};

// --------------------------------------------------------------------

struct content_spec_base
{
	content_spec_base(const content_spec_base&) = delete;
	content_spec_base& operator=(const content_spec_base&) = delete;

	virtual ~content_spec_base() {}

	virtual state_ptr create_state() const = 0;
	virtual bool element_content() const { return false; }

	ContentSpecType content_spec() const	{ return m_content_spec; }

	virtual void print(std::ostream& os) = 0;

  protected:

	content_spec_base(ContentSpecType contentSpec)
		: m_content_spec(contentSpec) {}

	ContentSpecType m_content_spec;
};

struct content_spec_any : public content_spec_base
{
	content_spec_any() : content_spec_base(ContentSpecType::Any) {}

	virtual state_ptr create_state() const;
	virtual void print(std::ostream& os);
};

struct content_spec_empty : public content_spec_base
{
	content_spec_empty() : content_spec_base(ContentSpecType::Empty) {}

	virtual state_ptr create_state() const;
	virtual void print(std::ostream& os);
};

struct content_spec_element : public content_spec_base
{
	content_spec_element(const std::string& name)
		: content_spec_base(ContentSpecType::Children), m_name(name) {}

	virtual state_ptr create_state() const;
	virtual bool element_content() const { return true; }

	virtual void print(std::ostream& os);

	std::string m_name;
};

struct content_spec_repeated : public content_spec_base
{
	content_spec_repeated(content_spec_ptr allowed, char repetion)
		: content_spec_base(allowed->content_spec()), m_allowed(allowed), m_repetition(repetion)
	{
		assert(allowed);
	}

	~content_spec_repeated();

	virtual state_ptr create_state() const;
	virtual bool element_content() const;

	virtual void print(std::ostream& os);

	content_spec_ptr m_allowed;
	char m_repetition;
};

struct content_spec_seq : public content_spec_base
{
	content_spec_seq(content_spec_ptr a)
		: content_spec_base(a->content_spec()) { add(a); }
	~content_spec_seq();

	void add(content_spec_ptr a);

	virtual state_ptr create_state() const;
	virtual bool element_content() const;

	virtual void print(std::ostream& os);

	content_spec_list m_allowed;
};

struct content_spec_choice : public content_spec_base
{
	content_spec_choice(bool mixed)
		: content_spec_base(mixed ? ContentSpecType::Mixed : ContentSpecType::Children), m_mixed(mixed) {}
	content_spec_choice(content_spec_ptr a, bool mixed)
		: content_spec_base(mixed ? ContentSpecType::Mixed : a->content_spec()), m_mixed(mixed) { add(a); }
	~content_spec_choice();

	void add(content_spec_ptr a);

	virtual state_ptr create_state() const;
	virtual bool element_content() const;

	virtual void print(std::ostream& os);

	content_spec_list m_allowed;
	bool m_mixed;
};

// --------------------------------------------------------------------

enum AttributeType
{
	attTypeString,
	attTypeTokenizedID,
	attTypeTokenizedIDREF,
	attTypeTokenizedIDREFS,
	attTypeTokenizedENTITY,
	attTypeTokenizedENTITIES,
	attTypeTokenizedNMTOKEN,
	attTypeTokenizedNMTOKENS,
	attTypeNotation,
	attTypeEnumerated
};

enum AttributeDefault
{
	attDefNone,
	attDefRequired,
	attDefImplied,
	attDefFixed,
	attDefDefault
};

class attribute
{
public:
	attribute(const std::string& name, AttributeType type)
		: m_name(name), m_type(type), m_default(attDefNone), m_external(false) {}

	attribute(const std::string& name, AttributeType type,
			  const std::vector<std::string> &enums)
		: m_name(name), m_type(type), m_default(attDefNone), m_enum(enums), m_external(false) {}

	const std::string& name() const { return m_name; }

	bool validate_value(std::string& value, const entity_list& entities) const;

	void set_default(AttributeDefault def, const std::string& value)
	{
		m_default = def;
		m_default_value = value;
	}

	std::tuple<AttributeDefault, std::string>
	get_default() const { return std::make_tuple(m_default, m_default_value); }

	AttributeType get_type() const { return m_type; }
	AttributeDefault get_default_type() const { return m_default; }
	const std::vector<std::string> &
	get_enums() const { return m_enum; }

	void external(bool external) { m_external = external; }
	bool external() const { return m_external; }

private:
	// routines used to check _and_ reformat attribute value strings
	bool is_name(std::string& s) const;
	bool is_names(std::string& s) const;
	bool is_nmtoken(std::string& s) const;
	bool is_nmtokens(std::string& s) const;

	bool is_unparsed_entity(const std::string& s, const entity_list& l) const;

	std::string m_name;
	AttributeType m_type;
	AttributeDefault m_default;
	std::string m_default_value;
	std::vector<std::string> m_enum;
	bool m_external;
};

// --------------------------------------------------------------------

class element
{
public:
	element(const element &) = delete;
	element& operator=(const element &) = delete;

	element(const std::string& name, bool declared, bool external)
		: m_name(name), m_allowed(nullptr), m_declared(declared), m_external(external) {}

	~element();

	void add_attribute(attribute* attr);

	const attribute* get_attribute(const std::string& name) const;

	const std::string& name() const { return m_name; }

	const attribute_list &
	attributes() const { return m_attlist; }

	void set_allowed(content_spec_ptr allowed);

	void declared(bool declared) { m_declared = declared; }
	bool declared() const { return m_declared; }

	void external(bool external) { m_external = external; }
	bool external() const { return m_external; }

	bool empty() const;
	bool element_content() const;

	ContentSpecType content_spec() const { return m_allowed->content_spec(); }

	content_spec_ptr allowed() const { return m_allowed; }

private:
	std::string m_name;
	attribute_list m_attlist;
	content_spec_ptr m_allowed;
	bool m_declared, m_external;
};

// --------------------------------------------------------------------

class entity
{
public:
	entity(const entity &) = delete;
	entity& operator=(const entity &) = delete;

	const std::string& name() const { return m_name; }
	const std::string& replacement() const { return m_replacement; }
	const std::string& path() const { return m_path; }
	bool parameter() const { return m_parameter; }

	bool parsed() const { return m_parsed; }
	void parsed(bool parsed) { m_parsed = parsed; }

	const std::string& ndata() const { return m_ndata; }
	void ndata(const std::string& ndata) { m_ndata = ndata; }

	bool external() const { return m_external; }

	bool externally_defined() const { return m_externally_defined; }
	void externally_defined(bool externally_defined)
	{
		m_externally_defined = externally_defined;
	}

protected:
	entity(const std::string& name, const std::string& replacement,
		   bool external, bool parsed)
		: m_name(name), m_replacement(replacement), m_parameter(false), m_parsed(parsed), m_external(external), m_externally_defined(false) {}

	entity(const std::string& name, const std::string& replacement,
		   const std::string& path)
		: m_name(name), m_replacement(replacement), m_path(path), m_parameter(true), m_parsed(true), m_external(true), m_externally_defined(false) {}

	std::string m_name;
	std::string m_replacement;
	std::string m_ndata;
	std::string m_path;
	bool m_parameter;
	bool m_parsed;
	bool m_external;
	bool m_externally_defined;
};

class general_entity : public entity
{
public:
	general_entity(const std::string& name, const std::string& replacement,
				   bool external = false, bool parsed = true)
		: entity(name, replacement, external, parsed) {}
};

class parameter_entity : public entity
{
public:
	parameter_entity(const std::string& name, const std::string& replacement,
					 const std::string& path)
		: entity(name, replacement, path) {}
};

} // namespace doctype
} // namespace xml
} // namespace zeep

#endif
