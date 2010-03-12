//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>

#include <boost/algorithm/string.hpp>

#include <libxml/parser.h>

#include "zeep/xml/libxml2_doc.hpp"
#include "zeep/exception.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct libxml2_doc_imp
{
	node_ptr		root;

	stack<node_ptr>	cur;		// construction
	vector<pair<string,string> >
					namespaces;

	static void		XML_StartElementHandler(
						void*				userData,
						const xmlChar*		name,
						const xmlChar**		atts);

	static void		XML_EndElementHandler(
						void*				userData,
						const xmlChar*		name);

	static void		XML_CharacterDataHandler(
						void*				userData,
						const xmlChar*		s,
						int					len);

	static void		XML_ProcessingInstructionHandler(
						void*				userData,
						const xmlChar*		target,
						const xmlChar*		data);

	static void		XML_CommentHandler(
						void*				userData,
						const xmlChar*		data);

	static void		XML_StartCdataSectionHandler(
						void *userData);

	static void		XML_EndCdataSectionHandler(
						void *userData);

	static void		XML_StartNamespaceDeclHandler(
                        void*				userData,
                        const xmlChar*		prefix,
                        const xmlChar*		uri);

	static void		XML_EndNamespaceDeclHandler(
						void*				userData,
						const xmlChar*		prefix);

	void			StartElementHandler(
						const xmlChar*		name,
						const xmlChar**	atts);

	void			EndElementHandler(
						const xmlChar*		name);

	void			CharacterDataHandler(
						const xmlChar*		s,
						int					len);

	void			ProcessingInstructionHandler(
						const xmlChar*		target,
						const xmlChar*		data);

	void			CommentHandler(
						const xmlChar*		data);

	void			StartCdataSectionHandler();

	void			EndCdataSectionHandler();

	void			StartNamespaceDeclHandler(
                        const xmlChar*		prefix,
                        const xmlChar*		uri);

	void			EndNamespaceDeclHandler(
						const xmlChar*		prefix);

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

void libxml2_doc_imp::XML_StartElementHandler(
	void*				userData,
	const xmlChar*		name,
	const xmlChar**	atts)
{
	assert(name);
	static_cast<libxml2_doc_imp*>(userData)->StartElementHandler(name, atts);
}

void libxml2_doc_imp::XML_EndElementHandler(
	void*				userData,
	const xmlChar*		name)
{
	assert(name);
	static_cast<libxml2_doc_imp*>(userData)->EndElementHandler(name);
}

void libxml2_doc_imp::XML_CharacterDataHandler(
	void*				userData,
	const xmlChar*		s,
	int					len)
{
	assert(s);
	static_cast<libxml2_doc_imp*>(userData)->CharacterDataHandler(s, len);
}

void libxml2_doc_imp::XML_ProcessingInstructionHandler(
	void*				userData,
	const xmlChar*		target,
	const xmlChar*		data)
{
	assert(target);
	assert(data);
	static_cast<libxml2_doc_imp*>(userData)->ProcessingInstructionHandler(target, data);
}

void libxml2_doc_imp::XML_CommentHandler(
	void*				userData,
	const xmlChar*		data)
{
	assert(data);
	static_cast<libxml2_doc_imp*>(userData)->CommentHandler(data);
}

void libxml2_doc_imp::XML_StartCdataSectionHandler(
	void *userData)
{
	static_cast<libxml2_doc_imp*>(userData)->StartCdataSectionHandler();
}

void libxml2_doc_imp::XML_EndCdataSectionHandler(
	void *userData)
{
	static_cast<libxml2_doc_imp*>(userData)->EndCdataSectionHandler();
}

void libxml2_doc_imp::XML_StartNamespaceDeclHandler(
    void*				userData,
    const xmlChar*		prefix,
    const xmlChar*		uri)
{
	assert(uri);
	static_cast<libxml2_doc_imp*>(userData)->StartNamespaceDeclHandler(prefix ? prefix : (const xmlChar*)"", uri);
}

void libxml2_doc_imp::XML_EndNamespaceDeclHandler(
	void*				userData,
	const xmlChar*		prefix)
{
	static_cast<libxml2_doc_imp*>(userData)->EndNamespaceDeclHandler(prefix ? prefix : (const xmlChar*)"");
}

// --------------------------------------------------------------------

void libxml2_doc_imp::parse_name(
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

void libxml2_doc_imp::StartElementHandler(
	const xmlChar*		name,
	const xmlChar**		atts)
{
	string element, ns, prefix;
	
	parse_name((const char*)name, element, ns, prefix);

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
	
	if (atts)
	{
		for (const unsigned char** att = atts; *att; att += 2)
		{
			if (not att[0] or not att[1])
				break;
			
			parse_name((const char*)att[0], element, ns, prefix);
			if (not prefix.empty())
				element = prefix + ':' + element;
			
			attribute_ptr attr(new xml::attribute(element, (const char*)att[1]));
			cur.top()->add_attribute(attr);
		}
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

void libxml2_doc_imp::EndElementHandler(
	const xmlChar*		name)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.pop();
}

void libxml2_doc_imp::CharacterDataHandler(
	const xmlChar*		s,
	int					len)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.top()->add_content((const char*)s, len);
}

void libxml2_doc_imp::ProcessingInstructionHandler(
	const xmlChar*		target,
	const xmlChar*		data)
{
//	cerr << "processing instruction, target: " << target << ", data: " << data << endl;
}

void libxml2_doc_imp::CommentHandler(
	const xmlChar*		data)
{
//	cerr << "comment " << data << endl;
}

void libxml2_doc_imp::StartCdataSectionHandler()
{
//	cerr << "start cdata" << endl;
}

void libxml2_doc_imp::EndCdataSectionHandler()
{
//	cerr << "end cdata" << endl;
}

void libxml2_doc_imp::StartNamespaceDeclHandler(
	const xmlChar*		prefix,
	const xmlChar*		uri)
{
	if (prefix == NULL)
		prefix = (const xmlChar*)"";

	namespaces.push_back(make_pair((const char*)prefix, (const char*)uri));
}
	
void libxml2_doc_imp::EndNamespaceDeclHandler(
	const xmlChar*		prefix)
{
}

// --------------------------------------------------------------------

void libxml2_doc_imp::parse(
	istream&		data)
{
	xmlSAXHandler myHandler = {};
	myHandler.startElement = XML_StartElementHandler;
	myHandler.endElement = XML_EndElementHandler; 
	myHandler.characters = XML_CharacterDataHandler; 
	
	// get length of file:
	data.seekg(0, ios::end);
	size_t length = data.tellg();
	data.seekg(0, ios::beg);
	
	// allocate memory:
	char* buffer = new char[length];
	
	// read data as a block:
	data.read(buffer,length);

	xmlSAXUserParseMemory(&myHandler, this, buffer, length);
	
	delete[] buffer;
}

libxml2_doc::libxml2_doc(
	istream&		data)
	: impl(new libxml2_doc_imp)
{
	impl->parse(data);
}
					
libxml2_doc::libxml2_doc(
	const string&	data)
	: impl(new libxml2_doc_imp)
{
	stringstream s;
	s.str(data);
	impl->parse(s);
}

libxml2_doc::libxml2_doc(
	node_ptr		data)
	: impl(new libxml2_doc_imp)
{
	impl->root = data;
}
					
libxml2_doc::~libxml2_doc()
{
	delete impl;
}

node_ptr libxml2_doc::root() const
{
	return impl->root;
}

ostream& operator<<(ostream& lhs, const libxml2_doc& rhs)
{
	lhs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	
	if (rhs.root())
		rhs.root()->write(lhs, 0);

	return lhs;
}
	
} // xml
} // zeep
