//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <libxml/xmlreader.h>

#include "zeep/xml/unicode_support.hpp"
#include "zeep/xml/libxml2_doc.hpp"
#include "zeep/exception.hpp"
#include "zeep/xml/writer.hpp"
#include "zeep/xml/parser.hpp"

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct libxml2_doc_imp
{
					libxml2_doc_imp(libxml2_doc* doc);

	void			ProcessNode(
						xmlTextReaderPtr	reader);

	void			StartElementHandler(
						xmlTextReaderPtr	reader);

	void			EndElementHandler(
						xmlTextReaderPtr	reader);

	void			CharacterDataHandler(
						xmlTextReaderPtr	reader);

	void			ProcessingInstructionHandler(
						xmlTextReaderPtr	reader);

	void			CommentHandler(
						xmlTextReaderPtr	reader);

	static void		ErrorHandler(
						void*					arg,
						const char*				msg, 
						xmlParserSeverities		severity, 
						xmlTextReaderLocatorPtr	locator);

	void			parse(
						istream&			data);

	string			prefix_for_namespace(const string& ns);
	
	bool			find_external_dtd(const string& uri, fs::path& path);

	root_node		m_root;
	fs::path		m_dtd_dir;
	
	// some content information
	encoding_type	m_encoding;
	bool			m_standalone;
	int				m_indent;
	bool			m_empty;
	bool			m_wrap;
	bool			m_trim;
	bool			m_escape_whitespace;
	
	bool			m_validating;

	struct notation
	{
		string		m_name;
		string		m_sysid;
		string		m_pubid;
	};

	libxml2_doc*	m_doc;
	element*		m_cur;		// construction
	int				m_depth;
	vector<pair<string,string> >
					m_namespaces;
	list<notation>	m_notations;
};

// --------------------------------------------------------------------

libxml2_doc_imp::libxml2_doc_imp(libxml2_doc* doc)
	: m_encoding(enc_UTF8)
	, m_standalone(false)
	, m_indent(2)
	, m_empty(true)
	, m_wrap(true)
	, m_trim(true)
	, m_escape_whitespace(false)
	, m_validating(false)
	, m_doc(doc)
	, m_cur(NULL)
	, m_depth(0)
{
}

string libxml2_doc_imp::prefix_for_namespace(const string& ns)
{
	vector<pair<string,string> >::iterator i = find_if(m_namespaces.begin(), m_namespaces.end(),
		boost::bind(&pair<string,string>::second, _1) == ns);
	
	string result;
	if (i != m_namespaces.end())
		result = i->first;
	else if (m_cur != NULL)
		result = m_cur->prefix_for_namespace(ns);
	else
		throw exception("namespace not found: %s", ns.c_str());
	
	return result;
}

void libxml2_doc_imp::StartElementHandler(
	xmlTextReaderPtr		inReader)
{
	const char* qname = (const char*)xmlTextReaderConstName(inReader);
	if (qname == NULL)
		throw exception("nil qname");
	
	auto_ptr<element> n(new element(qname));

	if (m_cur == NULL)
		m_root.child_element(n.get());
	else
		m_cur->append(n.get());

	m_cur = n.release();
	
	unsigned int count = xmlTextReaderAttributeCount(inReader);
	for (unsigned int i = 0; i < count; ++i)
	{
		xmlTextReaderMoveToAttributeNo(inReader, i);
		m_cur->set_attribute(
			(const char*)xmlTextReaderConstName(inReader),
			(const char*)xmlTextReaderConstValue(inReader));
	}

	const string name_prefix("xmlns:");

	for (vector<pair<string,string> >::iterator ns = m_namespaces.begin(); ns != m_namespaces.end(); ++ns)
		m_cur->set_name_space(ns->first, ns->second);
	
	m_namespaces.clear();
	
	if (xmlTextReaderIsEmptyElement(inReader))
		EndElementHandler(inReader);
}

