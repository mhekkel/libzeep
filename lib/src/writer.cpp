// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>

#include <iostream>

#include <zeep/xml/writer.hpp>
#include <zeep/exception.hpp>

namespace zeep { namespace xml {

std::string k_empty_string;

writer::writer(std::ostream& os)
	: m_os(os)
	, m_encoding(encoding_type::enc_UTF8)
	, m_version(1.0f)
	, m_write_xml_decl(false)
	, m_wrap(true)
	, m_wrap_prolog(true)
	, m_wrap_attributes(false)
	, m_collapse_empty(true)
	, m_escape_whitespace(false)
	, m_trim(false)
	, m_no_comment(false)
	, m_indent(2)
	, m_level(0)
	, m_element_open(false)
	, m_wrote_element(false)
	, m_prolog(true)
{
}
				
writer::writer(std::ostream& os, bool write_decl, bool standalone)
	: writer(os)
{
	if (write_decl)
	{
		m_write_xml_decl = true;
		xml_decl(standalone);
	}
}
				
writer::~writer()
{
}

void writer::xml_decl(bool standalone)
{
	if (m_write_xml_decl)
	{
		assert(m_encoding == encoding_type::enc_UTF8);

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
		
		if (m_wrap_prolog)
			m_os << std::endl;
		
		m_write_xml_decl = false;
	}
}

void writer::doctype(const std::string& root, const std::string& pubid, const std::string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	
	if (not pubid.empty())
		m_os << " PUBLIC \"" << pubid << "\"";
	else
		m_os << " SYSTEM";
	
	m_os << " \"" << dtd << "\">";

	if (m_wrap_prolog)
		m_os << std::endl;
}

void writer::start_doctype(const std::string& root, const std::string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	if (not dtd.empty())
		m_os << " \"" << dtd << '"';
	m_os << " [";

//	if (m_wrap_prolog)
		m_os << std::endl;
}

void writer::end_doctype()
{
	m_os << "]>";
//	if (m_wrap_prolog)
		m_os << std::endl;
}

void writer::empty_doctype(const std::string& root, const std::string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	if (not dtd.empty())
		m_os << " \"" << dtd << '"';
	m_os << ">";

//	if (m_wrap_prolog)
		m_os << std::endl;
}

void writer::notation(const std::string& name,
	const std::string& sysid, const std::string& pubid)
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
//	if (m_wrap_prolog)
		m_os << std::endl;
}

void writer::attribute(const std::string& name, const std::string& value)
{
	if (not m_element_open)
		throw exception("no open element to write attribute to");

	if ((m_wrap_attributes and m_level <= m_wrap_attributes_max_level) and m_indent_attr > 0)
	{
		m_os << std::endl;
		for (int i = 0; i < m_indent_attr; ++i)
			m_os << ' ';
	}
	else
		m_os << ' ';
	
	m_indent_attr = abs(m_indent_attr);
	
	m_os << name << "=\"";
	
	bool last_is_space = false;

	for (char c: value)
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
			default:	if ((c >= 1 and c <= 8) or (c >= 0x0b and c <= 0x0c) or (c >= 0x0e and c <= 0x1f) or c == 0x7f)
							m_os << "&#" << std::hex << c << ';';
						else	
							m_os << c;
						last_is_space = false;
						break;
		}
	}
	
	m_os << '"';
}

void writer::start_element(const std::string& qname)
{
	if (m_element_open)
	{
		m_os << '>';
		if (m_wrap)
			m_os << std::endl;
	}
	
	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';

	// initialize to negative value, to flag opening of element
	m_indent_attr = -(m_indent * m_level + 1 + qname.length() + 1);
		
	++m_level;
	
	m_os << '<' << qname;
	
	m_stack.push(qname);
	m_element_open = true;
	m_wrote_element = false;
	m_prolog = false;
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
		if ((m_wrap_attributes and m_level < m_wrap_attributes_max_level) and m_indent_attr > 0)
		{
			m_os << std::endl;
			for (int i = 0; i < m_indent_attr; ++i)
				m_os << ' ';
		}

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
		m_os << std::endl;

	m_stack.pop();
	m_element_open = false;
	m_wrote_element = true;
}

void writer::cdata(const std::string& text)
{
	if (m_element_open)
	{
		m_os << '>';
		if (m_wrap)
			m_os << std::endl;
	}

	m_element_open = false;

	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';

	m_os << "<![CDATA[" << text << "]]>";
	
	if (m_wrap)
		m_os << std::endl;
}

void writer::comment(const std::string& text)
{
	if (not m_no_comment)
	{
		if (m_element_open)
		{
			m_os << '>';
			if (m_wrap)
				m_os << std::endl;
		}

		m_element_open = false;
	
		for (int i = 0; i < m_indent * m_level; ++i)
			m_os << ' ';
	
//		m_os << "<!--" << text << "-->";
		m_os << "<!--";
		
		bool lastWasHyphen = false;
		for (char ch: text)
		{
			if (ch == '-' and lastWasHyphen)
				m_os << ' ';
			
			m_os << ch;
			lastWasHyphen = ch == '-';

			if (ch == '\n')
			{
				for (int i = 0; i < m_indent * m_level; ++i)
					m_os << ' ';
			}
		}
		
		m_os << "-->";
		
		if ((m_prolog and m_wrap_prolog) or (not m_prolog and m_wrap))
			m_os << std::endl;
	}
}

void writer::processing_instruction(const std::string& target,
					const std::string& text)
{
	if (m_element_open)
		m_os << '>';
	m_element_open = false;

	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';

	m_os << "<?" << target << ' ' << text << "?>";

	if ((m_prolog and m_wrap_prolog) or (not m_prolog and m_wrap))
		m_os << std::endl;
}

void writer::content(const std::string& text)
{
	if (m_element_open)
		m_os << '>';
	m_element_open = false;
	
	bool last_is_space = false;
	
	for (char c: text)
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
			default:	if ((c >= 1 and c <= 8) or (c >= 0x0b and c <= 0x0c) or (c >= 0x0e and c <= 0x1f) or c == 0x7f)
							m_os << "&#" << std::hex << c << ';';
						else	
							m_os << c;
						last_is_space = false;
						break;
		}
	}
	m_wrote_element = false;
}

}
}
