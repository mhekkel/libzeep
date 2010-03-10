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

#include "zeep/xml/document.hpp"
#include "zeep/exception.hpp"

#include "zeep/xml/parser.hpp"

using namespace std;
namespace ba = boost::algorithm;

extern int VERBOSE;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

namespace
{

string wstring_to_string(const wstring& s)
{
	string result;
	result.reserve(s.length());
	
	for (wstring::const_iterator ch = s.begin(); ch != s.end(); ++ch)
	{
		if (*ch < 0x080)
			result += (static_cast<const char> (*ch));
		else if (*ch < 0x0800)
		{
			result += (static_cast<const char> (0x0c0 | (*ch >> 6)));
			result += (static_cast<const char> (0x080 | (*ch & 0x3f)));
		}
		else if (*ch < 0x00010000)
		{
			result += (static_cast<const char> (0x0e0 | (*ch >> 12)));
			result += (static_cast<const char> (0x080 | ((*ch >> 6) & 0x3f)));
			result += (static_cast<const char> (0x080 | (*ch & 0x3f)));
		}
		else
		{
			result += (static_cast<const char> (0x0f0 | (*ch >> 18)));
			result += (static_cast<const char> (0x080 | ((*ch >> 12) & 0x3f)));
			result += (static_cast<const char> (0x080 | ((*ch >> 6) & 0x3f)));
			result += (static_cast<const char> (0x080 | (*ch & 0x3f)));
		}
	}
	
	return result;
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

class data_source
{
  public:
	virtual			~data_source() {}

	virtual wchar_t	get_next_char() = 0;
};

typedef boost::shared_ptr<data_source>	data_ptr;

// --------------------------------------------------------------------

class istream_data_source : public data_source
{
  public:
					istream_data_source(
						istream&		data)
						: m_data(data)
						, m_encoding(enc_UTF8)
						, m_has_bom(false)
					{
					}

	Encoding		guess_encoding();
	
	bool			has_bom()				{ return m_has_bom; }

  private:

	virtual wchar_t	get_next_char();

	wchar_t			next_utf8_char();
	
	wchar_t			next_utf16le_char();
	
	wchar_t			next_utf16be_char();
	
	wchar_t			next_iso88591_char();

	char			next_byte();

	istream&		m_data;
	stack<char>		m_byte_buffer;
	Encoding		m_encoding;

	boost::function<wchar_t(void)>
					m_next;
	bool			m_has_bom;
	bool			m_valid_utf8;
};

Encoding istream_data_source::guess_encoding()
{
	// 1. easy first step, see if there is a BOM
	
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
	
	return m_encoding;
}

char istream_data_source::next_byte()
{
	char result;
	
	if (m_byte_buffer.empty())
		m_data.read(&result, 1);
	else
	{
		result = m_byte_buffer.top();
		m_byte_buffer.pop();
	}

	return result;
}

wchar_t istream_data_source::next_utf8_char()
{
	wchar_t result = 0;
	char ch[5];
	
	ch[0] = next_byte();
	
	if ((ch[0] & 0x080) == 0)
		result = ch[0];
	else if ((ch[0] & 0x0E0) == 0x0C0)
	{
		ch[1] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<wchar_t>(((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F));
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<wchar_t>(((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F));
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = static_cast<wchar_t>(((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F));
	}
	
	if (m_data.eof())
		result = 0;
	
	return result;
}

wchar_t istream_data_source::next_utf16le_char()
{
	char c1 = next_byte(), c2 = next_byte();
	
	wchar_t result = (wchar_t(c2) << 16) | c1;

	return result;
}

wchar_t istream_data_source::next_utf16be_char()
{
	char c1 = next_byte(), c2 = next_byte();
	
	wchar_t result = (wchar_t(c1) << 16) | c2;

	return result;
}

wchar_t istream_data_source::next_iso88591_char()
{
	throw exception("to be implemented");
	return 0;
}

wchar_t istream_data_source::get_next_char()
{
	return m_next();
}

// --------------------------------------------------------------------

class wstring_data_source : public data_source
{
  public:
					wstring_data_source(
						const wstring&	data)
						: m_data(data)
						, m_ptr(m_data.begin())
					{
					}

  private:

	virtual wchar_t	get_next_char();

	char			next_byte();
					
	const wstring	m_data;
	wstring::const_iterator
					m_ptr;
};

char wstring_data_source::next_byte()
{
	char result = 0;
	if (m_ptr != m_data.end())
	{
		result = *m_ptr;
		++m_ptr;
	}
	return result;
}

wchar_t	wstring_data_source::get_next_char()
{
	wchar_t result = 0;
	if (m_ptr != m_data.end())
	{
		char ch[4];
		
		ch[0] = next_byte();
		
		if ((ch[0] & 0x080) == 0)
			result = ch[0];
		else if ((ch[0] & 0x0E0) == 0x0C0)
		{
			ch[1] = next_byte();
			if ((ch[1] & 0x0c0) != 0x080)
				throw exception("Invalid utf-8");
			result = static_cast<wchar_t>(((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F));
		}
		else if ((ch[0] & 0x0F0) == 0x0E0)
		{
			ch[1] = next_byte();
			ch[2] = next_byte();
			if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
				throw exception("Invalid utf-8");
			result = static_cast<wchar_t>(((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F));
		}
		else if ((ch[0] & 0x0F8) == 0x0F0)
		{
			ch[1] = next_byte();
			ch[2] = next_byte();
			ch[3] = next_byte();
			if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
				throw exception("Invalid utf-8");
			result = static_cast<wchar_t>(((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F));
		}
	}
	
	return result;
}				

// --------------------------------------------------------------------

struct parser_imp
{
					parser_imp(
						istream&	data,
						parser&		parser);
	
	void			parse();
	void			prolog();
	void			xml_decl();
	void			s();
	void			eq();
	void			misc();
	void			comment();

	void			element();
	void			content();

	void			doctypedecl();
	wstring			external_id(bool require_system);
	
	void			intsubset();
	void			element_decl();
	void			contentspec();
	void			cp();
	
	void			attlist_decl();
	void			notation_decl();
	void			entity_decl();
	void			parameter_entity_decl();
	void			general_entity_decl();
	void			entity_value();
	
	void			parse_parameter_entity_declaration(wstring& s);
	void			parse_general_entity_declaration(wstring& s);
	void			normalize_attribute_value(wstring& s);

	enum XMLToken
	{
		xml_Undef,
		xml_Eof = 256,

		// these are tokens for the markup
		
		xml_XMLDecl,
		xml_Space,		// Really needed
		xml_Comment,
		xml_Name,
		xml_NMToken,
		xml_String,
		xml_PI,			// <?
		xml_PIEnd,		// ?>
		xml_STag,		// <
		xml_ETag,		// </
		xml_DocType,	// <!DOCTYPE
		xml_Element,	// <!ELEMENT
		xml_AttList,	// <!ATTLIST
		xml_Entity,		// <!ENTITY
		xml_Notation,	// <!NOTATION
		
		
		xml_PEReference,	// %name;
		
		// next are tokens for the content part
		
		xml_Reference,		// &name;
		xml_CDSect,			// CData section
		xml_Content,		// anything else up to the next element start
	};

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
								case xml_PIEnd:			result = "'?>'";		 				break;
								case xml_STag:			result = "start of tag"; 				break;
								case xml_ETag:			result = "start of end tag";			break;
								case xml_DocType:		result = "<!DOCTYPE"; 					break;
								case xml_Element:		result = "<!ELEMENT"; 					break;
								case xml_AttList:		result = "<!ATTLIST"; 					break;
								case xml_Entity:		result = "<!ENTITY"; 					break;
								case xml_Notation:		result = "<!NOTATION"; 					break;
								case xml_PEReference:	result = "parameter entity reference";	break;
								case xml_Reference:		result = "entity reference"; 			break;
								case xml_CDSect:		result = "CDATA section";	 			break;
								case xml_Content:		result = "content";			 			break;
							}
						}
						
						return result;
					}

	enum State {
		state_Start			= 0,
		state_Comment		= 100,
		state_PI			= 200,
		state_EndTagStart	= 290,
		state_DocTypeDecl	= 350,
		state_TagStart		= 400,
		state_Name			= 450,
		state_String		= 500,
		state_PERef			= 700,
		state_Misc			= 1000
	};
	
	wchar_t			get_next_char();
	
	int				get_next_token();
	int				get_next_content();

	void			restart(int& start, int& state);

	void			retract();
	
	void			match(int token, bool content = false);

	bool			is_name_start_char(wchar_t uc);

	bool			is_name_char(wchar_t uc);

	wstring			m_standalone;
	parser&			m_parser;
	wstring			m_token;
	wstring			m_pi_target;
	int				m_lookahead;
	float			m_version;
	
	Encoding		m_encoding;

	stack<data_ptr>	m_data;
	stack<wchar_t>	m_buffer;
	
	map<wstring,wstring>
					m_general_entities, m_parameter_entities;
};

parser_imp::parser_imp(
	istream&		data,
	parser&			parser)
	: m_parser(parser)
{
	boost::shared_ptr<istream_data_source> source(new istream_data_source(data));
	
	m_data.push(source);
	
	m_general_entities[L"lt"] = L"&#60;";
	m_general_entities[L"gt"] = L"&#62;";
	m_general_entities[L"amp"] = L"&#38;";
	m_general_entities[L"apos"] = L"&#39;";
	m_general_entities[L"quot"] = L"&#34;";
	
	m_encoding = source->guess_encoding();
}

wchar_t parser_imp::get_next_char()
{
	wchar_t result = 0;

	if (not m_buffer.empty())
	{
		result = m_buffer.top();
		m_buffer.pop();
	}
	else
	{
		while (result == 0 and not m_data.empty())
		{
			result = m_data.top()->get_next_char();
	
			if (result == 0)
				m_data.pop();
		}
	}
	
	if (result == '\r')
	{
		result = get_next_char();

		m_token.erase(m_token.end() - 1);

		if (result != '\n')
			m_buffer.push(result);

		result = '\n';
	}
	
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
		m_lookahead = get_next_token();
}

void parser_imp::restart(int& start, int& state)
{
//	int saved_start = start, saved_state = state;
	
	while (not m_token.empty())
		retract();
	
	switch (start)
	{
		case state_Start:
			start = state = state_Comment;
			break;
		
		case state_Comment:
			start = state = state_PI;
			break;
		
		case state_PI:
			start = state = state_DocTypeDecl;
			break;
		
		case state_DocTypeDecl:
			start = state = state_EndTagStart;
			break;
		
		case state_EndTagStart:
			start = state = state_TagStart;
			break;
		
		case state_TagStart:
			start = state = state_Name;
			break;
		
		case state_Name:
			start = state = state_String;
			break;
		
		case state_String:
			start = state = state_PERef;
			break;
		
		case state_PERef:
			start = state = state_Misc;
			break;
		
		default:
			throw exception("Invalid state in restart");
	}
}

bool parser_imp::is_name_start_char(wchar_t uc)
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

bool parser_imp::is_name_char(wchar_t uc)
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

int parser_imp::get_next_token()
{
	int token = xml_Undef;
	int state, start;
	bool nmtoken = false;
	
	assert(state_Start == 0);
	
	state = start = state_Start;
	m_token.clear();
	
	while (token == xml_Undef)
	{
		wchar_t uc = get_next_char();
		
		switch (state)
		{
			// start scanning for spaces or EOF
			case state_Start:
				if (uc == 0)
					token = xml_Eof;
				else if (uc == ' ' or uc == '\t' or uc == '\n')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_Start + 1:
				if (uc != ' ' and uc != '\t' and uc != '\n')
				{
					retract();
					token = xml_Space;
				}
				break;
			
			case state_Comment:
				if (uc == '<')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_Comment + 1:
				if (uc == '!')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_Comment + 2:
				if (uc == '-')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_Comment + 3:
				if (uc == '-')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_Comment + 4:
				if (uc == '-')
					state += 1;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away comment?");
				break;
			
			case state_Comment + 5:
				if (uc == '-')
					state += 1;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away comment?");
				else
					state -= 1;
				break;
			
			case state_Comment + 6:
				if (uc == '>')
					token = xml_Comment;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away comment?");
				else
					throw exception("Invalid comment");
				break;

			// scan for processing instructions
			case state_PI:
				if (uc == '<')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_PI + 1:
				if (uc == '?')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_PI + 2:
				if (is_name_start_char(uc))
					state += 1;
				else
					restart(start, state);
				break;

			case state_PI + 3:
				if (not is_name_char(uc))
				{
					retract();

					m_pi_target = m_token.substr(2);
					
					if (m_pi_target == L"xml")
						token = xml_XMLDecl;
					else
						state += 1;
				}
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away processing instruction?");
				break;
			
			case state_PI + 4:
				if (uc == '?')
					state += 1;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away processing instruction?");
				break;
			
			case state_PI + 5:
				if (uc == '>')
					token = xml_PI;
				else if (uc == 0)
					throw exception("Unexpected end of file, run-away processing instruction?");
				else
					state -= 1;
				break;

			case state_DocTypeDecl:
				if (uc == '<')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_DocTypeDecl + 1:
				if (uc == '!')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_DocTypeDecl + 2:
				if (is_name_start_char(uc))
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_DocTypeDecl + 3:
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
						restart(start, state);
				}
				break;

			// scan for end tags 
			case state_EndTagStart:
				if (uc == '<')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_EndTagStart + 1:
				if (uc == '/')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_EndTagStart + 2:
				if (is_name_start_char(uc))
				{
					retract();
					token = xml_ETag;
				}
				else
					throw exception("Unexpected character following </");
				break;
			
			// scan for tags 
			case state_TagStart:
				if (uc == '<')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_TagStart + 1:
				if (uc == '?')
					state = state_TagStart + 20;
				else if (uc == '!')
					token += 1;
				else if (is_name_start_char(uc))
				{
					retract();
					token = xml_STag;
				}
				else
					throw exception("Unexpected character following <");
				break;
			
			// comment, doctype, entity, etc
			case state_TagStart + 2:
				if (uc == '-')						// comment
					state += 1;
				else
					restart(start, state);
				break;

			// comment
			case state_TagStart + 3:
				if (uc == '-')
					state += 1;
				break;
			
			case state_TagStart + 4:
				if (uc == '-')
					state += 1;
				else
					state -= 1;
				break;
			
			case state_TagStart + 5:
				if (uc == '>')
					token = xml_Comment;
				else
					throw exception("Invalid formatted comment");
				break;
			
			// Names
			case state_Name:
				if (is_name_start_char(uc))
					state += 1;
				else if (is_name_char(uc))
				{
					nmtoken = true;
					state += 1;
				}
				else
					restart(start, state);
				break;
			
			case state_Name + 1:
				if (not is_name_char(uc))
				{
					retract();
	
					if (nmtoken)
						token = xml_NMToken;
					else
						token = xml_Name;
				}
				break;
			
			// strings
			case state_String:
				if (uc == '"')
					state += 10;
				else if (uc == '\'')
					state += 20;
				else
					restart(start, state);
				break;

			case state_String + 10:
				if (uc == '"')
				{
					token = xml_String;
					m_token = m_token.substr(1, m_token.length() - 2);
				}
				else if (uc == 0)
					throw exception("unexpected end of file, runaway string");
				break;

			case state_String + 20:
				if (uc == '\'')
				{
					token = xml_String;
					m_token = m_token.substr(1, m_token.length() - 2);
				}
				else if (uc == 0)
					throw exception("unexpected end of file, runaway string");
				break;
			
			// parameter entity references
			case state_PERef:
				if (uc == '%')
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_PERef + 1:
				if (is_name_start_char(uc))
					state += 1;
				else
					restart(start, state);
				break;
			
			case state_PERef + 2:
				if (uc == ';')
				{
					m_token.erase(m_token.end() - 1);
					m_token.erase(m_token.begin(), m_token.begin() + 1);
					token = xml_PEReference;
				}
				else if (not is_name_char(uc))
					restart(start, state);
				break;
			
			// misc
			case state_Misc:
				if (uc == '?')
					state += 1;
				else
					token = uc;
				break;
			
			case state_Misc + 1:
				if (uc == '>')
					token = xml_PIEnd;
				else
				{
					retract();
					token = '?';
				}
				break;
		}
	}
	
	if (VERBOSE)
		cout << "get_next_token: " << describe_token(token) << " (" << wstring_to_string(m_token) << ")" << endl;
	
	return token;
}

int parser_imp::get_next_content()
{
	int token = xml_Undef;
	
	m_token.clear();
	
	int state = 0 /*, start = 0*/;
	wchar_t charref = 0;
	
	while (token == xml_Undef)
	{
		wchar_t uc = get_next_char();
		
		switch (state)
		{
			case 0:
				if (uc == 0)
					token = xml_Eof;
				else if (uc == '<')
					state = 5;
				else if (uc == '&')
					state = 10;
				else
					state += 1;	// anything else
				break;
			
			case 1:
				if (uc == 0 or uc == '<' or uc == '&')
				{
					retract();
					token = xml_Content;
				}
				break;
			
			case 5:
				if (uc == '/')
					token = xml_ETag;
				else if (uc == '?')
					state += 1;
				else if (uc == '!')
					state = 15;
				else
				{
					retract();
					token = xml_STag;
				}
				break;
			
			case 6:
				if (is_name_start_char(uc))
					state = 7;
				else
					throw exception("expected target in processing instruction");
				break;
			
			case 7:
				if (uc == '?')
					state = 8;
				else if (uc == 0)
					throw exception("runaway processing instruction");
				break;
			
			case 8:
				if (uc == '>')
					token = xml_PI;
				else if (uc == 0)
					throw exception("runaway processing instruction");
				else if (uc != '?')
					state = 7;
				break;
			
			case 15:
				if (uc == '-')		// comment
					state = 16;
				else if (uc == '[')
					state = 40;		// CDATA
				else
					throw exception("invalid content");
				break;

			case 16:
				if (uc == '-')
					state = 17;
				else
					throw exception("invalid content");
				break;
			
			case 17:
				if (uc == '-')
					state = 18;
				else if (uc == 0)
					throw exception("runaway comment");
				break;
			
			case 18:
				if (uc == '-')
					state = 19;
				else if (uc == 0)
					throw exception("runaway processing instruction");
				else
					state = 17;
				break;
			
			case 19:
				if (uc == '>')
					token = xml_Comment;
				else
					throw exception("invalid comment");
				break;
			
			case 10:
				if (uc == '#')
					state = 20;
				else if (is_name_start_char(uc))
					state = 11;
				else
					throw exception("stray ampersand found in content");
				break;
			
			case 11:
				if (not is_name_char(uc))
				{
					if (uc != ';')
						throw exception("invalid entity found in content, missing semicolon?");
					token = xml_Reference;
					m_token = m_token.substr(1, m_token.length() - 2);
				}
				break;
			
			case 20:
				if (uc == 'x')
					state = 30;
				else if (uc >= '0' and uc <= '9')
				{
					charref = uc - '0';
					state += 1;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 21:
				if (uc >= '0' and uc <= '9')
					charref = charref * 10 + (uc - '0');
				else if (uc == ';')
				{
					m_token = charref;
					token = xml_Content;
				}
				else
					throw exception("invalid character reference");
				break;
			
			case 30:
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
			
			case 31:
				if (uc >= 'a' and uc <= 'f')
					charref = (charref << 4) + (uc - 'a' + 10);
				else if (uc >= 'A' and uc <= 'F')
					charref = (charref << 4) + (uc - 'A' + 10);
				else if (uc >= '0' and uc <= '9')
					charref = (charref << 4) + (uc - '0');
				else if (uc == ';')
				{
					m_token = charref;
					token = xml_Content;
				}
				else
					throw exception("invalid character reference");
				break;

			case 40:
				if (is_name_start_char(uc))
					state += 1;
				else
					throw exception("invalid content");
				break;
			
			case 41:
				if (uc == '[' and m_token == L"<![CDATA[")
					state += 1;	
				else if (not is_name_char(uc))
					throw exception("invalid content");
				break;

			case 42:
				if (uc == ']')
					state += 1;
				else if (uc == 0)
					throw exception("runaway cdata section");
				break;
			
			case 43:
				if (uc == ']')
					state += 1;
				else if (uc == 0)
					throw exception("runaway cdata section");
				else if (uc != ']')
					state = 42;
				break;

			case 44:
				if (uc == '>')
				{
					token = xml_CDSect;
					m_token = m_token.substr(9, m_token.length() - 12);
				}
				else if (uc == 0)
					throw exception("runaway cdata section");
				else if (uc != ']')
					state = 42;
				break;

		}
	}
	
	if (VERBOSE)
		cout << "get_next_content: " << describe_token(token) << " (" << wstring_to_string(m_token) << ")" << endl;
	
	return token;
}

void parser_imp::parse()
{
	m_lookahead = get_next_token();
	
	// first parse the xmldecl
	
	prolog();
	
	element();

	// misc
	while (m_lookahead != xml_Eof)
		misc();
}

void parser_imp::prolog()
{
	if (m_lookahead == xml_XMLDecl)	// <?
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
	match(xml_XMLDecl);
	match(xml_Space);
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
		match(xml_Space);
		
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
			}
			else if (m_token == L"ISO-8859-1")
				m_encoding = enc_ISO88591;
			match(xml_String);
			continue;
		}
		
		if (m_token == L"standalone")
		{
			match(xml_Name);
			eq();
			if (m_token != L"yes" and m_token != L"no")
				throw exception("Invalid XML declaration, standalone value should be either yes or no");
			if (m_token == L"no")
				throw exception("Unsupported XML declaration, only standalone documents are supported");
			m_standalone = m_token;
			match(xml_String);
			continue;
		}
		
		throw exception("unexpected attribute in xml declaration");
	}
	
	match(xml_PIEnd);
}

void parser_imp::s()
{
	if (m_lookahead == xml_Space)
		match(xml_Space);
}

void parser_imp::eq()
{
	s();

	match('=');

	s();
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
	match(xml_DocType);
	
	match(xml_Space);
	
	wstring name = m_token;
	match(xml_Name);

	if (m_lookahead == xml_Space)
	{
		match(xml_Space);
		
		wstring e;
		
		if (m_lookahead == xml_Name)
		{
			e = external_id(true);
			
			if (not e.empty())
			{
				// push e
				wstring replacement;
				replacement += ' ';
				replacement += e;
				replacement += ' ';
				
				m_data.push(data_ptr(new wstring_data_source(replacement)));
				
				// parse e as if it was internal
				intsubset();
			}
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
	
	match('>');
}

void parser_imp::intsubset()
{
	while (m_lookahead != xml_Eof and m_lookahead != ']' and m_lookahead != '[')
	{
		switch (m_lookahead)
		{
			case xml_PEReference:
			{
				map<wstring,wstring>::iterator r = m_parameter_entities.find(m_token);
				if (r == m_parameter_entities.end())
					throw exception("undefined parameter entity %s", m_token.c_str());
				
				wstring replacement;
				replacement += ' ';
				replacement += r->second;
				replacement += ' ';
				
				m_data.push(data_ptr(new wstring_data_source(replacement)));
				
				match(xml_PEReference);
				break;
			}
			
			case xml_Space:
				match(xml_Space);
				break;
			
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
				match(xml_Comment);
				break;

			default:
				throw exception("unexpected token %s", describe_token(m_lookahead).c_str());
		}
	}
	
}

void parser_imp::element_decl()
{
	match(xml_Element);
	match(xml_Space);
	match(xml_Name);
	match(xml_Space);
	contentspec();
	s();
	match('>');
}

void parser_imp::contentspec()
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
		
		if (m_lookahead == '#')	// Mixed
		{
			match(m_lookahead);
			if (m_token != L"PCDATA")
				throw exception("Invalid element content specification, expected #PCDATA");
			match(xml_Name);
			
			s();
			
			if (m_lookahead == '|')
			{
				while (m_lookahead == '|')
				{
					match('|');
					s();
					match(xml_Name);
					s();
				}
				
				match(')');
				match('*');
			}
			else
				match(')');
		}
		else					// children
		{
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
				}
				while (m_lookahead == '|');
			}
			match(')');
			
			if (m_lookahead == '*' or m_lookahead == '+' or m_lookahead == '?')
				match(m_lookahead);
		}
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
			}
			while (m_lookahead == '|');
		}
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
	match(xml_Space);

	if (m_lookahead == '%')	// PEDecl
		parameter_entity_decl();
	else
		general_entity_decl();
}

