#include <expat.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>

#include <boost/algorithm/string.hpp>

#include "soap/xml/document.hpp"
#include "soap/exception.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace soap { namespace xml {

// --------------------------------------------------------------------

struct document_imp
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
};

// --------------------------------------------------------------------
//
// --------------------------------------------------------------------

void document_imp::XML_StartElementHandler(
	void*				userData,
	const XML_Char*		name,
	const XML_Char**	atts)
{
	assert(name);
	static_cast<document_imp*>(userData)->StartElementHandler(name, atts);
}

void document_imp::XML_EndElementHandler(
	void*				userData,
	const XML_Char*		name)
{
	assert(name);
	static_cast<document_imp*>(userData)->EndElementHandler(name);
}

void document_imp::XML_CharacterDataHandler(
	void*				userData,
	const XML_Char*		s,
	int					len)
{
	assert(s);
	static_cast<document_imp*>(userData)->CharacterDataHandler(s, len);
}

void document_imp::XML_ProcessingInstructionHandler(
	void*				userData,
	const XML_Char*		target,
	const XML_Char*		data)
{
	assert(target);
	assert(data);
	static_cast<document_imp*>(userData)->ProcessingInstructionHandler(target, data);
}

void document_imp::XML_CommentHandler(
	void*				userData,
	const XML_Char*		data)
{
	assert(data);
	static_cast<document_imp*>(userData)->CommentHandler(data);
}

void document_imp::XML_StartCdataSectionHandler(
	void *userData)
{
	static_cast<document_imp*>(userData)->StartCdataSectionHandler();
}

void document_imp::XML_EndCdataSectionHandler(
	void *userData)
{
	static_cast<document_imp*>(userData)->EndCdataSectionHandler();
}

void document_imp::XML_StartNamespaceDeclHandler(
    void*				userData,
    const XML_Char*		prefix,
    const XML_Char*		uri)
{
	assert(uri);
	static_cast<document_imp*>(userData)->StartNamespaceDeclHandler(prefix ? prefix : "_", uri);
}

void document_imp::XML_EndNamespaceDeclHandler(
	void*				userData,
	const XML_Char*		prefix)
{
	static_cast<document_imp*>(userData)->EndNamespaceDeclHandler(prefix ? prefix : "_");
}

// --------------------------------------------------------------------

void document_imp::StartElementHandler(
	const XML_Char*		name,
	const XML_Char**	atts)
{
	vector<string> n3;
	ba::split(n3, name, ba::is_any_of("="));
	
	node_ptr n;
	
	if (n3.size() == 3)
		n.reset(new node(n3[1], n3[0], n3[2]));
	else if (n3.size() == 2)
		n.reset(new node(n3[1], n3[0]));
	else
		n.reset(new node(name));
	
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
		
		attribute_ptr attr(new xml::attribute(att[0], att[1]));
		cur.top()->add_attribute(attr);
	}

	const string name_prefix("xmlns:");

	for (vector<pair<string,string> >::iterator ns = namespaces.begin(); ns != namespaces.end(); ++ns)
	{
		attribute_ptr attr(new xml::attribute(name_prefix + ns->first, ns->second));
		cur.top()->add_attribute(attr);
	}
	
	namespaces.clear();
}

void document_imp::EndElementHandler(
	const XML_Char*		name)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.pop();
}

void document_imp::CharacterDataHandler(
	const XML_Char*		s,
	int					len)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.top()->add_content(s, len);
}

void document_imp::ProcessingInstructionHandler(
	const XML_Char*		target,
	const XML_Char*		data)
{
//	cerr << "processing instruction, target: " << target << ", data: " << data << endl;
}

void document_imp::CommentHandler(
	const XML_Char*		data)
{
//	cerr << "comment " << data << endl;
}

void document_imp::StartCdataSectionHandler()
{
//	cerr << "start cdata" << endl;
}

void document_imp::EndCdataSectionHandler()
{
//	cerr << "end cdata" << endl;
}

void document_imp::StartNamespaceDeclHandler(
	const XML_Char*		prefix,
	const XML_Char*		uri)
{
	if (prefix == NULL)
		prefix = "";
	
	namespaces.push_back(make_pair(prefix, uri));
}
	
void document_imp::EndNamespaceDeclHandler(
	const XML_Char*		prefix)
{
}

// --------------------------------------------------------------------

void document_imp::parse(
	istream&		data)
{
	XML_Parser p = XML_ParserCreateNS(NULL, '=');
	
	if (p == NULL)
		throw exception("failed to create expat parser object");
	
	try
	{
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
		
		while (not data.eof())
		{
			string line;
			getline(data, line);
			line += '\n';

			XML_Status err = XML_Parse(p, line.c_str(), line.length(), data.eof() or line.empty());
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

document::document(
	istream&		data)
	: impl(new document_imp)
{
	impl->parse(data);
}
					
document::document(
	const string&	data)
	: impl(new document_imp)
{
	istringstream s(data);
	impl->parse(s);
}

document::document(
	node_ptr		data)
	: impl(new document_imp)
{
	impl->root = data;
}
					
document::~document()
{
	delete impl;
}

node_ptr document::root() const
{
	return impl->root;
}

ostream& operator<<(ostream& lhs, const document& rhs)
{
	lhs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	
	if (rhs.root())
		rhs.root()->write(lhs, 0);

	return lhs;
}
	
} // xml
} // soap
