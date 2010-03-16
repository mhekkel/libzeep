//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>
#include <deque>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tuple/tuple.hpp>

#include "zeep/xml/document.hpp"
#include "zeep/exception.hpp"

#include "zeep/xml/parser.hpp"

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

#ifndef nil
#define nil NULL
#endif

namespace zeep { namespace xml {

// --------------------------------------------------------------------

namespace
{

// first some very basic code to check the class of scanned characters

bool is_name_start_char(wchar_t uc)
{
	return
		uc == L':' or
		(uc >= L'A' and uc <= L'Z') or
		uc == L'_' or
		(uc >= L'a' and uc <= L'z') or
		(uc >= 0x0C0 and uc <= 0x0D6) or
		(uc >= 0x0D8 and uc <= 0x0F6) or
		(uc >= 0x0F8 and uc <= 0x02FF) or
		(uc >= 0x0370 and uc <= 0x037D) or
		(uc >= 0x037F and uc <= 0x01FFF) or
		(uc >= 0x0200C and uc <= 0x0200D) or
		(uc >= 0x02070 and uc <= 0x0218F) or
		(uc >= 0x02C00 and uc <= 0x02FEF) or
		(uc >= 0x03001 and uc <= 0x0D7FF) or
		(uc >= 0x0F900 and uc <= 0x0FDCF) or
		(uc >= 0x0FDF0 and uc <= 0x0FFFD) or
		(uc >= 0x010000 and uc <= 0x0EFFFF);	
}

bool is_name_char(wchar_t uc)
{
	return
		is_name_start_char(uc) or
		uc == '-' or
		uc == '.' or
		(uc >= '0' and uc <= '9') or
		uc == 0x0B7 or
		(uc >= 0x00300 and uc <= 0x0036F) or
		(uc >= 0x0203F and uc <= 0x02040);
}

bool is_char(wchar_t uc)
{
	return
		uc == 0x09 or
		uc == 0x0A or
		uc == 0x0D or
		(uc >= 0x020 and uc <= 0x0D7FF) or
		(uc >= 0x0E000 and uc <= 0x0FFFD) or
		(uc >= 0x010000 and uc <= 0x010FFFF);
}

}

enum Encoding {
	enc_UTF8,
	enc_UTF16BE,
	enc_UTF16LE,
	enc_ISO88591
};

// parsing XML is somewhat like macro processing,
// we can encounter entities that need to be expanded into replacement text
// and so we declare data_source objects that can be stacked.

class data_source;
typedef boost::shared_ptr<data_source>	data_ptr;

// A data source can have a base dir, that is the directory the data came from.
// This information is needed when a relative uri is found in an external ID.

class data_source : public boost::enable_shared_from_this<data_source>, boost::noncopyable
{
  public:
					data_source(data_ptr next)
						: m_next(next), m_base_dir(fs::current_path()) {}

	virtual			~data_source() {}

	// data_source is a virtual base class. Derivatives need to declare the next function.
	virtual wchar_t	get_next_char() = 0;

	// to avoid recursively nested entity values, we have a check:
	virtual bool	is_entity_on_stack(const wstring& name)
					{
						bool result = false;
						if (m_next != nil)
							result = m_next->is_entity_on_stack(name);
						return result;
					}

	void			base_dir(const fs::path& dir)					{ m_base_dir = dir; }
	const fs::path&	base_dir() const								{ return m_base_dir; }

	virtual bool	auto_discard() const							{ return false; }

	data_ptr		next_data_source() const						{ return m_next; }

  protected:
	data_ptr		m_next;			// generates a linked list of data_sources
	fs::path		m_base_dir;
};

// --------------------------------------------------------------------
// An istream implementation of data_source.

class istream_data_source : public data_source
{
  public:
					istream_data_source(istream& data, data_ptr next = data_ptr())
						: data_source(next)
						, m_data(data)
						, m_char_buffer(0)
						, m_encoding(enc_UTF8)
						, m_has_bom(false)
					{
						guess_encoding();
					}

					istream_data_source(auto_ptr<istream> data, data_ptr next = data_ptr())
						: data_source(next)
						, m_data(*data)
						, m_data_ptr(data)
						, m_char_buffer(0)
						, m_encoding(enc_UTF8)
						, m_has_bom(false)
					{
						guess_encoding();
					}

	bool			has_bom()				{ return m_has_bom; }

  private:

	void			guess_encoding();
	
	void			parse_text_decl();
	
	virtual wchar_t	get_next_char();

	wchar_t			next_utf8_char();
	
	wchar_t			next_utf16le_char();
	
	wchar_t			next_utf16be_char();
	
	wchar_t			next_iso88591_char();

	char			next_byte();

	istream&		m_data;
	auto_ptr<istream>
					m_data_ptr;
	stack<char>		m_byte_buffer;
	wchar_t			m_char_buffer;	// used in detecting \r\n algorithm
	Encoding		m_encoding;