void parser_imp::parameter_entity_decl()
{
	match('%');
	match(xml_Space);
	
	wstring name = m_token;
	match(xml_Name);
	
	match(xml_Space);

	wstring value;
	
	// PEDef is either a EntityValue...
	if (m_lookahead == xml_String)
	{
		value = m_token;
		match(xml_String);
		parse_parameter_entity_declaration(value);
	}
	else
	{
		
		
		value = external_id(true);
	}

	s();
	
	match('>');
	
	if (m_parameter_entities.find(name) == m_parameter_entities.end())
		m_parameter_entities[name] = value;
}

void parser_imp::general_entity_decl()
{
	wstring name = m_token;
	match(xml_Name);
	match(xml_Space);
	
	wstring value;

	if (m_lookahead == xml_String)
	{
		value = m_token;
		match(xml_String);
	
		parse_general_entity_declaration(value);
	}
	else // ... or an ExternalID
	{
		value = external_id(true);

		if (m_lookahead == xml_Space)
		{
			match(xml_Space);
			if (m_lookahead == xml_Name and m_token == L"NDATA")
			{
				match(xml_Name);
				match(xml_Space);
				match(xml_Name);
			}
		}
	}	
	
	s();
	
	match('>');
	
	if (m_general_entities.find(name) == m_general_entities.end())
		m_general_entities[name] = value;
}

