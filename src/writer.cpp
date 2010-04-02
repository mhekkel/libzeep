//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include "zeep/xml/writer.hpp"
#include "zeep/exception.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

writer::writer(std::ostream& os)
	: m_os(os)
	, m_encoding(enc_UTF8)
	, m_version(1.0f)
	, m_write_xml_decl(true)
	, m_wrap(true)
	, m_collapse_empty(true)
	, m_escape_whitespace(false)
	, m_trim(false)
	, m_indent(2)
	, m_level(0)
{
}
				
writer::~writer()
{
}

void writer::write_xml_decl(bool standalone)
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

void writer::write_start_doctype(const string& root, const string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	if (not dtd.empty())
		m_os << " \"" << dtd << '"';
	m_os << " [";

	if (m_wrap)
		m_os << endl;
}

void writer::write_end_doctype()
{
	m_os << "]>";
	if (m_wrap)
		m_os << endl;
}

void writer::write_empty_doctype(const string& root, const string& dtd)
{
	m_os << "<!DOCTYPE " << root;
	if (not dtd.empty())
		m_os << " \"" << dtd << '"';
	m_os << "]>";

	if (m_wrap)
		m_os << endl;
}

void writer::write_notation(const string& name,
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

void writer::write_attribute(const string& name, const string& value)
{
	if (m_wrap == false)
		m_os << ' ';
	else
	{
		for (int i = 0; i < m_indent * (m_level + 1); ++i)
			m_os << ' ';
	}
	
	m_os << name << "=\"";
	
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
			default:	m_os << c;					last_is_space = false; break;
		}
	}
	
	m_os << '"';

	if (m_wrap)
		m_os << endl;
}

void writer::write_empty_element(const string& prefix, const string& name,
	const attribute_list& attrs)
{
	assert(prefix.empty());
	
	if (m_collapse_empty)
	{
		for (int i = 0; i < m_indent * m_level; ++i)
			m_os << ' ';
		
		m_os << '<';
		if (not prefix.empty())
			m_os << prefix << ':';
		m_os << name;
		
		for_each (attrs.begin(), attrs.end(),
			boost::bind(&writer::write_attribute, this,
				boost::bind(&pair<string,string>::first, _1),
				boost::bind(&pair<string,string>::second, _1)));
		
		m_os << "/>";
		
		if (m_wrap)
			m_os << endl;
	}
	else
	{
		write_start_element(prefix, name, attrs);
		write_end_element(prefix, name);
	}
}

void writer::write_start_element(const string& prefix,
					const string& name, const attribute_list& attrs)
{
	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';
		
	m_os << '<';
	if (not prefix.empty())
		m_os << prefix << ':';
	m_os << name;

	for_each (attrs.begin(), attrs.end(),
		boost::bind(&writer::write_attribute, this,
			boost::bind(&pair<string,string>::first, _1),
			boost::bind(&pair<string,string>::second, _1)));

	m_os << '>';
	
	if (m_wrap)
		m_os << endl;
	
	++m_level;
}

void writer::write_end_element(const string& prefix,
					const string& name)
{
	--m_level;

	for (int i = 0; i < m_indent * m_level; ++i)
		m_os << ' ';
	
	assert(prefix.empty());
	m_os << "</" << name << '>';
	
	if (m_wrap)
		m_os << endl;
}

void writer::write_comment(const string& text)
{
	m_os << "<!--" << text << "-->";
}

void writer::write_processing_instruction(const string& target,
					const string& text)
{
	m_os << "<?" << target << ' ' << text << "?>";
}

void writer::write_text(const string& text)
{
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
			default:	m_os << c;					last_is_space = false; break;
		}
	}
}

}
}