void libxml2_doc_imp::EndElementHandler(
	xmlTextReaderPtr		inReader)
{
	if (m_cur == NULL)
		throw exception("Empty stack");
	
	m_cur = dynamic_cast<element*>(m_cur->parent());
}

void libxml2_doc_imp::CharacterDataHandler(
	xmlTextReaderPtr		inReader)
{
	while (m_depth > 0 and m_depth != xmlTextReaderDepth(inReader))
	{
		m_cur = dynamic_cast<element*>(m_cur->parent());
		--m_depth;
	}
	
	if (m_cur == nil)
		throw exception("Empty stack");
	
	m_cur->add_text((const char*)xmlTextReaderConstValue(inReader));
}

void libxml2_doc_imp::ProcessingInstructionHandler(
	xmlTextReaderPtr		inReader)
{
	const char* target = (const char*)xmlTextReaderConstName(inReader);
	const char* data = (const char*)xmlTextReaderConstValue(inReader);
	
	if (m_cur != nil)
		m_cur->append(new processing_instruction(target, data));
	else
		m_root.append(new processing_instruction(target, data));
}

void libxml2_doc_imp::CommentHandler(
	xmlTextReaderPtr		inReader)
{
	const char* data = (const char*)xmlTextReaderConstValue(inReader);
	
	if (m_cur != nil)
		m_cur->append(new comment(data));
	else
		m_root.append(new comment(data));
}


void libxml2_doc_imp::ProcessNode(
	xmlTextReaderPtr reader)
{
	switch (xmlTextReaderNodeType(reader))
	{
		case XML_READER_TYPE_ELEMENT:
			StartElementHandler(reader);
			break;
		
		case XML_READER_TYPE_END_ELEMENT:
			EndElementHandler(reader);
			break;

		case XML_READER_TYPE_WHITESPACE:
		case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
		case XML_READER_TYPE_TEXT:
		case XML_READER_TYPE_CDATA:
			CharacterDataHandler(reader);
			break;

		case XML_READER_TYPE_PROCESSING_INSTRUCTION:
			ProcessingInstructionHandler(reader);
			break;

		case XML_READER_TYPE_COMMENT:
//			CommentHandler(reader);
			break;

		case XML_READER_TYPE_DOCUMENT:
//			cout << "document" << endl;
			break;

		case XML_READER_TYPE_DOCUMENT_TYPE:
			if (m_validating)
				xmlTextReaderSetParserProp(reader, XML_PARSER_VALIDATE, 1);
			break;

		case XML_READER_TYPE_DOCUMENT_FRAGMENT:
//			cout << "document fragment" << endl;
			break;

		case XML_READER_TYPE_NOTATION:
			cout << "notation" << endl;
			break;

		case XML_READER_TYPE_END_ENTITY:
			cout << "end entity" << endl;
			break;

		case XML_READER_TYPE_XML_DECLARATION:
			cout << "xml decl" << endl;
			break;

	}
}

void libxml2_doc_imp::ErrorHandler(
	void*					arg,
	const char*				msg, 
	xmlParserSeverities		severity, 
	xmlTextReaderLocatorPtr	locator)
{
	throw invalid_exception(msg);
}

// --------------------------------------------------------------------

void libxml2_doc_imp::parse(
	istream&		data)
{
	// get length of file:
	data.seekg(0, ios::end);
	size_t length = data.tellg();
	data.seekg(0, ios::beg);
	
	// allocate memory:
	vector<char> buffer(length);
	
	// read data as a block:
	data.read(&buffer[0], length);
	bool valid = true;

	xmlTextReaderPtr reader = xmlReaderForMemory(&buffer[0], length,
		(fs::current_path().string() + "/").c_str(),
		nil, 
		XML_PARSE_NOENT | XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR | XML_PARSE_XINCLUDE);

	if (reader != nil)
	{
		xmlTextReaderSetErrorHandler(reader, &libxml2_doc_imp::ErrorHandler, this);
		try
		{
			int ret = xmlTextReaderRead(reader);
			while (ret == 1)
			{
				ProcessNode(reader);
				ret = xmlTextReaderRead(reader);
			}
		}
		catch (...)
		{
			xmlFreeTextReader(reader);
			throw;
		}
		
		if (xmlTextReaderIsValid(reader) != 1)
			valid = false;
		
		xmlFreeTextReader(reader);
	}
	
	if (not valid)
		throw invalid_exception("document is not valid");
}

