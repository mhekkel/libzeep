//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <expat.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>

#include <boost/algorithm/string.hpp>

#include "zeep/xml/expat_doc.hpp"
#include "zeep/exception.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct expat_doc_imp
{
	node_ptr		root;

	stack<node_ptr>	cur;		// construction
	vector<pair<string,string> >
					namespaces;

	static void		XML_StartElementHandler(
						void*				userData,
						const XML_Char*		name,
						const XML_Char**	atts);

	static void		XML_EndElementHandler(
						void*				userData,
						const XML_Char*		name);

	static void		XML_CharacterDataHandler(
						void*				userData,
						const XML_Char*		s,
						int					len);

	static void		XML_ProcessingInstructionHandler(
						void*				userData,
						const XML_Char*		target,
						const XML_Char*		data);

	static void		XML_CommentHandler(
						void*				userData,
						const XML_Char*		data);

	static void		XML_StartCdataSectionHandler(
						void *userData);

	static void		XML_EndCdataSectionHandler(
						void *userData);

	static void		XML_StartNamespaceDeclHandler(
                        void*				userData,
                        const XML_Char*		prefix,
                        const XML_Char*		uri);

	static void		XML_EndNamespaceDeclHandler(
						void*				userData,
						const XML_Char*		prefix);

	void			StartElementHandler(
						const XML_Char*		name,
						const XML_Char**	atts);

	void			EndElementHandler(
						const XML_Char*		name);

	void			CharacterDataHandler(
						const XML_Char*		s,
						int					len);

	void			ProcessingInstructionHandler(
						const XML_Char*		target,
						const XML_Char*		data);

	void			CommentHandler(
						const XML_Char*		data);

	void			StartCdataSectionHandler();

	void			EndCdataSectionHandler();

	void			StartNamespaceDeclHandler(
                        const XML_Char*		prefix,
                        const XML_Char*		uri);

	void			EndNamespaceDeclHandler(
						const XML_Char*		prefix);

	void			parse(
						istream&			data);


	void			parse_name(
						const char*			name,
						string&				element,
						string&				ns,
						string&				prefix);
};

// --------------------------------------------------------------------
//
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

// --------------------------------------------------------------------

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
		element = n3[1];
		ns = n3[0];
		prefix = n3[2];
	}
	else if (n3.size() == 2)
	{
		element = n3[1];
		ns = n3[0];
		prefix.clear();
		
		if (ns.empty() == false and not cur.empty())
			prefix = cur.top()->find_prefix(ns);
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
	string element, ns, prefix;
	
	parse_name(name, element, ns, prefix);

	node_ptr n;
	
	n.reset(new node(element, ns, prefix));

	if (cur.empty())
	{
		cur.push(n);
		root = cur.top();
	}
	else
	{
		cur.top()->add_child(n);
		cur.push(n);
	}
	
	for (const char** att = atts; *att; att += 2)
	{
		if (not att[0] or not att[1])
			break;
		
		parse_name(att[0], element, ns, prefix);
		if (not prefix.empty())
			element = prefix + ':' + element;
		
		attribute_ptr attr(new xml::attribute(element, att[1]));
		cur.top()->add_attribute(attr);
	}

	const string name_prefix("xmlns:");

	for (vector<pair<string,string> >::iterator ns = namespaces.begin(); ns != namespaces.end(); ++ns)
	{
		string name;
		if (ns->first.empty())
			name = "xmlns";
		else
			name = name_prefix + ns->first;

		attribute_ptr attr(new xml::attribute(name, ns->second));

		cur.top()->add_attribute(attr);
	}
	
	namespaces.clear();
}

void expat_doc_imp::EndElementHandler(
	const XML_Char*		name)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.pop();
}

void expat_doc_imp::CharacterDataHandler(
	const XML_Char*		s,
	int					len)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.top()->add_content(s, len);
}

void expat_doc_imp::ProcessingInstructionHandler(
	const XML_Char*		target,
	const XML_Char*		data)
{
//	cerr << "processing instruction, target: " << target << ", data: " << data << endl;
}

void expat_doc_imp::CommentHandler(
	const XML_Char*		data)
{
//	cerr << "comment " << data << endl;
}

void expat_doc_imp::StartCdataSectionHandler()
{
//	cerr << "start cdata" << endl;
}

void expat_doc_imp::EndCdataSectionHandler()
{
//	cerr << "end cdata" << endl;
}

void expat_doc_imp::StartNamespaceDeclHandler(
	const XML_Char*		prefix,
	const XML_Char*		uri)
{
	if (prefix == NULL)
		prefix = "";

	namespaces.push_back(make_pair(prefix, uri));
}
	
void expat_doc_imp::EndNamespaceDeclHandler(
	const XML_Char*		prefix)
{
}

// --------------------------------------------------------------------

void expat_doc_imp::parse(
	istream&		data)
{
	XML_Parser p = XML_ParserCreateNS(NULL, '=');
	
	if (p == NULL)
		throw exception("failed to create expat parser object");
	
	try
	{
		XML_SetParamEntityParsing(p, XML_PARAM_ENTITY_PARSING_ALWAYS);
		
		XML_SetUserData(p, this);
		XML_SetElementHandler(p, XML_StartElementHandler, XML_EndElementHandler);
		XML_SetCharacterDataHandler(p, XML_CharacterDataHandler);
//		XML_SetProcessingInstructionHandler(p, XML_ProcessingInstructionHandler);
//		XML_SetCommentHandler(p, XML_CommentHandler);
//		XML_SetCdataSectionHandler(p, XML_StartCdataSectionHandler, XML_EndCdataSectionHandler);
//		XML_SetDefaultHandler(p, XML_DefaultHandler);
//		XML_SetDoctypeDeclHandler(p, XML_StartDoctypeDeclHandler, XML_EndDoctypeDeclHandler);
//		XML_SetUnparsedEntityDeclHandler(p, XML_UnparsedEntityDeclHandler);
//		XML_SetNotationDeclHandler(p, XML_NotationDeclHandler);
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
				throw exception(p);
		}
	}
	catch (std::exception& e)
	{
		XML_ParserFree(p);
		throw;
	}

	XML_ParserFree(p);
}

expat_doc::expat_doc(
	istream&		data)
	: impl(new expat_doc_imp)
{
	impl->parse(data);
}
					
expat_doc::expat_doc(
	const string&	data)
	: impl(new expat_doc_imp)
{
	stringstream s;
	s.str(data);
	impl->parse(s);
}

expat_doc::expat_doc(
	node_ptr		data)
	: impl(new expat_doc_imp)
{
	impl->root = data;
}
					
expat_doc::~expat_doc()
{
	delete impl;
}

node_ptr expat_doc::root() const
{
	return impl->root;
}

ostream& operator<<(ostream& lhs, const expat_doc& rhs)
{
	lhs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	
	if (rhs.root())
		rhs.root()->write(lhs, 0);

	return lhs;
}
	
} // xml
} // zeep
