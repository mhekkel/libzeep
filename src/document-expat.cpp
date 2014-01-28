//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#if SOAP_XML_HAS_EXPAT_SUPPORT

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <zeep/xml/unicode_support.hpp>
#include "document-expat.hpp"
#include <zeep/exception.hpp>
#include <zeep/xml/writer.hpp>

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep { namespace xml {

const char* kXML_Parser_Error_Messages[] = {
	"NONE",
	"NO_MEMORY",
	"SYNTAX",
	"NO_ELEMENTS",
	"INVALID_TOKEN",
	"UNCLOSED_TOKEN",
	"PARTIAL_CHAR",
	"TAG_MISMATCH",
	"DUPLICATE_ATTRIBUTE",
	"JUNK_AFTER_DOC_ELEMENT",
	"PARAM_ENTITY_REF",
	"UNDEFINED_ENTITY",
	"RECURSIVE_ENTITY_REF",
	"ASYNC_ENTITY",
	"BAD_CHAR_REF",
	"BINARY_ENTITY_REF",
	"ATTRIBUTE_EXTERNAL_ENTITY_REF",
	"MISPLACED_XML_PI",
	"UNKNOWN_ENCODING",
	"INCORRECT_ENCODING",
	"UNCLOSED_CDATA_SECTION",
	"EXTERNAL_ENTITY_HANDLING",
	"NOT_STANDALONE",
	"UNEXPECTED_STATE",
	"ENTITY_DECLARED_IN_PE",
	"FEATURE_REQUIRES_XML_DTD",
	"CANT_CHANGE_FEATURE_ONCE_PARSING",
	"UNBOUND_PREFIX",
	"UNDECLARING_PREFIX",
	"INCOMPLETE_PE",
	"XML_DECL",
	"TEXT_DECL",
	"PUBLICID",
	"SUSPENDED",
	"NOT_SUSPENDED",
	"ABORTED",
	"FINISHED",
	"SUSPEND_PE",
	"RESERVED_PREFIX_XML",
	"RESERVED_PREFIX_XMLNS",
	"RESERVED_NAMESPACE_URI",
};
	
class expat_exception : public zeep::exception
{
  public:
					expat_exception(XML_Parser parser);
};

expat_exception::expat_exception(
	XML_Parser		parser)
	: exception("")
{
	try
	{
		stringstream s;
		
		XML_Error error = XML_GetErrorCode(parser);
		if (error <= XML_ERROR_RESERVED_NAMESPACE_URI)
			s << kXML_Parser_Error_Messages[error];
		else
			s << "Unknown Expat error code";
	
		s << endl
		  << "Parse error at line " << XML_GetCurrentLineNumber(parser)
		  << " column " << XML_GetCurrentColumnNumber(parser)
		  << ":" << endl;
		 
		int offset = 0, size = 0;
		const char* context = XML_GetInputContext(parser, &offset, &size);
		if (context != nullptr)
			s << string(context + offset, size) << endl;
	
		m_message = s.str();
	}
	catch (...)
	{
		m_message = "oeps";
	}
}

// --------------------------------------------------------------------

void expat_doc_imp::XML_StartElementHandler(
	void*				userData,
	const XML_Char*		name,
	const XML_Char**	atts)
{
	assert(name);
	static_cast<expat_doc_imp*>(userData)->StartElementHandler(name, atts);
}

void expat_doc_imp::XML_EndElementHandler(
	void*				userData,
	const XML_Char*		name)
{
	assert(name);
	static_cast<expat_doc_imp*>(userData)->EndElementHandler(name);
}

void expat_doc_imp::XML_CharacterDataHandler(
	void*				userData,
	const XML_Char*		s,
	int					len)
{
	assert(s);
	static_cast<expat_doc_imp*>(userData)->CharacterDataHandler(s, len);
}

void expat_doc_imp::XML_ProcessingInstructionHandler(
	void*				userData,
	const XML_Char*		target,
	const XML_Char*		data)
{
	assert(target);
	assert(data);
	static_cast<expat_doc_imp*>(userData)->ProcessingInstructionHandler(target, data);
}

void expat_doc_imp::XML_CommentHandler(
	void*				userData,
	const XML_Char*		data)
{
	assert(data);
	static_cast<expat_doc_imp*>(userData)->CommentHandler(data);
}

void expat_doc_imp::XML_StartCdataSectionHandler(
	void *userData)
{
	static_cast<expat_doc_imp*>(userData)->StartCdataSectionHandler();
}

void expat_doc_imp::XML_EndCdataSectionHandler(
	void *userData)
{
	static_cast<expat_doc_imp*>(userData)->EndCdataSectionHandler();
}

void expat_doc_imp::XML_StartNamespaceDeclHandler(
    void*				userData,
    const XML_Char*		prefix,
    const XML_Char*		uri)
{
	assert(uri);
	static_cast<expat_doc_imp*>(userData)->StartNamespaceDeclHandler(prefix ? prefix : "", uri);
}

void expat_doc_imp::XML_EndNamespaceDeclHandler(
	void*				userData,
	const XML_Char*		prefix)
{
	static_cast<expat_doc_imp*>(userData)->EndNamespaceDeclHandler(prefix ? prefix : "");
}

void expat_doc_imp::XML_NotationDeclHandler(
	void*				userData,
	const XML_Char*		notationName,
	const XML_Char*		base,
	const XML_Char*		systemId,
	const XML_Char*		publicId)
{
	static_cast<expat_doc_imp*>(userData)->NotationDeclHandler(notationName, base, systemId, publicId);
}

int expat_doc_imp::XML_ExternalEntityRefHandler(
    XML_Parser			parser,
    const XML_Char*		context,
    const XML_Char*		base,
    const XML_Char*		systemId,
    const XML_Char*		publicId)
{
	int result = XML_STATUS_OK;
	
	if (base != nullptr and systemId != nullptr)
	{
		fs::path basedir(base);
		fs::path file = basedir / systemId;
		if (fs::exists(file))
		{
			fs::ifstream data(file, ios::binary);
	
			XML_Parser entParser = XML_ExternalEntityParserCreate(parser, context, 0);
			XML_SetBase(entParser, file.string().c_str());
	
			data.seekg (0, ios::end);
			unsigned long length = data.tellg();
			data.seekg (0, ios::beg);
	
			while (length > 0)
			{
				char buffer[256];
	
				unsigned long k = length;
				if (k > sizeof(buffer))
					k = sizeof(buffer);
				length -= k;
				
				data.read(buffer, k);
				
				result = XML_Parse(entParser, buffer, k, length == 0);
				if (result != XML_STATUS_OK)
					break;
			}
			
			XML_ParserFree(entParser);
		}
	}
	
	return result;
}
	
// --------------------------------------------------------------------

expat_doc_imp::expat_doc_imp(document* doc)
	: document_imp(doc)
{
}

void expat_doc_imp::parse_name(
	const char*			name,
	string&				element,
	string&				ns,
	string&				prefix)
{
	vector<string> n3;
	ba::split(n3, name, ba::is_any_of("="));

	if (n3.size() == 3)
	{
		ns = n3[0];
		element = n3[1];
		prefix = n3[2];
	}
	else if (n3.size() == 2)
	{
		ns = n3[0];
		element = n3[1];
		if (not ns.empty())
			prefix = prefix_for_namespace(ns);
		else
			prefix.clear();
	}
	else
	{
		element = n3[0];
		ns.clear();
		prefix.clear();
	}
}

void expat_doc_imp::StartElementHandler(
	const XML_Char*		name,
	const XML_Char**	atts)
{
	string qname, uri, prefix;
	
	parse_name(name, qname, uri, prefix);
	
	if (not prefix.empty())
		qname = prefix + ':' + qname;

	unique_ptr<element> n(new element(qname));

	if (m_cur == nullptr)
		m_root.child_element(n.get());
	else
		m_cur->append(n.get());

	m_cur = n.release();
	
	for (const char** att = atts; *att; att += 2)
	{
		if (not att[0] or not att[1])
			break;
		
		parse_name(att[0], qname, uri, prefix);
		if (not prefix.empty())
			qname = prefix + ':' + qname;

#pragma message("need to find the ID here")		
		m_cur->set_attribute(qname, att[1], false);
	}

	const string name_prefix("xmlns:");

	for (vector<pair<string,string> >::iterator ns = m_namespaces.begin(); ns != m_namespaces.end(); ++ns)
		m_cur->set_name_space(ns->first, ns->second);
	
	m_namespaces.clear();
	
	n.release();
}

void expat_doc_imp::EndElementHandler(
	const XML_Char*		name)
{
	if (m_cur == nullptr)
		throw exception("Empty stack");
	
	m_cur = dynamic_cast<element*>(m_cur->parent());
}

void expat_doc_imp::CharacterDataHandler(
	const XML_Char*		s,
	int					len)
{
	if (m_cur == nullptr)
		throw exception("Empty stack");
	
	if (m_cdata != nullptr)
		m_cdata->append(data);
	else
		m_cur->add_text(string(s, len));
}

void expat_doc_imp::ProcessingInstructionHandler(
	const XML_Char*		target,
	const XML_Char*		data)
{
	if (m_cur != nullptr)
		m_cur->append(new processing_instruction(target, data));
	else
		m_root.append(new processing_instruction(target, data));
}

void expat_doc_imp::CommentHandler(
	const XML_Char*		data)
{
	if (m_cur != nullptr)
		m_cur->append(new comment(data));
	else
		m_root.append(new comment(data));
}

void expat_doc_imp::StartCdataSectionHandler()
{
	if (m_cur == nullptr)
		throw exception("empty stack");
	
	if (m_cdata != nullptr)
		throw exception("Nested CDATA?");
	
	m_cdata = new cdata();
	m_cur->append(m_cdata);
}

void expat_doc_imp::EndCdataSectionHandler()
{
	m_cdata = nullptr;
}

void expat_doc_imp::StartNamespaceDeclHandler(
	const XML_Char*		prefix,
	const XML_Char*		uri)
{
	if (prefix == nullptr)
		prefix = "";

	m_namespaces.push_back(make_pair(prefix, uri));
}
	
void expat_doc_imp::EndNamespaceDeclHandler(
	const XML_Char*		prefix)
{
}

void expat_doc_imp::NotationDeclHandler(
    const XML_Char*		notationName,
    const XML_Char*		base,
    const XML_Char*		systemId,
    const XML_Char*		publicId)
{
	notation n = {
		notationName ? notationName : "",
		systemId ? systemId : "",
		publicId ? publicId : ""
	};
	
	list<notation>::iterator i = find_if(m_notations.begin(), m_notations.end(),
		boost::bind(&notation::m_name, _1) >= notationName);
	
	m_notations.insert(i, n);
}

// --------------------------------------------------------------------

void expat_doc_imp::parse(
	istream&		data)
{
	XML_Parser p = XML_ParserCreateNS(nullptr, '=');
	
	if (p == nullptr)
		throw exception("failed to create expat parser object");
	
	try
	{
		XML_SetParamEntityParsing(p, XML_PARAM_ENTITY_PARSING_ALWAYS);
		XML_UseForeignDTD(p, true);
		XML_SetBase(p, (fs::current_path().string() + "/").c_str());
		
		XML_SetUserData(p, this);
		XML_SetElementHandler(p, XML_StartElementHandler, XML_EndElementHandler);
		XML_SetCharacterDataHandler(p, XML_CharacterDataHandler);
		XML_SetProcessingInstructionHandler(p, XML_ProcessingInstructionHandler);
		XML_SetCommentHandler(p, XML_CommentHandler);
//		XML_SetCdataSectionHandler(p, XML_StartCdataSectionHandler, XML_EndCdataSectionHandler);
//		XML_SetDefaultHandler(p, XML_DefaultHandler);
//		XML_SetDoctypeDeclHandler(p, XML_StartDoctypeDeclHandler, XML_EndDoctypeDeclHandler);
//		XML_SetUnparsedEntityDeclHandler(p, XML_UnparsedEntityDeclHandler);
		XML_SetExternalEntityRefHandler(p, XML_ExternalEntityRefHandler);
		XML_SetNotationDeclHandler(p, XML_NotationDeclHandler);
		XML_SetNamespaceDeclHandler(p, XML_StartNamespaceDeclHandler, XML_EndNamespaceDeclHandler);
		XML_SetReturnNSTriplet(p, true);

		// for some reason, readsome does not work when using
		// boost::iostreams::stream<boost::iostreams::array_source>
		// and so we have to come up with a kludge.
		
		data.seekg (0, ios::end);
		unsigned long length = data.tellg();
		data.seekg (0, ios::beg);

		while (length > 0)
		{
			char buffer[256];

			unsigned long k = length;
			if (k > sizeof(buffer))
				k = sizeof(buffer);
			length -= k;
			
			data.read(buffer, k);
			
			XML_Status err = XML_Parse(p, buffer, k, length == 0);
			if (err != XML_STATUS_OK)
				throw expat_exception(p);
		}
	}
	catch (std::exception& e)
	{
		XML_ParserFree(p);
		throw;
	}

	XML_ParserFree(p);
}

} // xml
} // zeep

#endif
