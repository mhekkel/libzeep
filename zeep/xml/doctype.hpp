//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ZEEP_XML_DOCTYPE_HPP
#define ZEEP_XML_DOCTYPE_HPP

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem/operations.hpp>

namespace zeep { namespace xml { namespace doctype {

// --------------------------------------------------------------------
// doctype support. We don't do full validation yet, but here is the support
// for filling in default values and cleaning up attributes.

class element;
class attlist;
class entity;
class attribute;

typedef boost::ptr_vector<entity>		entity_list;
typedef boost::ptr_vector<element>		element_list;
typedef boost::ptr_vector<attribute>	attribute_list;

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

class attribute : public boost::noncopyable
{
  public:
						attribute(const std::wstring& name, AttributeType type)
							: m_name(name), m_type(type), m_default(attDefNone) {}

						attribute(const std::wstring& name, AttributeType type,
								const std::vector<std::wstring>& enums)
							: m_name(name), m_type(type), m_default(attDefNone)
							, m_enum(enums) {}

	std::wstring		name() const							{ return m_name; }

	bool				validate_value(std::wstring& value, const entity_list& entities);
	
	void				set_default(AttributeDefault def, const std::wstring& value)
						{
							m_default = def;
							m_default_value = value;
						}

	boost::tuple<AttributeDefault,std::wstring>
						get_default() const						{ return boost::make_tuple(m_default, m_default_value); }
	
	AttributeType		get_type() const						{ return m_type; }
	AttributeDefault	get_default_type() const				{ return m_default; }
	const std::vector<std::wstring>&
						get_enums() const						{ return m_enum; }
	
  private:

	// routines used to check _and_ reformat attribute value strings
	bool				is_name(std::wstring& s);
	bool				is_names(std::wstring& s);
	bool				is_nmtoken(std::wstring& s);
	bool				is_nmtokens(std::wstring& s);

	bool				is_unparsed_entity(const std::wstring& s, const entity_list& l);

	std::wstring		m_name;
	AttributeType		m_type;
	AttributeDefault	m_default;
	std::wstring		m_default_value;
	std::vector<std::wstring>m_enum;
};

// --------------------------------------------------------------------

class element : boost::noncopyable
{
  public:
						element(const std::wstring& name)
							: m_name(name) {}

						~element();

	void				add_attribute(std::auto_ptr<attribute> attr);
	
	attribute*			get_attribute(const std::wstring& name);

	std::wstring		name() const								{ return m_name; }
	
	const attribute_list&
						attributes() const							{ return m_attlist; }

  private:
	std::wstring		m_name;
	attribute_list		m_attlist;
};

// --------------------------------------------------------------------

class entity : boost::noncopyable
{
  public:
	const std::wstring&	name() const							{ return m_name; }		
	const std::wstring&	replacement() const						{ return m_replacement; }		
	const boost::filesystem::path&
						path() const							{ return m_path; }		
	bool				parameter() const						{ return m_parameter; }		

	bool				parsed() const							{ return m_parsed; }
	void				parsed(bool parsed)						{ m_parsed = parsed; }

	const std::wstring&	ndata() const							{ return m_ndata; }
	void				ndata(const std::wstring& ndata)		{ m_ndata = ndata; }

	bool				external() const						{ return m_external; }

	bool				externally_defined() const				{ return m_externally_defined; }
	void				externally_defined(bool externally_defined)
																{ m_externally_defined = externally_defined; }

  protected:
						entity(const std::wstring& name, const std::wstring& replacement,
								bool external, bool parsed)
							: m_name(name), m_replacement(replacement), m_parameter(false), m_parsed(parsed), m_external(external)
							, m_externally_defined(false) {}

						entity(const std::wstring& name, const std::wstring& replacement,
								const boost::filesystem::path& path)
							: m_name(name), m_replacement(replacement), m_path(path), m_parameter(true), m_parsed(true), m_external(true)
							, m_externally_defined(false) {}

	std::wstring		m_name;
	std::wstring		m_replacement;
	std::wstring		m_ndata;
	boost::filesystem::path
						m_path;
	bool				m_parameter;
	bool				m_parsed;
	bool				m_external;
	bool				m_externally_defined;
};

class general_entity : public entity
{
  public:
						general_entity(const std::wstring& name, const std::wstring& replacement,
								bool external = false, bool parsed = true)
							: entity(name, replacement, external, parsed) {}
};

class parameter_entity : public entity
{
  public:
						parameter_entity(const std::wstring& name, const std::wstring& replacement,
								const boost::filesystem::path& path)
							: entity(name, replacement, path) {}
};
	
}
}
}

#endif
