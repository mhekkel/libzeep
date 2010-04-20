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
#include <boost/filesystem/operations.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "zeep/xml/unicode_support.hpp"
#include "zeep/xml/expat_doc.hpp"
#include "zeep/exception.hpp"
#include "zeep/xml/writer.hpp"

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct expat_doc_imp
{
					expat_doc_imp(expat_doc* doc);

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

	static void		XML_NotationDeclHandler(
                        void*				userData,
                        const XML_Char*		notationName,
                        const XML_Char*		base,
                        const XML_Char*		systemId,
                        const XML_Char*		publicId);

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

	void			NotationDeclHandler(
                        const XML_Char*		notationName,
                        const XML_Char*		base,
                        const XML_Char*		systemId,
                        const XML_Char*		publicId);

	void			parse(
						istream&			data);


	void			parse_name(
						const char*			name,
						string&				element,
						string&				ns,
						string&				prefix);

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

	expat_doc*		m_doc;
	element*		m_cur;		// construction
	vector<pair<string,string> >
					m_namespaces;
	list<notation>	m_notations;
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

void expat_doc_imp::XML_NotationDeclHandler(
	void*				userData,
	const XML_Char*		notationName,
	const XML_Char*		base,
	const XML_Char*		systemId,
	const XML_Char*		publicId)
{
	static_cast<expat_doc_imp*>(userData)->NotationDeclHandler(notationName, base, systemId, publicId);
}
	
// --------------------------------------------------------------------

expat_doc_imp::expat_doc_imp(expat_doc* doc)
	: m_encoding(enc_UTF8)
	, m_standalone(false)
	, m_indent(2)
	, m_empty(true)
	, m_wrap(true)
	, m_trim(true)
	, m_escape_whitespace(false)
	, m_validating(false)
	, m_doc(doc)
	, m_cur(nil)
{
}

string expat_doc_imp::prefix_for_namespace(const string& ns)
{
	vector<pair<string,string> >::iterator i = find_if(m_namespaces.begin(), m_namespaces.end(),
		boost::bind(&pair<string,string>::second, _1) == ns);
	
	string result;
	if (i != m_namespaces.end())
		result = i->first;
	else if (m_cur != nil)
		result = m_cur->prefix_for_namespace(ns);
	else
		throw exception("namespace not found: %s", ns.c_str());
	
	return result;
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

	auto_ptr<element> n(new element(qname));

	if (m_cur == nil)
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
	if (m_cur == nil)
		throw exception("Empty stack");
	
	m_cur = dynamic_cast<element*>(m_cur->parent());
}

void expat_doc_imp::CharacterDataHandler(
	const XML_Char*		s,
	int					len)
{
	if (m_cur == nil)
		throw exception("Empty stack");
	
	m_cur->add_text(string(s, len));
}

void expat_doc_imp::ProcessingInstructionHandler(
	const XML_Char*		target,
	const XML_Char*		data)
{
	if (m_cur != nil)
		m_cur->append(new processing_instruction(target, data));
	else
		m_root.append(new processing_instruction(target, data));
}

void expat_doc_imp::CommentHandler(
	const XML_Char*		data)
{
	if (m_cur != nil)
		m_cur->append(new comment(data));
	else
		m_root.append(new comment(data));
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
	if (prefix == nil)
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
	notation n = { notationName, systemId, publicId };
	
	list<notation>::iterator i = find_if(m_notations.begin(), m_notations.end(),
		boost::bind(&notation::m_name, _1) >= notationName);
	
	m_notations.insert(i, n);
}

// --------------------------------------------------------------------

void expat_doc_imp::parse(
	istream&		data)
{
	XML_Parser p = XML_ParserCreateNS(nil, '=');
	
	if (p == nil)
		throw exception("failed to create expat parser object");
	
	try
	{
		XML_SetParamEntityParsing(p, XML_PARAM_ENTITY_PARSING_ALWAYS);
		
		XML_UseForeignDTD(p, true);
		
		XML_SetBase(p, fs::current_path().string().c_str());
		
		XML_SetUserData(p, this);
		XML_SetElementHandler(p, XML_StartElementHandler, XML_EndElementHandler);
		XML_SetCharacterDataHandler(p, XML_CharacterDataHandler);
		XML_SetProcessingInstructionHandler(p, XML_ProcessingInstructionHandler);
//		XML_SetCommentHandler(p, XML_CommentHandler);
//		XML_SetCdataSectionHandler(p, XML_StartCdataSectionHandler, XML_EndCdataSectionHandler);
//		XML_SetDefaultHandler(p, XML_DefaultHandler);
//		XML_SetDoctypeDeclHandler(p, XML_StartDoctypeDeclHandler, XML_EndDoctypeDeclHandler);
//		XML_SetUnparsedEntityDeclHandler(p, XML_UnparsedEntityDeclHandler);
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
				throw exception("expat error"/*p*/);
		}
	}
	catch (std::exception& e)
	{
		XML_ParserFree(p);
		throw;
	}

	XML_ParserFree(p);
}

