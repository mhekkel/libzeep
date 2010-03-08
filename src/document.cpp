//  Copyright Maarten L. Hekkelman, Radboud University 2008.
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

namespace zeep { namespace xml {

//// --------------------------------------------------------------------
//
//enum Encoding {
//	enc_UTF8,
//	enc_UTF16BE,
//	enc_UTF16LE,
//	enc_ISO88591
//};
//
//// parsing XML is somewhat like macro processing,
//// we can encounter entities that need to be expanded into replacement text
//// and so we declare data_source objects that can be stacked.
//
//class data_source
//{
//  public:
//	virtual			~data_source() {}
//
//	wchar_t			next();
//
//	void			put_back(wchar_t ch);
//	
//  protected:
//	virtual wchar_t	get_next_char() = 0;
//
//	stack<wchar_t>	m_char_buffer;
//};
//
//typedef boost::shared_ptr<data_source>	data_ptr;
//
//wchar_t data_source::next()
//{
//	wchar_t result = 0;
//	
//	if (not m_char_buffer.empty())
//	{
//		result = m_char_buffer.top();
//		m_char_buffer.pop();
//	}
//	else
//		result = get_next_char();
//	
//	return result;
//}
//
//void data_source::put_back(wchar_t ch)
//{
//	m_char_buffer.push(ch);
//}
//
//// --------------------------------------------------------------------
//
//class istream_data_source : public data_source
//{
//  public:
//					istream_data_source(
//						istream&		data)
//						: m_data(data)
//						, m_encoding(enc_UTF8)
//						, m_has_bom(false)
//					{
//					}
//
//	Encoding		guess_encoding();
//	
//	bool			has_bom();
//
//  private:
//
//	virtual wchar_t	get_next_char();
//
//	wchar_t			next_utf8_char();
//	
//	wchar_t			next_utf16le_char();
//	
//	wchar_t			next_utf16be_char();
//	
//	wchar_t			next_iso88591_char();
//
//	char			next_byte();
//
//	istream&		m_data;
//	stack<char>		m_byte_buffer;
//	Encoding		m_encoding;
//
//	boost::function<wchar_t(void)>
//					m_next;
//	bool			m_has_bom;
//	bool			m_valid_utf8;
//};
//
//Encoding istream_data_source::guess_encoding()
//{
//	// 1. easy first step, see if there is a BOM
//	
//	char c1, c2, c3;
//	
//	m_data.read(&c1, 1);
//
//	if (not m_data.eof())
//	{
//		if (c1 == 0xfe)
//		{
//			m_data.read(&c2, 1);
//			
//			if (not m_data.eof() and c2 == 0xff)
//			{
//				m_encoding = enc_UTF16BE;
//				m_has_bom = true;
//			}
//			else
//			{
//				m_byte_buffer.push(c2);
//				m_byte_buffer.push(c1);
//			}
//		}
//		else if (c1 == 0xff)
//		{
//			m_data.read(&c2, 1);
//			
//			if (not m_data.eof() and c2 == 0xfe)
//			{
//				m_encoding = enc_UTF16LE;
//				m_has_bom = true;
//			}
//			else
//			{
//				m_byte_buffer.push(c2);
//				m_byte_buffer.push(c1);
//			}
//		}
//		else if (c1 == 0xef)
//		{
//			m_data.read(&c2, 1);
//			m_data.read(&c3, 1);
//			
//			if (not m_data.eof() and c2 == 0xbb and c3 == 0xbf)
//			{
//				m_encoding = enc_UTF8;
//				m_has_bom = true;
//			}
//			else
//			{
//				m_byte_buffer.push(c3);
//				m_byte_buffer.push(c2);
//				m_byte_buffer.push(c1);
//			}
//		}
//		else
//			m_byte_buffer.push(c1);
//	}
//
//	switch (m_encoding)
//	{
//		case enc_UTF8:		m_next = boost::bind(&istream_data_source::next_utf8_char, this); break;
//		case enc_UTF16LE:	m_next = boost::bind(&istream_data_source::next_utf16le_char, this); break;
//		case enc_UTF16BE:	m_next = boost::bind(&istream_data_source::next_utf16be_char, this); break;
//		case enc_ISO88591:	m_next = boost::bind(&istream_data_source::next_iso88591_char, this); break;
//	}
//	
//	return m_encoding;
//}
//
//char istream_data_source::next_byte()
//{
//	char result;
//	
//	if (m_byte_buffer.empty())
//		m_data.read(&result, 1);
//	else
//	{
//		result = m_byte_buffer.top();
//		m_byte_buffer.pop();
//	}
//
//	return result;
//}
//
//wchar_t istream_data_source::next_utf8_char()
//{
//	wchar_t result = 0;
//	char ch[5];
//	
//	ch[0] = next_byte();
//	
//	if ((ch[0] & 0x080) == 0)
//		result = ch[0];
//	else if ((ch[0] & 0x0E0) == 0x0C0)
//	{
//		ch[1] = next_byte();
//		if ((ch[1] & 0x0c0) != 0x080)
//			throw exception("Invalid utf-8");
//		result = static_cast<wchar_t>(((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F));
//	}
//	else if ((ch[0] & 0x0F0) == 0x0E0)
//	{
//		ch[1] = next_byte();
//		ch[2] = next_byte();
//		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
//			throw exception("Invalid utf-8");
//		result = static_cast<wchar_t>(((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F));
//	}
//	else if ((ch[0] & 0x0F8) == 0x0F0)
//	{
//		ch[1] = next_byte();
//		ch[2] = next_byte();
//		ch[3] = next_byte();
//		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
//			throw exception("Invalid utf-8");
//		result = static_cast<wchar_t>(((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F));
//	}
//	
//	if (m_data.eof())
//		result = 0;
//	
//	return result;
//}
//
//wchar_t istream_data_source::next_utf16le_char()
//{
//	throw exception("to be implemented");
//	return 0;
//}
//
//wchar_t istream_data_source::next_utf16be_char()
//{
//	throw exception("to be implemented");
//	return 0;
//}
//
//wchar_t istream_data_source::next_iso88591_char()
//{
//	throw exception("to be implemented");
//	return 0;
//}
//
//// --------------------------------------------------------------------
//
//class wstring_data_source : public data_source
//{
//  public:
//					wstring_data_source(
//						const wstring&	data)
//						: m_data(data)
//						, m_ptr(m_data.begin())
//					{
//					}
//
//  private:
//
//	virtual wchar_t	get_next_char();
//
//	char			next_byte();
//					
//	const wstring&	m_data;
//	wstring::const_iterator
//					m_ptr;
//};
//
//char wstring_data_source::next_byte()
//{
//	char result = 0;
//	if (m_ptr != m_data.end())
//	{
//		result = *m_ptr;
//		++m_ptr;
//	}
//	return result;
//}
//
//wchar_t	wstring_data_source::get_next_char()
//{
//	wchar_t result = 0;
//	if (m_ptr != m_data.end())
//	{
//		char ch[4];
//		
//		ch[0] = next_byte();
//		
//		if ((ch[0] & 0x080) == 0)
//			result = ch[0];
//		else if ((ch[0] & 0x0E0) == 0x0C0)
//		{
//			ch[1] = next_byte();
//			if ((ch[1] & 0x0c0) != 0x080)
//				throw exception("Invalid utf-8");
//			result = static_cast<wchar_t>(((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F));
//		}
//		else if ((ch[0] & 0x0F0) == 0x0E0)
//		{
//			ch[1] = next_byte();
//			ch[2] = next_byte();
//			if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
//				throw exception("Invalid utf-8");
//			result = static_cast<wchar_t>(((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F));
//		}
//		else if ((ch[0] & 0x0F8) == 0x0F0)
//		{
//			ch[1] = next_byte();
//			ch[2] = next_byte();
//			ch[3] = next_byte();
//			if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
//				throw exception("Invalid utf-8");
//			result = static_cast<wchar_t>(((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F));
//		}
//	}
//	
//	return result;
//}				
//
//// --------------------------------------------------------------------
//
//struct document_imp
//{
//					document_imp(
//						istream&	data);
//	
//	void			parse();
//	void			prolog();
//	void			xml_decl();
//	void			s();
//	void			eq();
//	void			misc();
//	void			comment();
//
//	void			doctypedecl();
//	wstring			external_id();
//	
//	void			intsubset();
//	void			element_decl();
//	void			contentspec();
//	
//	void			attlist_decl();
//	void			notation_decl();
//	void			entity_decl();
//	void			parameter_entity_decl();
//	void			general_entity_decl();
//	void			entity_value();
//	
//	void			parse_parameter_entity_declaration(wstring& s);
//	void			parse_general_entity_declaration(wstring& s);
//
//	enum XMLToken
//	{
//		xml_Undef,
//		xml_Eof = 256,
//		xml_XMLDecl,
//		xml_Space,		// Really needed
//		xml_Comment,
//		xml_Name,
//		xml_String,
//		xml_DeclStart,	// <?xml
//		xml_PI,			// <?
//		xml_PIEnd,		// ?>
//		xml_STag,		// <
//		xml_ETag,		// >
//		xml_DocType,	// <!DOCTYPE
//		xml_Element,	// <!ELEMENT
//		xml_AttList,	// <!ATTLIST
//		xml_Entity,		// <!ENTITY
//		xml_Notation,	// <!NOTATION
//		
//		
//		xml_PEReference,	// %name;
//		xml_Reference,		// &name;
//	};
//
//	enum State {
//		state_Start		= 0,
//		state_Comment	= 100,
//		state_PI		= 200,
//		state_TagStart	= 300,
//		state_Name		= 400,
//		state_String	= 500,
//		state_GERef		= 600,
//		state_PERef		= 700,
//		state_Misc		= 1000
//	};
//	
//	wchar_t			get_next_char();
//	
//	int				get_next_token();
//
//	void			restart(int& start, int& state);
//
//	void			retract();
//	
//	void			match(int token);
//
//	bool			is_name_start_char(wchar_t uc);
//
//	bool			is_name_char(wchar_t uc);
//
//	wstring			m_standalone;
//	node_ptr		m_root;
//	wstring			m_token;
//	wstring			m_pi_target;
//	int				m_lookahead;
//	float			m_version;
//	
//	Encoding		m_encoding;
//
//	stack<data_ptr>	m_data;
//	
//	map<wstring,wstring>
//					m_general_entities, m_parameter_entities;
//};
//
//document_imp::document_imp(
//	istream&		data)
//{
//	boost::shared_ptr<istream_data_source> source(new istream_data_source(data));
//	
//	m_data.push(source);
//	
//	m_general_entities[L"lt"] = L"<";
//	m_general_entities[L"gt"] = L">";
//	m_general_entities[L"amp"] = L"&";
//	m_general_entities[L"apos"] = L"'";
//	m_general_entities[L"quot"] = L"\"";
//	
//	m_encoding = source->guess_encoding();
//}
//
////string document_imp::get_token_string()
////{
////	string token;
////	token.reserve(m_token.size());
////	
////	for (vector<wchar_t>::iterator ch = m_token.begin(); ch != m_token.end(); ++ch)
////	{
////		if (*ch < 0x080)
////			token.push_back(static_cast<char> (*ch));
////		else if (*ch < 0x0800)
////		{
////			token.push_back(static_cast<char> (0x0c0 | (*ch >> 6)));
////			token.push_back(static_cast<char> (0x080 | (*ch & 0x3f)));
////		}
////		else if (*ch < 0x00010000)
////		{
////			token.push_back(static_cast<char> (0x0e0 | (*ch >> 12)));
////			token.push_back(static_cast<char> (0x080 | ((*ch >> 6) & 0x3f)));
////			token.push_back(static_cast<char> (0x080 | (*ch & 0x3f)));
////		}
////		else
////		{
////			token.push_back(static_cast<char> (0x0f0 | (*ch >> 18)));
////			token.push_back(static_cast<char> (0x080 | ((*ch >> 12) & 0x3f)));
////			token.push_back(static_cast<char> (0x080 | ((*ch >> 6) & 0x3f)));
////			token.push_back(static_cast<char> (0x080 | (*ch & 0x3f)));
////		}
////	}
////	
////	m_token = token;
////	
////	return token;
////}
//
//wchar_t document_imp::get_next_char()
//{
//	wchar_t result = 0;
//	
//	if (not m_data.empty())
//	{
//		result = m_data.top()->next();
//
//		if (result == 0)
//			m_data.pop();
//	}
//	
//	m_token += result;
//	
//	return result;
//}
//
//void document_imp::retract()
//{
//	assert(not m_token.empty());
//	
//	wstring::iterator last_char = m_token.end() - 1;
//	
//	m_data.top()->put_back(*last_char);
//	m_token.erase(last_char);
//}
//
//void document_imp::match(int token)
//{
//	if (m_lookahead != token)
//		throw exception("Error parsing XML, expected %d but found %d", token, m_lookahead);
//	
//	m_lookahead = get_next_token();
//}
//
//void document_imp::restart(int& start, int& state)
//{
//	while (not m_token.empty())
//		retract();
//	
//	switch (start)
//	{
//		case state_Start:
//			start = state = state_Comment;
//			break;
//		
//		case state_Comment:
//			start = state = state_PI;
//			break;
//		
//		case state_PI:
//			start = state = state_TagStart;
//			break;
//		
//		case state_TagStart:
//			start = state = state_Name;
//			break;
//		
//		case state_Name:
//			start = state = state_String;
//			break;
//		
//		case state_String:
//			start = state = state_PERef;
//			break;
//		
//		case state_PERef:
//			start = state = state_GERef;
//			break;
//		
//		case state_GERef:
//			start = state = state_Misc;
//			break;
//		
//		default:
//			throw exception("Invalid state in restart");
//	}
//}
//
//bool document_imp::is_name_start_char(wchar_t uc)
//{
//	return
//		uc == ':' or
//		(uc >= 'A' and uc <= 'Z') or
//		uc == '_' or
//		(uc >= 'a' and uc <= 'z') or
//		(uc >= 0x0C0 and uc <= 0x0D6) or
//		(uc >= 0x0D8 and uc <= 0x0F6) or
//		(uc >= 0x0F8 and uc <= 0x02FF) or
//		(uc >= 0x0370 and uc <= 0x037D) or
//		(uc >= 0x037F and uc <= 0x01FFF) or
//		(uc >= 0x0200C and uc <= 0x0200D) or
//		(uc >= 0x02070 and uc <= 0x0218F) or
//		(uc >= 0x02C00 and uc <= 0x02FEF) or
//		(uc >= 0x03001 and uc <= 0x0D7FF) or
//		(uc >= 0x0F900 and uc <= 0x0FDCF) or
//		(uc >= 0x0FDF0 and uc <= 0x0FFFD) or
//		(uc >= 0x010000 and uc <= 0x0EFFFF);	
//}
//
//bool document_imp::is_name_char(wchar_t uc)
//{
//	return
//		is_name_start_char(uc) or
//		uc == '-' or
//		uc == '.' or
//		(uc >= 0 and uc <= 9) or
//		uc == 0x0B7 or
//		(uc >= 0x00300 and uc <= 0x0036F) or
//		(uc >= 0x0203F and uc <= 0x02040);
//}
//
//int document_imp::get_next_token()
//{
//	int token = xml_Undef;
//	int state = state_Start, start = state_Start;
//	
//	while (token == xml_Undef)
//	{
//		wchar_t uc = get_next_char();
//		
//		switch (state)
//		{
//			// start scanning for spaces or EOF
//			case state_Start:
//				if (uc == 0)
//					token = xml_Eof;
//				else if (uc == ' ' or uc == '\t' or '\n')
//					state += 1;
//				else if (uc == '\r')
//				{
//					m_token[m_token.length() - 1] = '\n';
//					state += 2;
//				}
//				else
//					restart(start, state);
//				break;
//			
//			case state_Start + 1:
//				if (uc == '\r')
//					state += 1;
//				else if (uc != ' ' and uc != '\t' and uc != '\n')
//				{
//					retract();
//					token = xml_Space;
//				}
//				break;
//			
//			case state_Start + 2:
//				if (uc == '\n')
//				{
//					m_token.erase(m_token.end() - 1);
//					state -= 1;
//				}
//				else if (uc != ' ' and uc != '\t' and uc != '\n')
//				{
//					retract();
//					token = xml_Space;
//				}
//				else	
//					state -= 1;
//				break;
//			
//			case state_Comment:
//				if (uc == '<')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_Comment + 1:
//				if (uc == '!')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_Comment + 2:
//				if (uc == '-')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_Comment + 3:
//				if (uc == '-')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_Comment + 4:
//				if (uc == '-')
//					state += 1;
//				else if (uc == 0)
//					throw exception("Unexpected end of file, run-away comment?");
//				break;
//			
//			case state_Comment + 5:
//				if (uc == '-')
//					state += 1;
//				else if (uc == 0)
//					throw exception("Unexpected end of file, run-away comment?");
//				else
//					state -= 1;
//				break;
//			
//			case state_Comment + 6:
//				if (uc == '>')
//					token = xml_Comment;
//				else if (uc == 0)
//					throw exception("Unexpected end of file, run-away comment?");
//				else
//					throw exception("Invalid comment");
//				break;
//
//			// scan for processing instructions
//			case state_PI:
//				if (uc == '<')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_PI + 1:
//				if (uc == '?')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_PI + 2:
//				if (is_name_start_char(uc))
//					state += 1;
//				else
//					restart(start, state);
//				break;
//
//			case state_PI + 3:
//				if (not is_name_char(uc))
//				{
//					m_pi_target = m_token.substr(2);
//					
//					if (m_pi_target == L"xml")
//					{
//						retract();
//						token = xml_XMLDecl;
//					}
//					else
//						state += 1;
//				}
//				else if (uc == 0)
//					throw exception("Unexpected end of file, run-away processing instruction?");
//				break;
//			
//			case state_PI + 4:
//				if (uc == '?')
//					state += 1;
//				else if (uc == 0)
//					throw exception("Unexpected end of file, run-away processing instruction?");
//				break;
//			
//			case state_PI + 5:
//				if (uc == '>')
//					token = xml_PI;
//				else if (uc == 0)
//					throw exception("Unexpected end of file, run-away processing instruction?");
//				else
//					state -= 1;
//				break;
//
//			// scan for tags 
//			case state_TagStart:
//				if (uc == '<')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_TagStart + 1:
//				if (uc == '?')
//					state = state_TagStart + 20;
//				else if (uc == '!')
//					token += 1;
//				else if (is_name_start_char(uc))
//				{
//					retract();
//					token = xml_STag;
//				}
//				else
//					throw exception("Unexpected character following <");
//				break;
//			
//			// comment, doctype, entity, etc
//			case state_TagStart + 2:
//				if (uc == '-')						// comment
//					state += 1;
//				else if (is_name_start_char(uc))	// doctypedecl
//					state = state_TagStart + 30;
//				break;
//
//			// comment
//			case state_TagStart + 3:
//				if (uc == '-')
//					state += 1;
//				break;
//			
//			case state_TagStart + 4:
//				if (uc == '-')
//					state += 1;
//				else
//					state -= 1;
//				break;
//			
//			case state_TagStart + 5:
//				if (uc == '>')
//					token = xml_Comment;
//				else
//					throw exception("Invalid formatted comment");
//				break;
//			
//			// <!DOCTYPE and friends
//			case state_TagStart + 30:
//				if (not is_name_char(uc))
//				{
//					retract();
//					
//					if (m_token == L"<!DOCTYPE")
//						token = xml_DocType;
//					else if (m_token == L"<!ELEMENT")
//						token = xml_Element;
//					else if (m_token == L"<!ATTLIST")
//						token = xml_AttList;
//					else if (m_token == L"<!ENTITY")
//						token = xml_Entity;
//					else if (m_token == L"<!NOTATION")
//						token = xml_Notation;
//					else
//						throw exception("Unrecognized doctype declaration '%s'", m_token.c_str());
//				}
//				break;
//
//			// Names
//			case state_Name:
//				if (is_name_start_char(uc))
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_Name + 1:
//				if (not is_name_char(uc))
//				{
//					retract();
//					token = xml_Name;
//				}
//				break;
//			
//			// strings
//			case state_String:
//				if (uc == '"')
//					state += 10;
//				else if (uc == '\'')
//					state += 20;
//				else
//					restart(start, state);
//				break;
//
//			case state_String + 10:
//				if (uc == '"')
//					token = xml_String;
//				else if (uc == 0)
//					throw exception("unexpected end of file, runaway string");
//				break;
//
//			case state_String + 20:
//				if (uc == '\'')
//					token = xml_String;
//				else if (uc == 0)
//					throw exception("unexpected end of file, runaway string");
//				break;
//			
//			// parameter entity references
//			case state_PERef:
//				if (uc == '%')
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_PERef + 1:
//				if (is_name_start_char(uc))
//					state += 1;
//				else
//					restart(start, state);
//				break;
//			
//			case state_PERef + 2:
//				if (uc == ';')
//				{
//					m_token.erase(m_token.end() - 1);
//					m_token.erase(m_token.begin(), m_token.begin() + 1);
//					token = xml_PEReference;
//				}
//				else if (not is_name_char(uc))
//					restart(start, state);
//				break;
//			
//			// misc
//			case state_Misc:
//				if (uc == '?')
//					state += 1;
//				else
//					token = uc;
//				break;
//			
//			case state_Misc + 1:
//				if (uc == '>')
//					token = xml_PIEnd;
//				else
//				{
//					retract();
//					token = '?';
//				}
//				break;
//			
//			
//			
//		}
//	}
//	
//	return token;
//}
//
//void document_imp::parse()
//{
//	m_lookahead = get_next_token();
//	
//	// first parse the xmldecl
//	
//	prolog();
//	
//	while (m_lookahead != xml_Eof)
//	{
//		
//		
//		match(m_lookahead);
//	}
//}
//
//void document_imp::prolog()
//{
//	if (m_lookahead == xml_XMLDecl)	// <?
//		xml_decl();
//	
//	misc();
//
//	if (m_lookahead == xml_DocType)
//	{
//		doctypedecl();
//		misc();
//	}
//}
//
//void document_imp::xml_decl()
//{
//	match(xml_XMLDecl);
//	match(xml_Space);
//	if (m_token != L"version")
//		throw exception("expected a version attribute in XML declaration");
//	match(xml_Name);
//	eq();
//	m_version = boost::lexical_cast<float>(m_token);
//	if (m_version >= 2.0 or m_version < 1.0)
//		throw exception("This library only supports XML version 1.x");
//	match(xml_String);
//
//	while (m_lookahead == xml_Space)
//	{
//		match(xml_Space);
//		
//		if (m_lookahead != xml_Name)
//			break;
//		
//		if (m_token == L"encoding")
//		{
//			match(xml_Name);
//			eq();
//			ba::to_upper(m_token);
//			if (m_token == L"UTF-8")
//				m_encoding = enc_UTF8;
//			else if (m_token == L"UTF-16")
//			{
//				if (m_encoding != enc_UTF16LE and m_encoding != enc_UTF16BE)
//					throw exception("Inconsistent encoding attribute in XML declaration");
//			}
//			else if (m_token == L"ISO-8859-1")
//				m_encoding = enc_ISO88591;
//			match(xml_String);
//			continue;
//		}
//		
//		if (m_token == L"standalone")
//		{
//			match(xml_Name);
//			eq();
//			if (m_token != L"yes" and m_token != L"no")
//				throw exception("Invalid XML declaration, standalone value should be either yes or no");
//			if (m_token == L"no")
//				throw exception("Unsupported XML declaration, only standalone documents are supported");
//			m_standalone = m_token;
//			match(xml_String);
//			continue;
//		}
//		
//		throw exception("unexpected attribute in xml declaration");
//	}
//	
//	match(xml_PIEnd);
//}
//
//void document_imp::s()
//{
//	if (m_lookahead == xml_Space)
//		match(xml_Space);
//}
//
//void document_imp::eq()
//{
//	s();
//
//	match('=');
//
//	s();
//}
//
//void document_imp::misc()
//{
//	for (;;)
//	{
//		if (m_lookahead == xml_Space or m_lookahead == xml_Comment)
//		{
//			match(m_lookahead);
//			continue;
//		}
//		
//		if (m_lookahead == xml_PI)
//			match(xml_PI);
//		
//		break;
//	}	
//}
//
//void document_imp::doctypedecl()
//{
//	match(xml_DocType);
//	
//	match(xml_Space);
//	
//	wstring name = m_token;
//	match(xml_Name);
//
//	if (m_lookahead == xml_Space)
//	{
//		match(xml_Space);
//		
//		wstring e;
//		
//		if (m_lookahead == xml_Name)
//		{
//			e = external_id();
//			
//			if (not e.empty())
//			{
//				// push e
//				wstring replacement;
//				replacement += ' ';
//				replacement += e;
//				replacement += ' ';
//				
//				m_data.push(data_ptr(new wstring_data_source(replacement)));
//				
//				// parse e as if it was internal
//				intsubset();
//			}
//		}
//		
//		s();
//	}
//	
//	if (m_lookahead == '[')
//	{
//		match('[');
//		intsubset();
//		match(']');
//
//		s();
//	}
//	
//	match('>');
//}
//
//void document_imp::intsubset()
//{
//	while (m_lookahead != xml_Eof and m_lookahead != ']' and m_lookahead != '[')
//	{
//		switch (m_lookahead)
//		{
//			case xml_PEReference:
//			{
//				map<wstring,wstring>::iterator r = m_parameter_entities.find(m_token);
//				if (r == m_parameter_entities.end())
//					throw exception("undefined parameter entity %s", m_token.c_str());
//				
//				wstring replacement;
//				replacement += ' ';
//				replacement += r->second;
//				replacement += ' ';
//				
//				m_data.push(data_ptr(new wstring_data_source(replacement)));
//				break;
//			}
//			
//			case xml_Space:
//				match(xml_Space);
//				break;
//			
//			case xml_Element:
//				element_decl();
//				break;
//			
//			case xml_AttList:
//				attlist_decl();
//				break;
//			
//			case xml_Entity:
//				entity_decl();
//				break;
//
//			case xml_Notation:
//				notation_decl();
//				break;
//			
//			case xml_PI:
//				match(xml_PI);
//				break;
//			
//			
//		}
//	}
//	
//}
//
//void document_imp::element_decl()
//{
//	match(xml_Element);
//	match(xml_Space);
//	match(xml_Name);
//	match(xml_Space);
//	contentspec();
//}
//
//void document_imp::contentspec()
//{
//	if (m_lookahead == xml_Name)
//	{
//		if (m_token != L"EMPTY" and m_token != L"ANY")
//			throw exception("Invalid element content specification");
//		match(xml_Name);
//	}
//	else
//	{
//		match('(');
//		
//		if (m_lookahead == '#')	// Mixed
//		{
//			match(m_lookahead);
//			if (m_token != L"PCDATA")
//				throw exception("Invalid element content specification, expected #PCDATA");
//			match(xml_Name);
//			
//			s();
//			
//			while (m_lookahead == '|')
//			{
//				match('|');
//				s();
//				match(xml_Name);
//				s();
//			}
//			
//			match(')');
//			match('*');
//		}
//	}
//}
//
//void document_imp::entity_decl()
//{
//	match(xml_Entity);
//	match(xml_Space);
//
//	if (m_lookahead == '%')	// PEDecl
//		parameter_entity_decl();
//	else
//		general_entity_decl();
//}
//
//void document_imp::parameter_entity_decl()
//{
//	match('%');
//	
//	wstring name = m_token;
//	match(xml_Name);
//	
//	match(xml_Space);
//
//	wstring value;
//	
//	// PEDef is either a EntityValue...
//	if (m_lookahead == xml_String)
//	{
//		value = m_token;
//		match(xml_String);
//		parse_parameter_entity_declaration(value);
//	}
//	else
//	{
//		
//		
//		value = external_id();
//	}
//
//	if (m_lookahead == xml_Space)
//		match(xml_Space);
//	
//	match('>');
//	
//	if (m_parameter_entities.find(name) == m_parameter_entities.end())
//		m_parameter_entities[name] = value;
//}
//
//void document_imp::general_entity_decl()
//{
//	wstring name = m_token;
//	match(xml_Name);
//	match(xml_Space);
//	
//	wstring value;
//
//	if (m_lookahead == xml_String)
//	{
//		value = m_token;
//		match(xml_String);
//	
//		parse_general_entity_declaration(value);
//	}
//	else // ... or an ExternalID
//	{
//		value = external_id();
//
//		if (m_lookahead == xml_Space)
//		{
//			match(xml_Space);
//			if (m_lookahead == xml_String and m_token == L"NDATA")
//			{
//				match(xml_String);
//				match(xml_Space);
//				match(xml_Name);
//			}
//		}
//	}	
//	
//	if (m_lookahead == xml_Space)
//		match(xml_Space);
//	
//	match('>');
//	
//	if (m_general_entities.find(name) == m_general_entities.end())
//		m_general_entities[name] = value;
//}
//
//wstring document_imp::external_id()
//{
//	wstring result;
//	
//	if (m_token == L"SYSTEM")
//	{
//		match(xml_Name);
//		match(xml_Space);
//		
//		wstring sys = m_token;
//		
//		match(xml_String);
//		
//		// result = retrieve_external_dtd(sys);
//	}
//	else if (m_token == L"PUBLIC")
//	{
//		match(xml_Name);
//		match(xml_Space);
//		
//		wstring pub = m_token;
//		match(xml_String);
//		match(xml_Space);
//		wstring sys = m_token;
//		match(xml_String);
//		
//		// retrieve_external_dtd(sys, pub);
//	}
//	else
//		throw exception("Expected external id starting with either SYSTEM or PUBLIC");
//	
//	return result;
//}
//
//void document_imp::parse_parameter_entity_declaration(wstring& s)
//{
////	wstring result;
////	wstring::iterator i = s.begin();
////	
////	int state = 0;
////	
////	wchar_t uc;
////	
////	while (i != s.end())
////	{
////		wchar_t c = *i++;
////		
////		switch (state)
////		{
////			case 0:
////				if (c == '&')
////					state = 1;
////				else
////					result += c;
////				break;
////			
////			case 1:
////				if (c == '#')
////					state = 2;
////				else if (is_name_start_char(c))
////					state = 10;
////				break;
////			
////			case 2:
////				if (c == 'x')
////					state = 3;
////				else if (
////		}
////		
////	}
////	
//}
//
//void document_imp::parse_general_entity_declaration(wstring& s)
//{
////	wstring result;
////	wstring::iterator i = s.begin();
////	
////	int state = 0;
////	
////	wchar_t uc;
////	
////	while (i != s.end())
////	{
////		wchar_t c = *i++;
////		
////		switch (state)
////		{
////			case 0:
////				if (c == '&')
////					state = 1;
////				else
////					result += c;
////				break;
////			
////			case 1:
////				if (c == '#')
////					state = 2;
////				else if (is_name_start_char(c))
////					state = 10;
////				break;
////			
////			case 2:
////				if (c == 'x')
////					state = 3;
////				else if (
////		}
////		
////	}
////	
//}

// --------------------------------------------------------------------

struct document_imp2
{
					document_imp2(istream& data)
						: m_parser(data)
					{
//						m_parser.start_element_handler = boost::bind(&document_imp2::start_element_handler, this);
					}

