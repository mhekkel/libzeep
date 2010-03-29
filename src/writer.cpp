//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>

#include "zeep/xml/writer.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

writer::writer(std::ostream& os)
	: m_os(os)
{
}
				
writer::~writer()
{
}

void writer::write_xml_decl(float version, encoding_type enc)
{
	assert(version == 1.0f);
	assert(enc == enc_UTF8);
	m_os << "<?xml version='1.0' encoding='UTF-8'?>";
}

void writer::write_empty_element(const std::string& ns,
					const std::string& name, const attribute_list& attrs)
{
	assert(ns.empty());
	
	write_start_element(ns, name, attrs);
	write_end_element(ns, name);
//	m_os << '<' << name;
//	foreach (const attribute& attr, attrs)
//		m_os << ' ' << attr.name() << "='" << attr.value() << '\'';
//	m_os << "/>";
}

void writer::write_start_element(const std::string& ns,
					const std::string& name, const attribute_list& attrs)
{
	bool escape_whitespace = true;

	assert(ns.empty());
	
	m_os << '<' << name;
	foreach (const attribute& attr, attrs)
//		m_os << ' ' << attr.name() << "=\"" << attr.value() << '"';
	{
		string value = attr.value();
		
		ba::replace_all(value, "&", "&amp;");
		ba::replace_all(value, "<", "&lt;");
		ba::replace_all(value, ">", "&gt;");
		ba::replace_all(value, "\"", "&quot;");
//		ba::replace_all(value, "'", "&apos;");
		if (escape_whitespace)
		{
			ba::replace_all(value, "\t", "&#9;");
			ba::replace_all(value, "\n", "&#10;");
			ba::replace_all(value, "\r", "&#13;");
		}

		m_os << ' ' << attr.name() << "=\"" << value << '"';
	}

	m_os << '>';
}

void writer::write_end_element(const std::string& ns,
					const std::string& name)
{
	assert(ns.empty());
	m_os << "</" << name << '>';
}

void writer::write_comment(const std::string& text)
{
	m_os << "<!--" << text << "-->";
}

void writer::write_processing_instruction(const std::string& target,
					const std::string& text)
{
	m_os << "<?" << target << ' ' << text << "?>";
}

void writer::write_text(const std::string& text)
{
	bool last_is_space = false;
	bool escape_whitespace = true;
	bool trim = false;
	
	foreach (char c, text)
	{
		switch (c)
		{
			case '&':	m_os << "&amp;";		last_is_space = false; break;
			case '<':	m_os << "&lt;";			last_is_space = false; break;
			case '>':	m_os << "&gt;";			last_is_space = false; break;
			case '\"':	m_os << "&quot;";		last_is_space = false; break;
			case '\n':	if (escape_whitespace)	m_os << "&#10;"; else m_os << c; last_is_space = true; break;
			case '\r':	if (escape_whitespace)	m_os << "&#13;"; else m_os << c; last_is_space = false; break;
			case '\t':	if (escape_whitespace)	m_os << "&#9;"; else m_os << c; last_is_space = false; break;
			case ' ':	if (not trim or not last_is_space) m_os << ' '; last_is_space = true; break;
			default:	m_os << c;				last_is_space = false; break;
		}
	}
}

}
}