void parser_imp::attlist_decl()
{
	match(xml_AttList);
	match(xml_Space);
	wstring name = m_token;
	match(xml_Name);
	
	while (m_lookahead == xml_Space)
	{
		match(xml_Space);
		
		if (m_lookahead != xml_Name)
			break;
	
		wstring n = m_token;
		match(xml_Name);
		match(xml_Space);
		
		// att type: several possibilities:
		if (m_lookahead == '(')	// enumeration
		{
			match(m_lookahead);
			
			s();
			
			if (m_lookahead == xml_Name)
				match(xml_Name);
			else
				match(xml_NMToken);

			s();
			
			while (m_lookahead == '|')
			{
				match('|');

				s();

				if (m_lookahead == xml_Name)
					match(xml_Name);
				else
					match(xml_NMToken);

				s();
			}

			s();
			
			match(')');
		}
		else
		{
			wstring type = m_token;
			match(xml_Name);
			
			if (type == L"CDATA")
				;
			else if (type == L"ID" or type == L"IDREF" or type == L"IDREFS" or type == L"ENTITY" or type == L"ENTITIES" or type == L"NMTOKEN" or type == L"NMTOKENS")
				;
			else if (type == L"NOTATION")
			{
				match(xml_Space);
				match('(');
				s();
				match(xml_Name);

				while (m_lookahead == '|')
				{
					match('|');
	
					s();
	
					match(xml_Name);
	
					s();
				}
	
				s();
				
				match(')');
			}
			else
				throw exception("invalid attribute type");
		}
		
		// att def

		s();
		
		if (m_lookahead == '#')
		{
			match(m_lookahead);
			wstring def = m_token;
			match(xml_Name);

			if (def == L"REQUIRED")
				;
			else if (def == L"IMPLIED")
				;
			else if (def == L"FIXED")
			{
				s();
				
				match(xml_String);
			}
			else
				throw exception("invalid attribute default");
		}
		else
		{
			match(xml_String);
		}
	}

	match('>');
}

