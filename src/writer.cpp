//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include <zeep/xml/writer.hpp>
#include <zeep/exception.hpp>

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

string k_empty_string;

writer::writer(std::ostream& os)
	: m_os(os)
	, m_encoding(enc_UTF8)
	, m_version(1.0f)
	, m_write_xml_decl(false)
	, m_wrap(true)
	, m_collapse_empty(true)
	, m_escape_whitespace(false)
	, m_trim(false)
	, m_no_comment(false)
	, m_indent(2)
	, m_level(0)
	, m_element_open(false)
	, m_wrote_element(false)
{
}
				
writer::writer(std::ostream& os, bool write_decl, bool standalone)
	: m_os(os)
	, m_encoding(enc_UTF8)
	, m_version(1.0f)
	, m_write_xml_decl(write_decl)
	, m_wrap(true)
	, m_collapse_empty(true)
	, m_escape_whitespace(false)
	, m_trim(false)
	, m_no_comment(false)
	, m_indent(2)
	, m_level(0)
	, m_element_open(false)
	, m_wrote_element(false)
{
	if (m_write_xml_decl)
		xml_decl(standalone);
}
				
writer::~writer()
{
}

void writer::xml_decl(bool standalone)
{
	if (m_write_xml_decl)
	{
		assert(m_encoding == enc_UTF8);

		if (m_version == 1.0f)
			m_os << "<?xml version=\"1.0\"";
		else if (m_version == 1.1f)
			m_os << "<?xml version=\"1.1\"";
		else
			throw exception("don't know how to write this version of XML");
		
		m_os << " encoding=\"UTF-8\"";
		
		if (standalone)
			m_os << " standalone=\"yes\"";
		
		m_os << "?>";
		
		if (m_wrap)
			m_os << endl;
	}
}

void writer::doctype(const string& root, const string& pubid, const string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	
	if (not pubid.empty())
		m_os << " PUBLIC \"" << pubid << "\"";
	
	m_os << " \"" << dtd << "\">";

	if (m_wrap)
		m_os << endl;
}

void writer::start_doctype(const string& root, const string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	if (not dtd.empty())
		m_os << " \"" << dtd << '"';
	m_os << " [";

	if (m_wrap)
		m_os << endl;
}

void writer::end_doctype()
{
	m_os << "]>";
	if (m_wrap)
		m_os << endl;
}

void writer::empty_doctype(const string& root, const string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	if (not dtd.empty())
		m_os << " \"" << dtd << '"';
	m_os << "]>";

	if (m_wrap)
		m_os << endl;
}

void writer::notation(const string& name,
	const string& sysid, const string& pubid)
{
	m_os << "<!NOTATION " << name;
	if (not pubid.empty())
	{
		m_os << " PUBLIC \'" << pubid << '\'';
		if (not sysid.empty())
			m_os << " \'" << sysid << '\'';
	}
	else
		m_os << " SYSTEM \'" << sysid << '\'';
	m_os << '>';
	if (m_wrap)
		m_os << endl;
}

void writer::attribute(const string& name, const string& value)
{
	if (not m_element_open)
		throw exception("no open element to write attribute to");
	
	m_os << ' ' << name << "=\"";
	
	bool last_is_space = false;

	foreach (char c, value)
	{
		switch (c)
		{
			case '&':	m_os << "&amp;";			last_is_space = false; break;
			case '<':	m_os << "&lt;";				last_is_space = false; break;
			case '>':	m_os << "&gt;";				last_is_space = false; break;
			case '\"':	m_os << "&quot;";			last_is_space = false; break;
			case '\n':	if (m_escape_whitespace)	m_os << "&#10;"; else m_os << c; last_is_space = true; break;
			case '\r':	if (m_escape_whitespace)	m_os << "&#13;"; else m_os << c; last_is_space = false; break;
			case '\t':	if (m_escape_whitespace)	m_os << "&#9;"; else m_os << c; last_is_space = false; break;
			case ' ':	if (not m_trim or not last_is_space) m_os << ' '; last_is_space = true; break;
			case 0:		throw exception("Invalid null character in XML content");
			default:	m_os << c;					last_is_space = false; break;
		}
	}
	
	m_os << '"';
}

void writer::start_element(const string& qname)
{
	if (m_element_open)
	{
		m_os << '>';
		if (m_wrap)
			m_os << endl;
	}
	
	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';
		
	++m_level;

	m_os << '<' << qname;
	
	m_stack.push(qname);
	m_element_open = true;
	m_wrote_element = false;
}

void writer::end_element()
{
	assert(m_level > 0);
	assert(not m_stack.empty());
	
	if (m_level == 0 or m_stack.empty())
		throw exception("inconsistent state in xml::writer");
	
	--m_level;

	if (m_element_open)
	{
		if (m_collapse_empty)
			m_os << "/>";
		else
			m_os << "></" << m_stack.top() << '>';
		
	}
	else
	{
		if (m_wrote_element)
		{
			for (int i = 0; i < m_indent * m_level; ++i)
				m_os << ' ';
		}

		m_os << "</" << m_stack.top() << '>';
	}

	if (m_wrap)
		m_os << endl;

	m_stack.pop();
	m_element_open = false;
	m_wrote_element = true;
}

void writer::cdata(const string& text)
{
	if (m_element_open)
	{
		m_os << '>';
		if (m_wrap)
			m_os << endl;
	}

	m_element_open = false;

	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';

	m_os << "<![CDATA[" << text << "]]>";
	
	if (m_wrap)
		m_os << endl;
}

void writer::comment(const string& text)
{
	if (not m_no_comment)
	{
		if (m_element_open)
		{
			m_os << '>';
			if (m_wrap)
				m_os << endl;
		}

		m_element_open = false;
	
		for (int i = 0; i < m_indent * m_level; ++i)
			m_os << ' ';
	
//		m_os << "<!--" << text << "-->";
		m_os << "<!--";
		
		bool lastWasHyphen = false;
		foreach (char ch, text)
		{
			if (ch == '-' and lastWasHyphen)
				m_os << ' ';
			m_os << ch;
			lastWasHyphen = ch == '-';
		}
		
		m_os << "-->";
		
		if (m_wrap)
			m_os << endl;
	}
}

void writer::processing_instruction(const string& target,
					const string& text)
{
	if (m_element_open)
		m_os << '>';
	m_element_open = false;

	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';

	m_os << "<?" << target << ' ' << text << "?>";

	if (m_wrap)
		m_os << endl;
}

void writer::content(const string& text)
{
	if (m_element_open)
		m_os << '>';
	m_element_open = false;
	
	bool last_is_space = false;
	
	foreach (char c, text)
	{
		switch (c)
		{
			case '&':	m_os << "&amp;";			last_is_space = false; break;
			case '<':	m_os << "&lt;";				last_is_space = false; break;
			case '>':	m_os << "&gt;";				last_is_space = false; break;
			case '\"':	m_os << "&quot;";			last_is_space = false; break;
			case '\n':	if (m_escape_whitespace)	m_os << "&#10;"; else m_os << c; last_is_space = true; break;
			case '\r':	if (m_escape_whitespace)	m_os << "&#13;"; else m_os << c; last_is_space = false; break;
			case '\t':	if (m_escape_whitespace)	m_os << "&#9;"; else m_os << c; last_is_space = false; break;
			case ' ':	if (not m_trim or not last_is_space) m_os << ' '; last_is_space = true; break;
			case 0:		throw exception("Invalid null character in XML content");
			default:	m_os << c;					last_is_space = false; break;
		}
	}
	m_wrote_element = false;
}

}
}