expat_doc::expat_doc()
	: m_impl(new expat_doc_imp(this))
{
}

expat_doc::expat_doc(const string& s)
	: m_impl(new expat_doc_imp(this))
{
	istringstream is(s);
	read(is);
}

expat_doc::expat_doc(std::istream& is)
	: m_impl(new expat_doc_imp(this))
{
	read(is);
}

expat_doc::~expat_doc()
{
	delete m_impl;
}

void expat_doc::read(const string& s)
{
	istringstream is(s);
	read(is);
}

void expat_doc::read(istream& is)
{
	m_impl->parse(is);
}

void expat_doc::read(istream& is, const boost::filesystem::path& base_dir)
{
	m_impl->m_dtd_dir = base_dir;
	m_impl->parse(is);
}

void expat_doc::write(writer& w) const
{
	element* e = m_impl->m_root.child_element();
	
	if (e == nil)
		throw exception("cannot write an empty XML expat_doc");
	
	w.xml_decl(m_impl->m_standalone);

	if (not m_impl->m_notations.empty())
	{
		w.start_doctype(e->qname(), "");
		foreach (const expat_doc_imp::notation& n, m_impl->m_notations)
			w.notation(n.m_name, n.m_sysid, n.m_pubid);
		w.end_doctype();
	}
	
	m_impl->m_root.write(w);
}

root_node* expat_doc::root() const
{
	return &m_impl->m_root;
}

element* expat_doc::child() const
{
	return m_impl->m_root.child_element();
}

void expat_doc::child(element* e)
{
	return m_impl->m_root.child_element(e);
}

element_set expat_doc::find(const std::string& path)
{
	return m_impl->m_root.find(path);
}

element* expat_doc::find_first(const std::string& path)
{
	return m_impl->m_root.find_first(path);
}

void expat_doc::base_dir(const fs::path& path)
{
	m_impl->m_dtd_dir = path;
}

encoding_type expat_doc::encoding() const
{
	return m_impl->m_encoding;
}

void expat_doc::encoding(encoding_type enc)
{
	m_impl->m_encoding = enc;
}

int expat_doc::indent() const
{
	return m_impl->m_indent;
}

void expat_doc::indent(int indent)
{
	m_impl->m_indent = indent;
}

bool expat_doc::wrap() const
{
	return m_impl->m_wrap;
}

void expat_doc::wrap(bool wrap)
{
	m_impl->m_wrap = wrap;
}

bool expat_doc::trim() const
{
	return m_impl->m_trim;
}

void expat_doc::trim(bool trim)
{
	m_impl->m_trim = trim;
}

void expat_doc::set_validating(bool validate)
{
	m_impl->m_validating = validate;
}

bool expat_doc::operator==(const expat_doc& other) const
{
	return m_impl->m_root.equals(&other.m_impl->m_root);
}

istream& operator>>(istream& lhs, expat_doc& rhs)
{
	rhs.read(lhs);
	return lhs;
}

ostream& operator<<(ostream& lhs, const expat_doc& rhs)
{
	writer w(lhs);
	
	rhs.write(w);
	return lhs;
}
	
} // xml
} // zeep