	void			start_element_handler(const string& name, const attribute_list& attrs);
	
	node_ptr		root()					{ return m_root; }
	
	void			parse()					{}
	
	parser			m_parser;
	node_ptr		m_root;
};

// --------------------------------------------------------------------

//document::document(
//	istream&		data)
//	: impl(new document_imp(data))
//{
//	impl->parse();
//}

//document::document(
//	const string&	data)
//	: impl(new document_imp)
//{
//	stringstream s;
//	s.str(data);
//	impl->parse(s);
//}
//
//document::document(
//	node_ptr		data)
//	: impl(new document_imp)
//{
//	impl->root = data;
//}
					
//document::~document()
//{
//	delete impl;
//}
//
//node_ptr document::root() const
//{
//	return impl->m_root;
//}
//
//ostream& operator<<(ostream& lhs, const document& rhs)
//{
//	lhs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
//	
//	if (rhs.root())
//		rhs.root()->write(lhs, 0);
//
//	return lhs;
//}

document::document(
	istream&		data)
	: impl(NULL)
{
}

document::document(
	const string&	data)
	: impl(NULL)
{
}

document::document(
	node_ptr		data)
	: impl(NULL)
{
}
					
document::~document()
{
}

node_ptr document::root() const
{
	return node_ptr();
}

ostream& operator<<(ostream& lhs, const document& rhs)
{
	lhs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	
	if (rhs.root())
		rhs.root()->write(lhs, 0);

	return lhs;
}
	
} // xml
} // zeep