void parser_imp::notation_decl()
{
	match(xml_Notation);
	match(xml_Space);
	match(xml_Name);
	match(xml_Space);
	external_id(false);
	s();
	match('>');
}

wstring parser_imp::external_id(bool require_system)
{
	wstring result;
	
	if (m_token == L"SYSTEM")
	{
		match(xml_Name);
		match(xml_Space);
		
		wstring sys = m_token;
		
		match(xml_String);
		
		// result = retrieve_external_dtd(sys);
	}
	else if (m_token == L"PUBLIC")
	{
		match(xml_Name);
		match(xml_Space);
		
		wstring pub = m_token;
		match(xml_String);
		
		if (require_system)
		{
			match(xml_Space);
			wstring sys = m_token;
			match(xml_String);
		}
		
		// retrieve_external_dtd(sys, pub);
	}
	else
		throw exception("Expected external id starting with either SYSTEM or PUBLIC");
	
	return result;
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
					state = 20;
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
					map<wstring,wstring>::iterator e = m_parameter_entities.find(name);
					if (e == m_parameter_entities.end())
						throw exception("undefined parameter entity reference %s", wstring_to_string(name).c_str());
					result += e->second;
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
					state = 20;
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
//					map<wstring,wstring>::iterator e = m_general_entities.find(name);
//					if (e == m_general_entities.end())
//						throw exception("undefined entity reference %s", wstring_to_string(name).c_str());
//					result += e->second;

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
					map<wstring,wstring>::iterator e = m_parameter_entities.find(name);
					if (e == m_parameter_entities.end())
						throw exception("undefined parameter entity reference %s", wstring_to_string(name).c_str());
					result += e->second;
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

void parser_imp::normalize_attribute_value(wstring& s)
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
				else if (c == ' ' or c == '\n' or c == '\t')
				{
					if (not result.empty() and result[result.length() - 1] != ' ')
						result += ' ';
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
					map<wstring,wstring>::iterator e = m_general_entities.find(name);
					if (e == m_general_entities.end())
						throw exception("undefined entity reference %s", wstring_to_string(name).c_str());
					
					wstring replacement = e->second;
					normalize_attribute_value(replacement);
					
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
	
	ba::trim(result);
	swap(s, result);
//	
//	bool space = false;
//	s.clear();
//	for (wstring::iterator i = result.begin(); i != result.end(); ++i)
//	{
//		if (*i == ' ')
//		{
//			if (not space)
//				s += ' ';
//			space = true;
//		}
//		else
//		{
//			s += *i;
//			space = false;
//		}
//	}
}

void parser_imp::element()
{
	match(xml_STag);
	wstring name = m_token;
	match(xml_Name);
	
	attribute_list attrs;
	
	for (;;)
	{
		if (m_lookahead != xml_Space)
			break;
		
		match(xml_Space);
		
		if (m_lookahead != xml_Name)
			break;
		
		string attr_name = wstring_to_string(m_token);
		match(xml_Name);
		
		s();
		
		match('=');

		s();

		wstring attr_value = m_token;
		match(xml_String);
		
		normalize_attribute_value(attr_value);
		
		attribute_ptr attr(new attribute(attr_name, wstring_to_string(attr_value)));
		
		attrs.push_back(attr);
	}

	if (m_lookahead == '/')
	{
		match('/');
		match('>');
		
		if (m_parser.start_element_handler)
			m_parser.start_element_handler(wstring_to_string(name), attrs);

		if (m_parser.end_element_handler)
			m_parser.end_element_handler(wstring_to_string(name));
	}
	else
	{
		if (m_parser.start_element_handler)
			m_parser.start_element_handler(wstring_to_string(name), attrs);
		
		match('>', true);

		content();
		
		match(xml_ETag);
		
		if (name != m_token)
			throw exception("end tag does not match start tag");
		
		match(xml_Name);

		while (m_lookahead == xml_Space)
			match(m_lookahead);
		
		match('>');
		
		if (m_parser.end_element_handler)
			m_parser.end_element_handler(wstring_to_string(name));
	}
	
	while (m_lookahead == xml_Space)
		match(m_lookahead);
}

void parser_imp::content()
{
	wstring data;
	
	while (m_lookahead != xml_ETag and m_lookahead != xml_Eof)
	{
		switch (m_lookahead)
		{
			case xml_Content:
				if (m_parser.character_data_handler)
					m_parser.character_data_handler(wstring_to_string(m_token));
				match(xml_Content, true);
				break;
			
			case xml_Reference:
			{
				map<wstring,wstring>::iterator e = m_general_entities.find(m_token);
				if (e == m_general_entities.end())
					throw exception("undefined entity reference %s", wstring_to_string(m_token).c_str());
				
				m_data.push(data_ptr(new wstring_data_source(e->second)));

				match(xml_Reference, true);
				break;
			}
			
			case xml_STag:
				element();
				break;
			
			case xml_PI:
				match(xml_PI, true);
				break;
			
			case xml_Comment:
				match(xml_Comment, true);
				break;
			
			case xml_CDSect:
				if (m_parser.start_cdata_section_handler)
					m_parser.start_cdata_section_handler();
				
				if (m_parser.character_data_handler)
					m_parser.character_data_handler(wstring_to_string(m_token));
				
				if (m_parser.end_cdata_section_handler)
					m_parser.end_cdata_section_handler();
				
				match(xml_CDSect, true);
				break;

			default:
				throw exception("unexpected token %s", describe_token(m_lookahead).c_str());

		}
	}
}

// --------------------------------------------------------------------

parser::parser(istream& data)
	: m_impl(new parser_imp(data, *this))
	, m_istream(NULL)
{
}

parser::parser(const string& data)
	: m_impl(NULL)
	, m_istream(NULL)
{
	m_istream = new istringstream(data);
	m_impl = new parser_imp(*m_istream, *this);
}

parser::~parser()
{
	delete m_impl;
	delete m_istream;
}

void parser::parse()
{
	m_impl->parse();
}

}
}