libxml2_doc::libxml2_doc()
	: m_impl(new libxml2_doc_imp(this))
{
}

libxml2_doc::libxml2_doc(const string& s)
	: m_impl(new libxml2_doc_imp(this))
{
	istringstream is(s);
	read(is);
}

libxml2_doc::libxml2_doc(std::istream& is)
	: m_impl(new libxml2_doc_imp(this))
{
	read(is);
}

libxml2_doc::~libxml2_doc()
{
	delete m_impl;
}

void libxml2_doc::read(const string& s)
{
	istringstream is(s);
	read(is);
}

void libxml2_doc::read(istream& is)
{
	m_impl->parse(is);
}

void libxml2_doc::read(istream& is, const boost::filesystem::path& base_dir)
{
	m_impl->m_dtd_dir = base_dir;
	m_impl->parse(is);
}

void libxml2_doc::write(writer& w) const
{
	element* e = m_impl->m_root.child_element();
	
	if (e == nil)
		throw exception("cannot write an empty XML libxml2_doc");
	
	w.xml_decl(m_impl->m_standalone);

	if (not m_impl->m_notations.empty())
	{
		w.start_doctype(e->qname(), "");
		foreach (const libxml2_doc_imp::notation& n, m_impl->m_notations)
			w.notation(n.m_name, n.m_sysid, n.m_pubid);
		w.end_doctype();
	}
	
	m_impl->m_root.write(w);
}

root_node* libxml2_doc::root() const
{
	return &m_impl->m_root;
}

element* libxml2_doc::child() const
{
	return m_impl->m_root.child_element();
}

void libxml2_doc::child(element* e)
{
	return m_impl->m_root.child_element(e);
}

element_set libxml2_doc::find(const std::string& path)
{
	return m_impl->m_root.find(path);
}

element* libxml2_doc::find_first(const std::string& path)
{
	return m_impl->m_root.find_first(path);
}

void libxml2_doc::base_dir(const fs::path& path)
{
	m_impl->m_dtd_dir = path;
}

encoding_type libxml2_doc::encoding() const
{
	return m_impl->m_encoding;
}

void libxml2_doc::encoding(encoding_type enc)
{
	m_impl->m_encoding = enc;
}

int libxml2_doc::indent() const
{
	return m_impl->m_indent;
}

void libxml2_doc::indent(int indent)
{
	m_impl->m_indent = indent;
}

bool libxml2_doc::wrap() const
{
	return m_impl->m_wrap;
}

void libxml2_doc::wrap(bool wrap)
{
	m_impl->m_wrap = wrap;
}

bool libxml2_doc::trim() const
{
	return m_impl->m_trim;
}

void libxml2_doc::trim(bool trim)
{
	m_impl->m_trim = trim;
}

void libxml2_doc::set_validating(bool validate)
{
	m_impl->m_validating = validate;
}

bool libxml2_doc::operator==(const libxml2_doc& other) const
{
	return m_impl->m_root.equals(&other.m_impl->m_root);
}

istream& operator>>(istream& lhs, libxml2_doc& rhs)
{
	rhs.read(lhs);
	return lhs;
}

ostream& operator<<(ostream& lhs, const libxml2_doc& rhs)
{
	writer w(lhs);
	
	rhs.write(w);
	return lhs;
}
	
} // xml
} // zeep