	boost::function<wchar_t(void)>
					m_next;
	bool			m_has_bom;
	bool			m_valid_utf8;
};

void istream_data_source::guess_encoding()
{
	// see if there is a BOM
	// if there isn't, we assume the data is UTF-8
	
	char c1 = 0, c2 = 0, c3 = 0;
	
	m_data.read(&c1, 1);

	if (c1 == char(0xfe))
	{
		m_data.read(&c2, 1);
		
		if (c2 == char(0xff))
		{
			m_encoding = enc_UTF16BE;
			m_has_bom = true;
		}
		else
		{
			m_byte_buffer.push(c2);
			m_byte_buffer.push(c1);
		}
	}
	else if (c1 == char(0xff))
	{
		m_data.read(&c2, 1);
		
		if (c2 == char(0xfe))
		{
			m_encoding = enc_UTF16LE;
			m_has_bom = true;
		}
		else
		{
			m_byte_buffer.push(c2);
			m_byte_buffer.push(c1);
		}
	}
	else if (c1 == char(0xef))
	{
		m_data.read(&c2, 1);
		m_data.read(&c3, 1);
		
		if (c2 == char(0xbb) and c3 == char(0xbf))
		{
			m_encoding = enc_UTF8;
			m_has_bom = true;
		}
		else
		{
			m_byte_buffer.push(c3);
			m_byte_buffer.push(c2);
			m_byte_buffer.push(c1);
		}
	}
	else
		m_byte_buffer.push(c1);

	switch (m_encoding)
	{
		case enc_UTF8:		m_next = boost::bind(&istream_data_source::next_utf8_char, this); break;
		case enc_UTF16LE:	m_next = boost::bind(&istream_data_source::next_utf16le_char, this); break;
		case enc_UTF16BE:	m_next = boost::bind(&istream_data_source::next_utf16be_char, this); break;
		case enc_ISO88591:	m_next = boost::bind(&istream_data_source::next_iso88591_char, this); break;
	}
}

char istream_data_source::next_byte()
{
	char result = 0;
	
	if (m_byte_buffer.empty())
	{
		if (not m_data.eof())
			m_data.read(&result, 1);
	}
	else
	{
		result = m_byte_buffer.top();
		m_byte_buffer.pop();
	}

	return result;
}

wchar_t istream_data_source::next_utf8_char()
{
	unsigned long result = 0;
	unsigned char ch[5];
	
	ch[0] = next_byte();
	
	if ((ch[0] & 0x080) == 0)
		result = ch[0];
	else if ((ch[0] & 0x0E0) == 0x0C0)
	{
		ch[1] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<unsigned long>(((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F));
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<unsigned long>(((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F));
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<unsigned long>(((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F));
	}

	if (result > 0x10ffff)
		throw exception("invalid utf-8 character (out of range)");
	
	if (m_data.eof())
		result = 0;
	
	return static_cast<wchar_t>(result);
}

wchar_t istream_data_source::next_utf16le_char()
{
	unsigned char c1 = next_byte(), c2 = next_byte();
	
	unsigned long result = (static_cast<unsigned long>(c2) << 8) | c1;

	return static_cast<wchar_t>(result);
}

wchar_t istream_data_source::next_utf16be_char()
{
	unsigned char c1 = next_byte(), c2 = next_byte();
	
	unsigned long result = (static_cast<unsigned long>(c1) << 8) | c2;

	return static_cast<wchar_t>(result);
}

wchar_t istream_data_source::next_iso88591_char()
{
	throw exception("to be implemented");
	return 0;
}

wchar_t istream_data_source::get_next_char()
{
	wchar_t ch = m_char_buffer;

	if (ch == 0)
		ch = m_next();
	else
		m_char_buffer = 0;
	
	if (ch == '\r')
	{
		ch = m_next();
		if (ch != '\n')
			m_char_buffer = ch;
		ch = '\n';
	}
	
	return ch;
}

// --------------------------------------------------------------------

class wstring_data_source : public data_source
{
  public:
						wstring_data_source(const wstring& data, data_ptr next = data_ptr())
							: data_source(next)
							, m_data(data)
							, m_ptr(m_data.begin())
						{
						}

  private:

	virtual wchar_t		get_next_char();

	wstring				m_data;
	wstring::iterator	m_ptr;
};

wchar_t	wstring_data_source::get_next_char()
{
	wchar_t result = 0;

	if (m_ptr != m_data.end())
	{
		result = *m_ptr;
		++m_ptr;
	}

	return result;
}

// --------------------------------------------------------------------

class entity_data_source : public wstring_data_source
{
  public:
					entity_data_source(const wstring& entity_name, const fs::path& entity_path,
							const wstring& text, data_ptr next = data_ptr())
						: wstring_data_source(text, next)
						, m_entity_name(entity_name)
					{
						base_dir(entity_path.branch_path());
					}

	virtual bool	is_entity_on_stack(const wstring& name)
					{
						bool result = m_entity_name == name;
						if (result == false and m_next != nil)
							result = m_next->is_entity_on_stack(name);
						return result;
					}

  protected:
	wstring			m_entity_name;
};

// --------------------------------------------------------------------

class parameter_entity_data_source : public wstring_data_source
{
  public:
					parameter_entity_data_source(const wstring& data, const fs::path& base, data_ptr next = data_ptr())
						: wstring_data_source(wstring(L" ") + data + L" ", next)
					{
						base_dir(base);
					}

	virtual bool	auto_discard() const							{ return true; }
};

// --------------------------------------------------------------------
// doctype support. We don't do full validation yet, but here is the support
// for filling in default values and cleaning up attributes.

class doctype_element;
class doctype_attlist;

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

class doctype_attribute : public boost::noncopyable
{
  public:
						doctype_attribute(const wstring& name, AttributeType type)
							: m_name(name), m_type(type), m_default(attDefNone) {}

						doctype_attribute(const wstring& name, const vector<wstring>& enums)
							: m_name(name), m_type(attTypeEnumerated), m_default(attDefNone), m_enum(enums) {}

	wstring				name() const							{ return m_name; }

	bool				validate_value(wstring& value);
	
	void				set_default(AttributeDefault def, const wstring& value)
						{
							m_default = def;
							m_default_value = value;
							
							if (not value.empty() and not validate_value(m_default_value))
								throw exception("default value for attribute is not valid");
						}

	boost::tuple<AttributeDefault,wstring>
						get_default() const						{ return boost::make_tuple(m_default, m_default_value); }
	
  private:

	// routines used to check _and_ reformat attribute value strings
	bool				is_name(wstring& s);
	bool				is_names(wstring& s);
	bool				is_nmtoken(wstring& s);
	bool				is_nmtokens(wstring& s);

	wstring				m_name;
	AttributeType		m_type;
	AttributeDefault	m_default;
	wstring				m_default_value;
	vector<wstring>		m_enum;
};

bool doctype_attribute::is_name(wstring& s)
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

bool doctype_attribute::is_names(wstring& s)
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

bool doctype_attribute::is_nmtoken(wstring& s)
{
	bool result = true;

	ba::trim(s);
	
	wstring::iterator c = s.begin();
	while (result and ++c != s.end())
		result = is_name_char(*c);
	
	return result;
}

bool doctype_attribute::is_nmtokens(wstring& s)
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

bool doctype_attribute::validate_value(wstring& value)
{
	bool result = true;

	if (m_type == attTypeString)
		result = true;
	else if (m_type == attTypeTokenizedENTITY or m_type == attTypeTokenizedID or m_type == attTypeTokenizedIDREF)
		result = is_name(value);
	else if (m_type == attTypeTokenizedENTITIES or m_type == attTypeTokenizedIDREFS)
		result = is_names(value);
	else if (m_type == attTypeTokenizedNMTOKEN)
		result = is_nmtoken(value);
	else if (m_type == attTypeTokenizedNMTOKENS)
		result = is_nmtokens(value);
	else if (m_type == attTypeEnumerated)
	{
		ba::trim(value);
		result = find(m_enum.begin(), m_enum.end(), value) != m_enum.end();
	}
	
	return result;
}

class doctype_element : boost::noncopyable
{
  public:
						doctype_element(const wstring& name)
							: m_name(name) {}

						~doctype_element();

	void				add_attribute(doctype_attribute* attr);
	
	doctype_attribute*	get_attribute(const wstring& name);

	wstring				name() const								{ return m_name; }
	
	bool				operator<(const doctype_element& rhs)		{ return m_name < rhs.m_name; }
	
	const vector<doctype_attribute*>&
						attributes() const							{ return m_attlist; }

  private:
	wstring				m_name;
	vector<doctype_attribute*>
						m_attlist;
};

doctype_element::~doctype_element()
{
	for (vector<doctype_attribute*>::iterator attr = m_attlist.begin(); attr != m_attlist.end(); ++attr)
		delete *attr;
}

void doctype_element::add_attribute(doctype_attribute* attr)
{
	if (find_if(m_attlist.begin(), m_attlist.end(), boost::bind(&doctype_attribute::name, _1) == attr->name()) == m_attlist.end())
		m_attlist.push_back(attr);
	else
		delete attr;
}

doctype_attribute* doctype_element::get_attribute(const wstring& name)
{
	vector<doctype_attribute*>::iterator dta =
		find_if(m_attlist.begin(), m_attlist.end(), boost::bind(&doctype_attribute::name, _1) == name);
	
	doctype_attribute* result = nil;
	
	if (dta != m_attlist.end())
		result = *dta;
	
	return result;
}

// --------------------------------------------------------------------

struct parser_imp
{
					parser_imp(
						istream&	data,
						parser&		parser);
	
					~parser_imp();

	string			wstring_to_string(const wstring& s);

	// Here comes the parser part
	void			parse();

	// the productions. Some are inlined below for obvious reasons.	
	// names of the productions try to follow those in the TR http://www.w3.org/TR/xml
	void			prolog();
	void			xml_decl();
	void			text_decl();

	void			s(bool at_least_one = false)
					{
						if (at_least_one)
							match(xml_Space);
						
						while (m_lookahead == xml_Space)
							match(xml_Space);
					}

	void			eq()
					{
						s();
						match('=');
						s();
					}
					
	void			misc();
	void			element();
	void			content();
	void			doctypedecl();
	data_ptr		external_id();
	boost::tuple<fs::path,wstring>
					read_external_id();
	void			intsubset();
	void			extsubset();
	void			conditionalsect();
	void			ignoresectcontents();
	void			markup_decl();
	void			element_decl();
	void			contentspec(doctype_element& element);
	void			cp();
	void			attlist_decl();
	void			notation_decl();
	void			entity_decl();
	void			parameter_entity_decl();
	void			general_entity_decl();
	void			entity_value();

	// at several locations we need to parse out entity references from strings:	
	void			parse_parameter_entity_declaration(wstring& s);
	void			parse_general_entity_declaration(wstring& s);

	// same goes for attribute values
	wstring			normalize_attribute_value(const wstring& s)
					{
						data_ptr data(new wstring_data_source(s));
						return normalize_attribute_value(data);
					}
					
	wstring			normalize_attribute_value(data_ptr data);

	// The scanner is next. We recognize the following tokens:
	enum XMLToken
	{
		xml_Undef,
		xml_Eof = 256,

		// these are tokens for the markup
		
		xml_XMLDecl,	// <?xml
		xml_Space,		// Really needed
		xml_Comment,	// <!--(
		xml_Name,		// name-start-char (name-char)*
		xml_NMToken,	// (name-char)+
		xml_String,		// (\"[^"]*\") | (\'[^\']*\')		// single or double quoted string
		xml_PI,			// <?
		xml_STag,		// <
		xml_ETag,		// </
		xml_DocType,	// <!DOCTYPE
		xml_Element,	// <!ELEMENT
		xml_AttList,	// <!ATTLIST
		xml_Entity,		// <!ENTITY
		xml_Notation,	// <!NOTATION
		
		xml_IncludeIgnore,	// <![
		
		xml_PEReference,	// %name;
		
		// next are tokens for the content part
		
		xml_Reference,		// &name;
		xml_CDSect,			// CData section <![CDATA[ ... ]]>
		xml_Content,		// anything else up to the next element start
	};

	// for debugging and error reporting we have the following describing routine
	string			describe_token(int token)
					{
						string result;
						
						if (token > xml_Undef and token < xml_Eof)
						{
							stringstream s;
							
							if (isprint(token))
								s << '\'' << char(token) << '\'';
							else
								s << "&#x" << hex << token << ';';
							
							result = s.str();
						}
						else
						{
							switch (XMLToken(token))
							{
								case xml_Undef:			result = "undefined"; 					break;
								case xml_Eof:			result = "end of file"; 				break;
								case xml_XMLDecl:		result = "'<?xml'";	 					break;
								case xml_Space:			result = "space character";				break;
								case xml_Comment:		result = "comment";	 					break;
								case xml_Name:			result = "identifier or name";			break;
								case xml_NMToken:		result = "nmtoken";						break;
								case xml_String:		result = "quoted string";				break;
								case xml_PI:			result = "processing instruction";		break;
								case xml_STag:			result = "tag";			 				break;
								case xml_ETag:			result = "end tag";						break;
								case xml_DocType:		result = "<!DOCTYPE"; 					break;
								case xml_Element:		result = "<!ELEMENT"; 					break;
								case xml_AttList:		result = "<!ATTLIST"; 					break;
								case xml_Entity:		result = "<!ENTITY"; 					break;
								case xml_Notation:		result = "<!NOTATION"; 					break;
								case xml_PEReference:	result = "parameter entity reference";	break;
								case xml_Reference:		result = "entity reference"; 			break;
								case xml_CDSect:		result = "CDATA section";	 			break;
								case xml_Content:		result = "content";			 			break;
								
								case xml_IncludeIgnore:	result = "<![ (as in <![INCLUDE[ )";	break;
							}
						}
						
						return result;
					}

	wchar_t			get_next_char();
	
	// Recognizing tokens differs if we are expecting markup or content in elements:
	int				get_next_token();
	int				get_next_content();

	// retract is used when we've read a character too much from the input stream
	void			retract();
	
	// match, check if the look-a-head token is really what we expect here.
	// throws if it isn't. Depending on the content flag we call either get_next_token or get_next_content
	// to find the next look-a-head token.
	void			match(int token, bool content = false);

	// Sometimes we need to reuse our parser/scanner to parse an external entity e.g.
	// We use stack based state objects to store the current state.	
	struct parser_state
	{
						parser_state(parser_imp* imp, data_ptr source)
							: m_impl(imp)
							, m_lookahead(0)
							, m_data_source(source)
							, m_version(1.0f)
							, m_encoding(enc_UTF8)
							, m_external_subset(true)
						{
							swap(m_impl->m_lookahead,		m_lookahead);
							swap(m_impl->m_token, 			m_token);					
							swap(m_impl->m_data_source,		m_data_source);
							swap(m_impl->m_buffer,			m_buffer);
							swap(m_impl->m_version,			m_version);
							swap(m_impl->m_encoding,		m_encoding);
							swap(m_impl->m_external_subset,	m_external_subset);
						}
						
						~parser_state()
						{
							swap(m_impl->m_lookahead,		m_lookahead);
							swap(m_impl->m_token, 			m_token);					
							swap(m_impl->m_data_source,		m_data_source);
							swap(m_impl->m_buffer,			m_buffer);
							swap(m_impl->m_version,			m_version);
							swap(m_impl->m_encoding,		m_encoding);
							swap(m_impl->m_external_subset,	m_external_subset);
						}
		
		parser_imp*		m_impl;
		int				m_lookahead;
		data_ptr		m_data_source;
		stack<wchar_t>	m_buffer;
		wstring			m_token;
		float			m_version;
		Encoding		m_encoding;
		bool			m_external_subset;
	};

	// And during parsing we keep track of the namespaces we encounter.
	struct ns_state
	{
					ns_state(parser_imp* imp)
						: m_parser_imp(imp)
						, m_next(imp->m_ns)
					{
						m_parser_imp->m_ns = this;
					}

					~ns_state()
					{
						m_parser_imp->m_ns = m_next;
					}

		parser_imp*	m_parser_imp;
		wstring		m_default_ns;
		ns_state*	m_next;
		
		map<wstring,wstring>
					m_known;

		wstring		default_ns()
					{
						wstring result = m_default_ns;
						if (result.empty() and m_next != nil)
							result = m_next->default_ns();
						return result;
					}

		wstring		ns_for_prefix(const wstring& prefix)
					{
						wstring result;
						
						if (m_known.find(prefix) != m_known.end())
							result = m_known[prefix];
						else if (m_next != nil)
							result = m_next->ns_for_prefix(prefix);
						
						return result;
					}
	};
	
	int				m_lookahead;
	data_ptr		m_data_source;
	stack<wchar_t>	m_buffer;
	wstring			m_token;
	wstring			m_pi_target;
	float			m_version;
	Encoding		m_encoding;
	wstring			m_standalone;
	parser&			m_parser;
	ns_state*		m_ns;
	bool			m_in_doctype;			// used to keep track where we are (parameter entities are only recognized inside a doctype section)
	bool			m_external_subset;
	bool			m_in_element;
	bool			m_allow_parameter_entity_references;

	struct parsed_entity
	{
		fs::path	m_entity_path;
		wstring		m_entity_text;
	};

	typedef map<wstring,parsed_entity>		ParameterEntityMap;

	ParameterEntityMap	 m_parameter_entities;
	
	struct general_entity
	{
					general_entity()
						: m_external(true) {}
		
					general_entity(const wstring& text, bool external = false)
						: m_entity_text(text), m_external(external) {}
		
		wstring		m_entity_text;
		bool		m_external;
	};
	
	typedef map<wstring,general_entity>		EntityMap;

	EntityMap		m_general_entities;
	
	typedef map<wstring,doctype_element*>	DocTypeMap;
	
	DocTypeMap		m_doctype;
	set<wstring>	m_notations;
};

parser_imp::parser_imp(
	istream&		data,
	parser&			parser)
	: m_lookahead(xml_Eof)
	, m_data_source(new istream_data_source(data, data_ptr()))
	, m_version(1.0f)
	, m_encoding(enc_UTF8)
	, m_parser(parser)
	, m_ns(nil)
	, m_in_doctype(false)
	, m_external_subset(false)
	, m_in_element(false)
	, m_allow_parameter_entity_references(false)
{
	// these entities are always recognized:
	m_general_entities[L"lt"] = general_entity(L"&#60;");
	m_general_entities[L"gt"] = general_entity(L"&#62;");
	m_general_entities[L"amp"] = general_entity(L"&#38;");
	m_general_entities[L"apos"] = general_entity(L"&#39;");
	m_general_entities[L"quot"] = general_entity(L"&#34;");
}

parser_imp::~parser_imp()
{
	for (DocTypeMap::iterator de = m_doctype.begin(); de != m_doctype.end(); ++de)
		delete de->second;
}

// wstring_to_string is a very simplistic UCS4 to UTF-8 converter
string parser_imp::wstring_to_string(const wstring& s)
{
	string result;
	result.reserve(s.length());
	
	for (wstring::const_iterator ch = s.begin(); ch != s.end(); ++ch)
	{
		unsigned long cv = static_cast<unsigned long>(*ch);
		
		if (cv < 0x080)
			result += (static_cast<const char> (cv));
		else if (cv < 0x0800)
		{
			result += (static_cast<const char> (0x0c0 | (cv >> 6)));
			result += (static_cast<const char> (0x080 | (cv & 0x3f)));
		}
		else if (cv < 0x00010000)
		{
			result += (static_cast<const char> (0x0e0 | (cv >> 12)));
			result += (static_cast<const char> (0x080 | ((cv >> 6) & 0x3f)));
			result += (static_cast<const char> (0x080 | (cv & 0x3f)));
		}
		else
		{
			result += (static_cast<const char> (0x0f0 | (cv >> 18)));
			result += (static_cast<const char> (0x080 | ((cv >> 12) & 0x3f)));
			result += (static_cast<const char> (0x080 | ((cv >> 6) & 0x3f)));
			result += (static_cast<const char> (0x080 | (cv & 0x3f)));
		}
	}
	
	return result;
}

wchar_t parser_imp::get_next_char()
{
	wchar_t result = 0;

	if (not m_buffer.empty())
	{
		result = m_buffer.top();
		m_buffer.pop();
	}

	while (result == 0 and m_data_source != nil)
	{
		result = m_data_source->get_next_char();
		if (result == 0)
		{
			if (m_data_source->auto_discard())
				m_data_source = m_data_source->next_data_source();
			else
				break;
		}
	}
	
	if (result == 0x0ffff or result == 0x0fffe)
		throw exception("character ffff is not allowed in xml");

	// surrogate support
	if (result >= 0x0D800 and result <= 0x0DBFF)
	{
		wchar_t uc2 = get_next_char();
		if (uc2 >= 0x0DC00 and uc2 <= 0x0DFFF)
			result = (result - 0x0D800) * 0x400 + (uc2 - 0x0DC00) + 0x010000;
		else
			throw exception("leading surrogate character without trailing surrogate character");
	}
	else if (result >= 0x0DC00 and result <= 0x0DFFF)
		throw exception("trailing surrogate character without a leading surrogate");
	
	m_token += result;
	
	return result;
}

void parser_imp::retract()
{
	assert(not m_token.empty());
	
	wstring::iterator last_char = m_token.end() - 1;
	
	m_buffer.push(*last_char);
	m_token.erase(last_char);
}

void parser_imp::match(int token, bool content)
{
	if (m_lookahead != token)
	{
		string expected = describe_token(token);
		string found = describe_token(m_lookahead);
	
		throw exception("Error parsing XML, expected %s but found %s (%s)", expected.c_str(), found.c_str(),
			wstring_to_string(m_token).c_str());
	}
	
	if (content)
		m_lookahead = get_next_content();
	else
	{
		m_lookahead = get_next_token();
		
		// PEReferences can occur anywhere in a DTD and their
		// content must match the production extsubset;
		if (m_lookahead == xml_PEReference and m_allow_parameter_entity_references)
		{
			ParameterEntityMap::iterator r = m_parameter_entities.find(m_token);
			if (r == m_parameter_entities.end())
				throw exception("undefined parameter entity %s", m_token.c_str());
			
			m_data_source.reset(new parameter_entity_data_source(
				r->second.m_entity_text, r->second.m_entity_path, m_data_source));
			
			match(xml_PEReference);
		}
	}
}

/*
	get_next_token is a hand optimised scanner for tokens in the input stream.
*/

int parser_imp::get_next_token()
{
	enum State {
		state_Start				= 0,
		state_WhiteSpace		= 10,
		state_Tag				= 20,
		state_String			= 30,
		state_PERef				= 40,
		state_Name				= 50,
		state_CommentOrDoctype	= 60,
		state_Comment			= 70,
		state_DocTypeDecl		= 80,
		state_PI				= 90,
	};

	int token = xml_Undef;
	wchar_t quote_char;
	int state = state_Start;

	m_token.clear();
	
	while (token == xml_Undef)
	{
		wchar_t uc = get_next_char();
		
		switch (state)
		{
			// start scanning. 
			case state_Start:
				if (uc == 0)
					token = xml_Eof;
				else if (uc == ' ' or uc == '\t' or uc == '\n')
					state = state_WhiteSpace;
				else if (uc == '<')
					state = state_Tag;
				else if (uc == '\'' or uc == '\"')
				{
					state = state_String;
					quote_char = uc;
				}
				else if (uc == '%')
					state = state_PERef;
				else if (is_name_char(uc))
					state = state_Name;
				else
					token = uc;
				break;
			
			// collect all whitespace
			case state_WhiteSpace:
				if (uc != ' ' and uc != '\t' and uc != '\n')
				{
					retract();
					token = xml_Space;
				}
				break;
			
			// We scanned a < character, decide what to do next.
			case state_Tag:
				if (uc == '!')				// comment or doctype thing
					state = state_CommentOrDoctype;
				else if (uc == '/')			// end tag
					token = xml_ETag;
				else if (uc == '?')			// processing instruction
					state = state_PI;
				else						// anything else
				{
					retract();
					token = xml_STag;
				}
				break;
			
			// So we had <! which can only be followed validly by '-', '[' or a character at the current location
			case state_CommentOrDoctype:
				if (uc == '-')
					state = state_Comment;
				else if (uc == '[' and m_external_subset)
					token = xml_IncludeIgnore;
				else if (is_name_start_char(uc))
					state = state_DocTypeDecl;
				else
					throw exception("Unexpected character");
				break;
			
			// Comment, strictly check for <!-- -->
			case state_Comment:
				if (uc == '-')
					state += 1;
				else
					throw exception("Invalid formatted comment");
				break;
			
			case state_Comment + 1:
				if (uc == '-')
					state += 1;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away comment?");
				break;
			
			case state_Comment + 2:
				if (uc == '-')
					state += 1;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away comment?");
				else
					state -= 1;
				break;
			
			case state_Comment + 3:
				if (uc == '>')
					token = xml_Comment;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away comment?");
				else
					throw exception("Invalid comment");
				break;

			// scan for processing instructions
			case state_PI:
				if (uc == 0)
					throw exception("Unexpected end of file, run-away processing instruction?");
				else if (not is_name_char(uc))
				{
					retract();

					m_pi_target = m_token.substr(2);
					
					// we treat the xml processing instruction separately.
					if (m_pi_target == L"xml")
						token = xml_XMLDecl;
					else if (ba::to_lower_copy(m_pi_target) == L"xml")
						throw exception("<?XML is neither an XML declaration nor a legal processing instruction target");
					else
						state += 1;
				}
				break;
			
			case state_PI + 1:
				if (uc == '?')
					state += 1;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away processing instruction?");
				break;
			
			case state_PI + 2:
				if (uc == '>')
					token = xml_PI;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away processing instruction?");
				else
					state -= 1;
				break;

			// One of the DOCTYPE tags. We scanned <!(char), continue until non-char
			case state_DocTypeDecl:
				if (not is_name_char(uc))
				{
					retract();
					
					if (m_token == L"<!DOCTYPE")
						token = xml_DocType;
					else if (m_token == L"<!ELEMENT")
						token = xml_Element;
					else if (m_token == L"<!ATTLIST")
						token = xml_AttList;
					else if (m_token == L"<!ENTITY")
						token = xml_Entity;
					else if (m_token == L"<!NOTATION")
						token = xml_Notation;
					else
						throw exception("invalid doctype declaration %s", wstring_to_string(m_token).c_str());
				}
				break;

			// strings
			case state_String:
				if (uc == quote_char)
				{
					token = xml_String;
					m_token = m_token.substr(1, m_token.length() - 2);
				}
				else if (uc == 0)
					throw exception("unexpected end of file, runaway string");
				break;

			// Names
			case state_Name:
				if (not is_name_char(uc))
				{
					retract();
	
					if (is_name_start_char(m_token[0]))
						token = xml_Name;
					else
						token = xml_NMToken;
				}
				break;
			
			// parameter entity references
			case state_PERef:
				if (is_name_start_char(uc))
					state += 1;
				else
				{
					retract();
					token = '%';
				}
				break;
			
			case state_PERef + 1:
				if (uc == ';')
				{
					m_token = m_token.substr(1, m_token.length() - 2);
					token = xml_PEReference;
				}
				else if (not is_name_char(uc))
					throw exception("invalid parameter entity reference");
				break;
			
			default:
				assert(false);
				throw exception("state should never be reached");
		}
	}
	
	return token;
}

int parser_imp::get_next_content()
{
	enum State
	{
		state_Start			= 10,
		state_Tag			= 20,
		state_Reference		= 30,
		state_WhiteSpace	= 40,
		state_Content		= 50,
		state_PI			= 60,
		state_CommentOrCDATA
							= 70,
		state_Comment		= 80,
		state_CDATA			= 90,
		state_Illegal		= 100
	};

	int token = xml_Undef;
	int state = state_Start;
	wchar_t charref = 0;
	
	m_token.clear();

	while (token == xml_Undef)
	{
		wchar_t uc = get_next_char();
		
		switch (state)
		{
			case state_Start:
				if (uc == 0)
					token = xml_Eof;			// end of file reached
				else if (uc == '<')
					state = state_Tag;			// beginning of a tag
				else if (uc == '&')
					state = state_Reference;	// a &reference;
				else if (uc == ']')
					state = state_Illegal;		// avoid ]]> in text
				else if (is_char(uc))
					state = state_Content;		// anything else
				break;
			
			// content. Only stop collecting character when uc is special
			case state_Content:
				if (uc == ']')
					state = state_Illegal;
				else if (uc == 0 or uc == '<' or uc == '&')
				{
					retract();
					token = xml_Content;
				}
				else if (not is_char(uc))
					throw exception("Illegal character in content text");
				break;
			
			// beginning of a tag?
			case state_Tag:
				if (uc == '/')
					token = xml_ETag;
				else if (uc == '?')			// processing instruction
					state = state_PI;
				else if (uc == '!')			// comment or CDATA
					state = state_CommentOrCDATA;
				else
				{
					retract();
					token = xml_STag;
				}
				break;
			
			// processing instructions
			case state_PI:
				if (is_name_start_char(uc))
				{
					m_pi_target = uc;
					state += 1;
				}
				else
					throw exception("expected target in processing instruction");
				break;
			
			case state_PI + 1:
				if (is_name_char(uc))
					m_pi_target += uc;
				else if (uc == 0)
					throw exception("runaway processing instruction");
				else if (not is_char(uc))
					throw exception("Illegal character in content text");
				else if (uc == '?')
					state += 2;
				else
					state += 1;
				break;
			
			case state_PI + 2:
				if (uc == '?')
					state += 1;
				else if (uc == 0)
					throw exception("runaway processing instruction");
				else if (not is_char(uc))
					throw exception("Illegal character in content text");
				break;
			
			case state_PI + 3:
				if (uc == '>')
					token = xml_PI;
				else if (uc == 0)
					throw exception("runaway processing instruction");
				else if (not is_char(uc))
					throw exception("Illegal character in content text");
				else if (uc != '?')
					state = state_PI + 2;
				break;

			// comment or CDATA			
			case state_CommentOrCDATA:
				if (uc == '-')				// comment
					state = state_Comment;
				else if (uc == '[')
					state = state_CDATA;	// CDATA
				else
					throw exception("invalid content");
				break;

			case state_Comment:
				if (uc == '-')
					state += 1;
				else
					throw exception("invalid content");
				break;
			
			case state_Comment + 1:
				if (uc == '-')
					state += 1;
				else if (uc == 0)
					throw exception("runaway comment");
				else if (not is_char(uc))
					throw exception("Illegal character in content text");
				break;
			
			case state_Comment + 2:
				if (uc == '-')
					state += 1;
				else if (uc == 0)
					throw exception("runaway processing instruction");
				else if (not is_char(uc))
					throw exception("Illegal character in content text");
				else
					state -= 1;
				break;
			
			case state_Comment + 3:
				if (uc == '>')
					token = xml_Comment;
				else
					throw exception("invalid comment");
				break;

			// CDATA (we parsed <![ up to this location
			case state_CDATA:
				if (is_name_start_char(uc))
					state += 1;
				else
					throw exception("invalid content");
				break;
			
			case state_CDATA + 1:
				if (uc == '[' and m_token == L"<![CDATA[")
					state += 1;
				else if (not is_name_char(uc))
					throw exception("invalid content");
				break;

			case state_CDATA + 2:
				if (uc == ']')
					state += 1;
				else if (uc == 0)
					throw exception("runaway cdata section");
				break;
			
			case state_CDATA + 3:
				if (uc == ']')
					state += 1;
				else if (uc == 0)
					throw exception("runaway cdata section");
				else if (uc != ']')
					state = state_CDATA + 2;
				break;

			case state_CDATA + 4:
				if (uc == '>')
				{
					token = xml_CDSect;
					m_token = m_token.substr(9, m_token.length() - 12);
				}
				else if (uc == 0)
					throw exception("runaway cdata section");
				else if (uc != ']')
					state = state_CDATA + 2;
				break;

			// reference, either a character reference or a general entity reference
			case state_Reference:
				if (uc == '#')
					state = state_Reference + 2;
				else if (is_name_start_char(uc))
					state = state_Reference + 1;
				else
					throw exception("stray ampersand found in content");
				break;
			
			case state_Reference + 1:
				if (not is_name_char(uc))
				{
					if (uc != ';')
						throw exception("invalid entity found in content, missing semicolon?");
					token = xml_Reference;
					m_token = m_token.substr(1, m_token.length() - 2);
				}
				break;
			
			case state_Reference + 2:
				if (uc == 'x')
					state = state_Reference + 4;
				else if (uc >= '0' and uc <= '9')
				{
					charref = uc - '0';
					state += 1;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case state_Reference + 3:
				if (uc >= '0' and uc <= '9')
					charref = charref * 10 + (uc - '0');
				else if (uc == ';')
				{
					if (not is_char(charref))
						throw exception("Illegal character in content text");
					m_token = charref;
					token = xml_Content;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case state_Reference + 4:
				if (uc >= 'a' and uc <= 'f')
				{
					charref = uc - 'a' + 10;
					state += 1;
				}
				else if (uc >= 'A' and uc <= 'F')
				{
					charref = uc - 'A' + 10;
					state += 1;
				}
				else if (uc >= '0' and uc <= '9')
				{
					charref = uc - '0';
					state += 1;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case state_Reference + 5:
				if (uc >= 'a' and uc <= 'f')
					charref = (charref << 4) + (uc - 'a' + 10);
				else if (uc >= 'A' and uc <= 'F')
					charref = (charref << 4) + (uc - 'A' + 10);
				else if (uc >= '0' and uc <= '9')
					charref = (charref << 4) + (uc - '0');
				else if (uc == ';')
				{
					if (not is_char(charref))
						throw exception("Illegal character in content text");
					m_token = charref;
					token = xml_Content;
				}
				else
					throw exception("invalid character reference");
				break;
			
			// ]]> is illegal
			case state_Illegal:
				if (uc == ']')
					state += 1;
				else
				{
					retract();
					state = state_Content;
				}
				break;
					
			case state_Illegal + 1:
				if (uc == '>')
					throw exception("the sequence ']]>' is illegal in content text");
				else if (uc != ']')
				{
					retract();
					retract();
					state = state_Content;
				}
				break;

			default:
				assert(false);
				throw exception("state reached that should not be reachable");
		}
	}
	
	return token;
}

void parser_imp::parse()
{
	m_lookahead = get_next_token();
	
	prolog();
	element();
	misc();
	
	if (m_lookahead != xml_Eof)
		throw exception("garbage at end of file");
}

void parser_imp::prolog()
{
	xml_decl();
	
	misc();

	if (m_lookahead == xml_DocType)
	{
		doctypedecl();
		misc();
	}
}

void parser_imp::xml_decl()
{
	if (m_lookahead == xml_XMLDecl)
	{
		match(xml_XMLDecl);
	
		s(true);
		if (m_token != L"version")
			throw exception("expected a version attribute in XML declaration");
		match(xml_Name);
		eq();
		m_version = boost::lexical_cast<float>(m_token);
		if (m_version >= 2.0 or m_version < 1.0)
			throw exception("This library only supports XML version 1.x");
		match(xml_String);

		while (m_lookahead == xml_Space)
		{
			s(true);
			
			if (m_lookahead != xml_Name)
				break;

			if (m_token == L"encoding")
			{
				match(xml_Name);
				eq();
				ba::to_upper(m_token);
				if (m_token == L"UTF-8")
					m_encoding = enc_UTF8;
				else if (m_token == L"UTF-16")
				{
					if (m_encoding != enc_UTF16LE and m_encoding != enc_UTF16BE)
						throw exception("Inconsistent encoding attribute in XML declaration");
//						cerr << "Inconsistent encoding attribute in XML declaration" << endl;
					m_encoding = enc_UTF16BE;
				}
//				else if (m_token == L"ISO-8859-1")
//					m_encoding = enc_ISO88591;
				else
					throw exception("Unsupported encoding value %s", wstring_to_string(m_token).c_str());
				match(xml_String);
				continue;
			}
			
			if (m_token == L"standalone")
			{
				match(xml_Name);
				eq();
				if (m_token != L"yes" and m_token != L"no")
					throw exception("Invalid XML declaration, standalone value should be either yes or no");
				m_standalone = m_token;
				match(xml_String);
				continue;
			}
			
			throw exception("unexpected attribute in xml declaration");
		}
		
		match('?');
		match('>');
	}
}

void parser_imp::text_decl()
{
	if (m_lookahead == xml_XMLDecl)
	{
		match(xml_XMLDecl);
	
		while (m_lookahead == xml_Space)
		{
			s(true);
			
			if (m_lookahead != xml_Name)
				break;
			
			if (m_token == L"version")
			{
				match(xml_Name);
				eq();
				m_version = boost::lexical_cast<float>(m_token);
				if (m_version >= 2.0 or m_version < 1.0)
					throw exception("This library only supports XML version 1.x");
				match(xml_String);
				continue;
			}
			
			if (m_token == L"encoding")
			{
				match(xml_Name);
				eq();
				match(xml_String);
				continue;
			}
			
			throw exception("unexpected attribute in xml declaration");
		}
		
		match('?');
		match('>');
	}
}

void parser_imp::misc()
{
	for (;;)
	{
		if (m_lookahead == xml_Space or m_lookahead == xml_Comment)
		{
			match(m_lookahead);
			continue;
		}
		
		if (m_lookahead == xml_PI)
		{
			match(xml_PI);
			continue;
		}
		
		break;
	}	
}

void parser_imp::doctypedecl()
{
	m_in_doctype = true;
	m_allow_parameter_entity_references = true;

	match(xml_DocType);
	
	s(true);
	
	wstring name = m_token;
	match(xml_Name);

	data_ptr dtd;

	if (m_lookahead == xml_Space)
	{
		s(true);
		
		if (m_lookahead == xml_Name)
		{
			dtd = external_id();
			match(xml_String);
		}
		
		s();
	}
	
	if (m_lookahead == '[')
	{
		match('[');
		intsubset();
		match(']');

		s();
	}

	// internal subset takes precedence over external subset, so
	// if the external subset is defined, include it here.
	if (dtd)
	{
		// save the parser state
		parser_state save(this, dtd);
		
		m_data_source = dtd;
		
		match(m_lookahead);
		
		text_decl();
		
		extsubset();

		if (m_lookahead != xml_Eof)
			throw exception("Error parsing external dtd");
	}

	match('>');

	m_in_doctype = false;
	m_allow_parameter_entity_references = false;
}

void parser_imp::intsubset()
{
	for (;;)
	{
		switch (m_lookahead)
		{
			case xml_Element:
			case xml_AttList:
			case xml_Entity:
			case xml_Notation:
			case xml_PI:
			case xml_Comment:
				markup_decl();
				continue;
			
			case xml_Space:
				s();
				continue;
		}
		
		break;
	}
}

void parser_imp::extsubset()
{
	for (;;)
	{
		switch (m_lookahead)
		{
			case xml_Element:
			case xml_AttList:
			case xml_Entity:
			case xml_Notation:
			case xml_PI:
			case xml_Comment:
				markup_decl();
				continue;
			
			case xml_IncludeIgnore:
				conditionalsect();
				continue;
			
			case xml_Space:
				s();
				continue;
		}
		
		break;
	}
}

void parser_imp::conditionalsect()
{
	match(xml_IncludeIgnore);
	
	s();
	
	bool include;
	
	if (m_token == L"INCLUDE")
		include = true;
	else if (m_token == L"IGNORE")
		include = false;
	else if (m_lookahead == xml_Name)
		throw exception("Unexpected literal %s", wstring_to_string(m_token).c_str());
	
	match(xml_Name);
	s();
		
	if (include)
	{
		match('[');
		extsubset();
		match(']');
		match (']');
		match ('>');
	}
	else
	{
		ignoresectcontents();
		m_lookahead = get_next_token();
	}
}

void parser_imp::ignoresectcontents()
{
	// yet another tricky routine, skip 
	
	int state = 0;
	bool done = false;
	
	while (not done)
	{
		wchar_t ch = get_next_char();
		if (ch == 0)
			throw exception("runaway IGNORE section");
		
		switch (state)
		{
			case 0:
				if (ch == ']')
					state = 1;
				else if (ch == '<')
					state = 10;
				break;
			
			case 1:
				if (ch == ']')
					state = 2;
				else
				{
					retract();
					state = 0;
				}
				break;
			
			case 2:
				if (ch == '>')
					done = true;
				else if (ch != ']')
				{
					retract();
					state = 0;
				}
				break;
			
			case 10:
				if (ch == '!')
					state = 11;
				else
				{
					retract();
					state = 0;
				}
				break;
			
			case 11:
				if (ch == '[')
				{
					ignoresectcontents();
					state = 0;
				}
				else
				{
					retract();
					state = 0;	
				}
				break;
		}
	}
}

void parser_imp::markup_decl()
{
	m_allow_parameter_entity_references = not m_external_subset;
	
	switch (m_lookahead)
	{
		case xml_Element:
			element_decl();
			break;
		
		case xml_AttList:
			attlist_decl();
			break;
		
		case xml_Entity:
			entity_decl();
			break;

		case xml_Notation:
			notation_decl();
			break;
		
		case xml_PI:
			match(xml_PI);
			break;

		case xml_Comment:
			if (m_parser.comment_handler)
				m_parser.comment_handler(wstring_to_string(m_token));
			match(xml_Comment);
			break;

		default:
			throw exception("unexpected token %s", describe_token(m_lookahead).c_str());
	}
	
	m_allow_parameter_entity_references = true;
}

void parser_imp::element_decl()
{
	match(xml_Element);
	s(true);

	wstring name = m_token;
	auto_ptr<doctype_element> element(new doctype_element(name));

	match(xml_Name);
	s(true);
	contentspec(*element);
	s();
	match('>');
	
	if (m_doctype.find(name) == m_doctype.end())
		m_doctype[name] = element.release();
}

void parser_imp::contentspec(doctype_element& element)
{
	if (m_lookahead == xml_Name)
	{
		if (m_token != L"EMPTY" and m_token != L"ANY")
			throw exception("Invalid element content specification");
		match(xml_Name);
	}
	else
	{
		match('(');
		
		s();
		
		bool mixed = false;

		if (m_lookahead == '#')	// Mixed
		{
			mixed = true;
			
			match(m_lookahead);
			if (m_token != L"PCDATA")
				throw exception("Invalid element content specification, expected #PCDATA");
			match(xml_Name);
			
			s();
			
			while (m_lookahead == '|')
			{
				match('|');
				s();
				match(xml_Name);
				s();
			}
		}
		else					// children
		{
			cp();
			s();
			if (m_lookahead == ',')
			{
				do
				{
					match(m_lookahead);
					s();
					cp();
					s();
				}
				while (m_lookahead == ',');
			}
			else if (m_lookahead == '|')
			{
				do
				{
					match(m_lookahead);
					s();
					cp();
					s();
				}
				while (m_lookahead == '|');
			}
		}

		s();
		match(')');
		
		if (m_lookahead == '*' or (not mixed and (m_lookahead == '+' or m_lookahead == '?')))
			match(m_lookahead);
	}
}

void parser_imp::cp()
{
	if (m_lookahead == '(')
	{
		match('(');
		
		s();
		cp();
		s();
		if (m_lookahead == ',')
		{
			do
			{
				match(m_lookahead);
				s();
				cp();
				s();
			}
			while (m_lookahead == ',');
		}
		else if (m_lookahead == '|')
		{
			do
			{
				match(m_lookahead);
				s();
				cp();
				s();
			}
			while (m_lookahead == '|');
		}

		s();
		match(')');
	}
	else
	{
		wstring name = m_token;
		match(xml_Name);
	}
	
	if (m_lookahead == '*' or m_lookahead == '+' or m_lookahead == '?')
		match(m_lookahead);
}

void parser_imp::entity_decl()
{
	match(xml_Entity);
	s(true);

	if (m_lookahead == '%')	// PEDecl
		parameter_entity_decl();
	else
		general_entity_decl();
}

void parser_imp::parameter_entity_decl()
{
	match('%');
	s(true);
	
	wstring name = m_token;
	match(xml_Name);
	
	s(true);

	fs::path path;
	wstring value;
	
	// PEDef is either a EntityValue...
	if (m_lookahead == xml_String)
	{
		value = m_token;
		match(xml_String);
		parse_parameter_entity_declaration(value);
	}
	else	// ... or an external id 
	{
		boost::tie(path, value) = read_external_id();
		match(xml_String);
	}

	s();
	
	match('>');
	
	if (m_parameter_entities.find(name) == m_parameter_entities.end())
	{
		parsed_entity pe = { path, value };
		
		m_parameter_entities[name] = pe;
	}
}

void parser_imp::general_entity_decl()
{
	wstring name = m_token;
	match(xml_Name);
	s(true);
	
	fs::path path; // not used
	wstring value;
	bool external = false;

	if (m_lookahead == xml_String)
	{
		value = m_token;
		match(xml_String);
	
		parse_general_entity_declaration(value);
	}
	else // ... or an ExternalID
	{
		boost::tie(path, value) = read_external_id();
		match(xml_String);
		external = true;

		if (m_lookahead == xml_Space)
		{
			s(true);
			if (m_lookahead == xml_Name and m_token == L"NDATA")
			{
				if (m_notations.count(m_token) == 0)
					throw exception("Undefined NOTATION %s", wstring_to_string(m_token).c_str());
				
				match(xml_Name);
				s(true);
				match(xml_Name);
			}
		}
	}	
	
	s();
	
	match('>');
	
	if (m_general_entities.find(name) == m_general_entities.end())
		m_general_entities[name] = general_entity(value, external);
}

void parser_imp::attlist_decl()
{
	match(xml_AttList);
	s(true);
	wstring element = m_token;
	match(xml_Name);
	
	if (m_doctype.find(element) == m_doctype.end())
	{
//		if (VERBOSE)
//			cerr << "ATTLIST declaration for an undefined ELEMENT " << wstring_to_string(element) << endl;
		m_doctype[element] = new doctype_element(element);
	}
	
	doctype_element* e = m_doctype[element];
	
	while (m_lookahead == xml_Space)
	{
		s(true);
		
		if (m_lookahead != xml_Name)
			break;
	
		wstring name = m_token;
		match(xml_Name);
		s(true);
		
		auto_ptr<doctype_attribute> attribute;
		
		// att type: several possibilities:
		if (m_lookahead == '(')	// enumeration
		{
			vector<wstring> enums;
			
			match(m_lookahead);
			
			s();
			
			enums.push_back(m_token);
			if (m_lookahead == xml_Name)
				match(xml_Name);
			else
				match(xml_NMToken);

			s();
			
			while (m_lookahead == '|')
			{
				match('|');

				s();

				enums.push_back(m_token);
				if (m_lookahead == xml_Name)
					match(xml_Name);
				else
					match(xml_NMToken);

				s();
			}

			s();
			
			match(')');
			
			attribute.reset(new doctype_attribute(name, enums));
		}
		else
		{
			wstring type = m_token;
			match(xml_Name);
			
			vector<wstring> notations;
			
			if (type == L"CDATA")
				attribute.reset(new doctype_attribute(name, attTypeString));
			else if (type == L"ID")
				attribute.reset(new doctype_attribute(name, attTypeTokenizedID));
			else if (type == L"IDREF")
				attribute.reset(new doctype_attribute(name, attTypeTokenizedIDREF));
			else if (type == L"IDREFS")
				attribute.reset(new doctype_attribute(name, attTypeTokenizedIDREFS));
			else if (type == L"ENTITY")
				attribute.reset(new doctype_attribute(name, attTypeTokenizedENTITY));
			else if (type == L"ENTITIES")
				attribute.reset(new doctype_attribute(name, attTypeTokenizedENTITIES));
			else if (type == L"NMTOKEN")
				attribute.reset(new doctype_attribute(name, attTypeTokenizedNMTOKEN));
			else if (type == L"NMTOKENS")
				attribute.reset(new doctype_attribute(name, attTypeTokenizedNMTOKENS));
			else if (type == L"NOTATION")
			{
				s(true);
				match('(');
				s();
				
				notations.push_back(m_token);
				match(xml_Name);
				
				s();

				while (m_lookahead == '|')
				{
					match('|');
	
					s();
	
					notations.push_back(m_token);
					match(xml_Name);
	
					s();
				}
	
				s();
				
				match(')');
				
				attribute.reset(new doctype_attribute(name, notations));
			}
			else
				throw exception("invalid attribute type");
		}
		
		// att def

		if (m_lookahead != '>')
		{
			s(true);
			
			if (m_lookahead == '#')
			{
				match(m_lookahead);
				wstring def = m_token;
				match(xml_Name);
	
				if (def == L"REQUIRED")
					attribute->set_default(attDefRequired, L"");
				else if (def == L"IMPLIED")
					attribute->set_default(attDefImplied, L"");
				else if (def == L"FIXED")
				{
					s();
					
					attribute->set_default(attDefFixed, normalize_attribute_value(m_token));
					match(xml_String);
				}
				else
					throw exception("invalid attribute default");
			}
			else
			{
				attribute->set_default(attDefNone, normalize_attribute_value(m_token));
				match(xml_String);
			}
		}
		
		e->add_attribute(attribute.release());
	}

	match('>');
}

void parser_imp::notation_decl()
{
	match(xml_Notation);
	s(true);
	
	if (m_notations.count(m_token) > 0)
		throw exception("notation names should be unique");
	m_notations.insert(m_token);
	
	match(xml_Name);
	s(true);

	wstring pubid, system;
	
	if (m_token == L"SYSTEM")
	{
		match(xml_Name);
		s(true);
		
		system = m_token;
		match(xml_String);
	}
	else if (m_token == L"PUBLIC")
	{
		match(xml_Name);
		s(true);
		
		pubid = m_token;
		match(xml_String);
		
		// validate the public ID
		if (pubid.find_first_not_of(L" \r\nabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVSXYZ0123456789-'()+,./:=?;!*#@$_%") != string::npos)
			throw exception("Invalid public ID");
		
		s();
		
		if (m_lookahead == xml_String)
		{
			system = m_token;
			match(xml_String);
		}
	}
	else
		throw exception("Expected either SYSTEM or PUBLIC");

	s();
	match('>');
}

data_ptr parser_imp::external_id()
{
	data_ptr result;
	wstring pubid, system;
	
	if (m_token == L"SYSTEM")
	{
		match(xml_Name);
		s(true);
		
		system = m_token;
	}
	else if (m_token == L"PUBLIC")
	{
		match(xml_Name);
		s(true);
		
		pubid = m_token;
		match(xml_String);

		// validate the public ID
		if (pubid.find_first_not_of(L" \r\nabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVSXYZ0123456789-'()+,./:=?;!*#@$_%") != string::npos)
			throw exception("Invalid public ID");
		
		s(true);
		system = m_token;
	}
	else
		throw exception("Expected external id starting with either SYSTEM or PUBLIC");

	if (not system.empty())
	{
		auto_ptr<istream> is;
		fs::path path;
		
		// first allow the client to retrieve the dtd
		is.reset(m_parser.find_external_dtd(pubid, system));
		
		// if that fails, we try it ourselves
		if (is.get() == nil)
		{
			path = fs::system_complete(m_data_source->base_dir() / wstring_to_string(system));
	
			if (fs::exists(path))
				is.reset(new fs::ifstream(path));
		}
		
		if (is.get() != nil)
		{
			result.reset(new istream_data_source(is));
			
			if (fs::exists(path) and fs::exists(path.branch_path()))
				result->base_dir(path.branch_path());
		}
	}

	return result;
}

boost::tuple<fs::path,wstring> parser_imp::read_external_id()
{
	wstring result;
	fs::path path;

	parser_state save(this, external_id());
	
	if (m_data_source)
	{
		path = m_data_source->base_dir();
		
		m_lookahead = get_next_token();

		text_decl();
		
		result = m_token;
	
		while (wchar_t ch = get_next_char())
			result += ch;
	}
	
	return boost::make_tuple(path, result);
}

void parser_imp::parse_parameter_entity_declaration(wstring& s)
{
	wstring result;
	
	int state = 0;
	wchar_t charref = 0;
	wstring name;
	
	for (wstring::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		wchar_t c = *i;
		
		switch (state)
		{
			case 0:
				if (c == '&')
					state = 1;
				else if (c == '%')
				{
					if (m_external_subset)
					{
						name.clear();
						state = 20;
					}
					else
						throw exception("parameter entities may not occur in declarations that are not in an external subset");
				}
				else
					result += c;
				break;
			
			case 1:
				if (c == '#')
					state = 2;
				else
				{
					result += '&';
					result += c;
					state = 0;
				}
				break;

			case 2:
				if (c == 'x')
					state = 4;
				else if (c >= '0' and c <= '9')
				{
					charref = c - '0';
					state = 3;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 3:
				if (c >= '0' and c <= '9')
					charref = charref * 10 + (c - '0');
				else if (c == ';')
				{
					result += charref;
					state = 0;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 4:
				if (c >= 'a' and c <= 'f')
				{
					charref = c - 'a' + 10;
					state = 5;
				}
				else if (c >= 'A' and c <= 'F')
				{
					charref = c - 'A' + 10;
					state = 5;
				}
				else if (c >= '0' and c <= '9')
				{
					charref = c - '0';
					state = 5;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 5:
				if (c >= 'a' and c <= 'f')
					charref = (charref << 4) + (c - 'a' + 10);
				else if (c >= 'A' and c <= 'F')
					charref = (charref << 4) + (c - 'A' + 10);
				else if (c >= '0' and c <= '9')
					charref = (charref << 4) + (c - '0');
				else if (c == ';')
				{
					result += charref;
					state = 0;
				}
				else
					throw exception("invalid character reference");
				break;

			case 20:
				if (c == ';')
				{
					ParameterEntityMap::iterator e = m_parameter_entities.find(name);
					if (e == m_parameter_entities.end())
						throw exception("undefined parameter entity reference %s", wstring_to_string(name).c_str());
					result += e->second.m_entity_text;
					state = 0;
				}
				else if (is_name_char(c))
					name += c;
				else
					throw exception("invalid parameter entity reference");
				break;
			
			default:
				assert(false);
				throw exception("invalid state");
		}
	}
	
	if (state != 0)
		throw exception("invalid reference");
	
	swap(s, result);
}

// parse out the general and parameter entity references in a value string
// for a general entity reference which is about to be stored.
void parser_imp::parse_general_entity_declaration(wstring& s)
{
	wstring result;
	
	int state = 0;
	wchar_t charref = 0;
	wstring name;
	
	for (wstring::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		wchar_t c = *i;
		
		switch (state)
		{
			case 0:
				if (c == '&')
					state = 1;
				else if (c == '%')
				{
					if (m_external_subset)
					{
						name.clear();
						state = 20;
					}
					else
						throw exception("parameter entities may not occur in declarations that are not in an external subset");
				}
				else
					result += c;
				break;
			
			case 1:
				if (c == '#')
					state = 2;
				else if (is_name_start_char(c))
				{
					name.assign(&c, 1);
					state = 10;
				}
				break;

			case 2:
				if (c == 'x')
					state = 4;
				else if (c >= '0' and c <= '9')
				{
					charref = c - '0';
					state = 3;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 3:
				if (c >= '0' and c <= '9')
					charref = charref * 10 + (c - '0');
				else if (c == ';')
				{
					result += charref;
					state = 0;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 4:
				if (c >= 'a' and c <= 'f')
				{
					charref = c - 'a' + 10;
					state = 5;
				}
				else if (c >= 'A' and c <= 'F')
				{
					charref = c - 'A' + 10;
					state = 5;
				}
				else if (c >= '0' and c <= '9')
				{
					charref = c - '0';
					state = 5;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 5:
				if (c >= 'a' and c <= 'f')
					charref = (charref << 4) + (c - 'a' + 10);
				else if (c >= 'A' and c <= 'F')
					charref = (charref << 4) + (c - 'A' + 10);
				else if (c >= '0' and c <= '9')
					charref = (charref << 4) + (c - '0');
				else if (c == ';')
				{
					result += charref;
					state = 0;
				}
				else
					throw exception("invalid character reference");
				break;

			case 10:
				if (c == ';')
				{
					result += '&';
					result += name;
					result += ';';

					state = 0;
				}
				else if (is_name_char(c))
					name += c;
				else
					throw exception("invalid entity reference");
				break;

			case 20:
				if (c == ';')
				{
					ParameterEntityMap::iterator e = m_parameter_entities.find(name);
					if (e == m_parameter_entities.end())
						throw exception("undefined parameter entity reference %s", wstring_to_string(name).c_str());
					result += e->second.m_entity_text;
					state = 0;
				}
				else if (is_name_char(c))
					name += c;
				else
					throw exception("invalid parameter entity reference");
				break;
			
			default:
				assert(false);
				throw exception("invalid state");
		}
	}
	
	if (state != 0)
		throw exception("invalid reference");
	
	swap(s, result);
}

wstring parser_imp::normalize_attribute_value(data_ptr data)
{
	wstring result;
	
	int state = 0;
	wchar_t charref = 0;
	wstring name;
	
	for (;;)
	{
		wchar_t c = data->get_next_char();
		
		if (c == 0)
			break;
		
		if (c == '<')
			throw exception("Attribute values may not contain '<' character");
		
		switch (state)
		{
			case 0:
				if (c == '&')
					state = 1;
				else if (c == ' ' or c == '\n' or c == '\t' or c == '\r')
					result += ' ';
				else
					result += c;
				break;
			
			case 1:
				if (c == '#')
					state = 2;
				else if (is_name_start_char(c))
				{
					name.assign(&c, 1);
					state = 10;
				}
				break;

			case 2:
				if (c == 'x')
					state = 4;
				else if (c >= '0' and c <= '9')
				{
					charref = c - '0';
					state = 3;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 3:
				if (c >= '0' and c <= '9')
					charref = charref * 10 + (c - '0');
				else if (c == ';')
				{
//					if (charref == '<')
//						throw exception("Attribute values may not contain '<' character");
					result += charref;
					state = 0;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 4:
				if (c >= 'a' and c <= 'f')
				{
					charref = c - 'a' + 10;
					state = 5;
				}
				else if (c >= 'A' and c <= 'F')
				{
					charref = c - 'A' + 10;
					state = 5;
				}
				else if (c >= '0' and c <= '9')
				{
					charref = c - '0';
					state = 5;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 5:
				if (c >= 'a' and c <= 'f')
					charref = (charref << 4) + (c - 'a' + 10);
				else if (c >= 'A' and c <= 'F')
					charref = (charref << 4) + (c - 'A' + 10);
				else if (c >= '0' and c <= '9')
					charref = (charref << 4) + (c - '0');
				else if (c == ';')
				{
//					if (charref == '<')
//						throw exception("Attribute values may not contain '<' character");
					result += charref;
					state = 0;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 10:
				if (c == ';')
				{
					if (data->is_entity_on_stack(name))
						throw exception("infinite recursion in nested entity references");
					
					EntityMap::iterator e = m_general_entities.find(name);
					if (e == m_general_entities.end())
						throw exception("undefined entity reference %s", wstring_to_string(name).c_str());
					
					if (e->second.m_external)
						throw exception("attribute value may not contain external entity reference");
					
					data_ptr next_data(new entity_data_source(name, m_data_source->base_dir(), e->second.m_entity_text, data));
					wstring replacement = normalize_attribute_value(next_data);
					result += replacement;

					state = 0;
				}
				else if (is_name_char(c))
					name += c;
				else
					throw exception("invalid entity reference");
				break;

			default:
				assert(false);
				throw exception("invalid state");
		}
	}
	
	if (state != 0)
		throw exception("invalid reference");
	
	return result;
}

void parser_imp::element()
{
	bool saved_in_element = m_in_element;
	m_in_element = true;
	
	match(xml_STag);
	wstring name = m_token;
	match(xml_Name);
	
	doctype_element* dte = m_doctype[name];
	
	list<pair<wstring,wstring> > attrs;
	
	ns_state ns(this);
	set<wstring> seen;
	
	for (;;)
	{
		if (m_lookahead != xml_Space)
			break;
		
		s(true);
		
		if (m_lookahead != xml_Name)
			break;
		
		wstring attr_name = m_token;
		match(xml_Name);
		
		if (seen.count(attr_name) > 0)
			throw exception("multiple values for attribute %s", wstring_to_string(attr_name).c_str());
		seen.insert(attr_name);
		
		eq();

		wstring attr_value = normalize_attribute_value(m_token);
		match(xml_String);
		
		if (attr_name == L"xmlns" or ba::starts_with(attr_name, L"xmlns:"))	// namespace support
		{
			if (attr_name.length() == 5)
			{
				ns.m_default_ns = attr_value;
				m_parser.start_namespace_decl(L"", attr_value);
			}
			else
			{
				wstring prefix = attr_name.substr(6);
				ns.m_known[prefix] = attr_value;
				m_parser.start_namespace_decl(prefix, attr_value);
			}
		}
		else
		{
			if (dte != nil)
			{
				doctype_attribute* dta = dte->get_attribute(attr_name);
				if (dta != nil and not dta->validate_value(attr_value))
					throw exception("invalid value for attribute");
			}
			
			attrs.push_back(make_pair(attr_name, attr_value));
		}
	}
	
	// add missing attributes
	if (dte != nil)
	{
		const vector<doctype_attribute*>& dtattrs = dte->attributes();
		
		for (vector<doctype_attribute*>::const_iterator a = dtattrs.begin(); a != dtattrs.end(); ++a)
		{
			const doctype_attribute* dta = *a;
			
			wstring name = dta->name();
			
			list<pair<wstring,wstring> >::iterator attr = find_if(attrs.begin(), attrs.end(),
				boost::bind(&pair<wstring,wstring>::first, _1) == name);
			
			AttributeDefault defType;
			wstring defValue;
			
			boost::tie(defType, defValue) = dta->get_default();
			
			if (defType == attDefRequired)
			{
//				if (attr == attrs.end() and VERBOSE)
//					cerr << "missing required attribute" << endl;
			}
			else if (not defValue.empty() and attr == attrs.end())
			{
				wstring attr_value = normalize_attribute_value(defValue);
				attrs.push_back(make_pair(name, attr_value));
			}
		}
	}
	
	// now find out the namespace we're supposed to pass
	wstring uri, raw(name);
	list<wstring> qname;
	ba::split(qname, name, ba::is_any_of(L":"));
	if (qname.size() == 2)
	{
		uri = ns.ns_for_prefix(qname.front());
		name = qname.back();
	}
	else
		uri = ns.default_ns();

	if (m_lookahead == '/')
	{
		match('/');
		m_parser.start_element(name, uri, attrs);
		m_parser.end_element(name, uri);
	}
	else
	{
		m_parser.start_element(name, uri, attrs);
		
		match('>', true);

		if (m_lookahead != xml_ETag)
			content();
		
		match(xml_ETag);
		
		if (m_token != raw)
			throw exception("end tag does not match start tag");
		
		match(xml_Name);

		s();
		
		m_parser.end_element(name, uri);
	}
	
	m_in_element = saved_in_element;
	match('>', m_in_element);
	
	s();
}

void parser_imp::content()
{
	wstring data;
	
	do
	{
		switch (m_lookahead)
		{
			case xml_Content:
				m_parser.character_data(m_token);
				match(xml_Content, true);
				break;
			
			case xml_Reference:
			{
				EntityMap::iterator e = m_general_entities.find(m_token);
				if (e == m_general_entities.end())
					throw exception("undefined entity reference %s", wstring_to_string(m_token).c_str());
				
				if (m_data_source->is_entity_on_stack(m_token))
					throw exception("infinite recursion of entity references");
				
				// scope
				{
					data_ptr source(new entity_data_source(m_token, m_data_source->base_dir(), e->second.m_entity_text, m_data_source));
				
					parser_state state(this, source);
					
					m_lookahead = get_next_content();
					
					if (m_lookahead != xml_Eof)
						content();

					if (m_lookahead != xml_Eof)
						throw exception("entity reference should be a valid content production");
				}
				
				match(xml_Reference, true);
				break;
			}
			
			case xml_STag:
				element();
				break;
			
			case xml_PI:
				if (m_pi_target == L"xml")	// <?xml declaration, not allowed here
					throw exception("<?xml declaration not allowed in content");
				else if (ba::to_lower_copy(m_pi_target) == L"xml")
					throw exception("<?XML is neither an XML declaration nor a legal processing instruction target");
				else
					m_parser.processing_instruction(m_pi_target, m_token);
				match(xml_PI, true);
				break;
			
			case xml_Comment:
				m_parser.comment(m_token);
				match(xml_Comment, true);
				break;
			
			case xml_CDSect:
				m_parser.start_cdata_section();
				m_parser.character_data(m_token);
				m_parser.end_cdata_section();
				
				match(xml_CDSect, true);
				break;

			default:
				throw exception("unexpected token %s", describe_token(m_lookahead).c_str());

		}
	}
	while (m_lookahead != xml_ETag and m_lookahead != xml_Eof);
}

// --------------------------------------------------------------------

basic_parser_base::~basic_parser_base()
{
	delete m_impl;
	delete m_istream;
}

string basic_parser_base::wstring_to_string(const wstring& s)
{
	return m_impl->wstring_to_string(s);
}

template<>
basic_parser<char>::basic_parser(istream& data)
	: basic_parser_base(new parser_imp(data, *this), nil)
	, m_traits(*this)
{
}

template<>
basic_parser<char>::basic_parser(const string& data)
	: basic_parser_base(nil, nil)
	, m_traits(*this)
{
	m_istream = new istringstream(data);
	m_impl = new parser_imp(*m_istream, *this);
}

template<>
basic_parser<char>::~basic_parser()
{
}

template<>
void basic_parser<char>::parse()
{
	m_impl->parse();
}

}
}
