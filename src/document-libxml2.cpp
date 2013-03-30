//  Copyright Maarten L. Hekkelman, Radboud University 2010-2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>
#include <deque>
#include <map>

#include <boost/tr1/memory.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "document-imp.hpp"
#include <zeep/xml/document-libxml2.hpp>
#include <zeep/exception.hpp>

#include <libxml/xmlreader.h>

#include <zeep/xml/parser.hpp>
#include <zeep/xml/writer.hpp>

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct libxml2_doc_imp : public document_imp
{
					libxml2_doc_imp(document* doc);

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

	virtual void	parse(
						istream&			data);

	int				m_depth;
};

// --------------------------------------------------------------------

libxml2_doc_imp::libxml2_doc_imp(document* doc)
	: document_imp(doc)
	, m_depth(0)
{
}

void libxml2_doc_imp::StartElementHandler(
	xmlTextReaderPtr		inReader)
{
	const char* qname = (const char*)xmlTextReaderConstName(inReader);
	if (qname == nullptr)
		throw exception("nullptr qname");
	
	unique_ptr<element> n(new element(qname));

	if (m_cur == nullptr)
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
	else
		++m_depth;
}

void libxml2_doc_imp::EndElementHandler(
	xmlTextReaderPtr		inReader)
{
//	if (m_cur == nullptr)
//		throw exception("Empty stack");
//	
//	m_cur = dynamic_cast<element*>(m_cur->parent());
//	--m_depth;
	if (m_cur != nullptr)
	{
		m_cur = dynamic_cast<element*>(m_cur->parent());
		--m_depth;
	}
}

void libxml2_doc_imp::CharacterDataHandler(
	xmlTextReaderPtr		inReader)
{
	while (m_depth > 0 and m_depth != xmlTextReaderDepth(inReader))
	{
		m_cur = dynamic_cast<element*>(m_cur->parent());
		--m_depth;
	}
	
	if (m_cur == nullptr)
		throw exception("Empty stack");
	
	m_cur->add_text((const char*)xmlTextReaderConstValue(inReader));
}

void libxml2_doc_imp::ProcessingInstructionHandler(
	xmlTextReaderPtr		inReader)
{
	const char* target = (const char*)xmlTextReaderConstName(inReader);
	const char* data = (const char*)xmlTextReaderConstValue(inReader);
	
	if (m_cur != nullptr)
		m_cur->append(new processing_instruction(target, data));
	else
		m_root.append(new processing_instruction(target, data));
}

void libxml2_doc_imp::CommentHandler(
	xmlTextReaderPtr		inReader)
{
	const char* data = (const char*)xmlTextReaderConstValue(inReader);
	
	if (m_cur != nullptr)
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
		nullptr, 
		XML_PARSE_NOENT | XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR | XML_PARSE_XINCLUDE);

	if (reader != nullptr)
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
	
	if (m_validating and not valid)
		throw invalid_exception("document is not valid");
}

// --------------------------------------------------------------------

libxml2_document::libxml2_document()
	: document(new libxml2_doc_imp(this))
{
}

libxml2_document::libxml2_document(const string& s)
	: document(new libxml2_doc_imp(this))
{
	istringstream is(s);
	read(is);
}

libxml2_document::libxml2_document(istream& is)
	: document(new libxml2_doc_imp(this))
{
	read(is);
}

}
}
