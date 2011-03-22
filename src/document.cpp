//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>
#include <deque>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <zeep/config.hpp>
#include "document-imp.hpp"
#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
#include <zeep/xml/writer.hpp>

#if SOAP_XML_HAS_EXPAT_SUPPORT
#include "document-expat.hpp"
#endif

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

document_imp::document_imp(document* doc)
	: m_encoding(enc_UTF8)
	, m_standalone(false)
	, m_indent(2)
	, m_empty(true)
	, m_wrap(true)
	, m_trim(true)
	, m_escape_whitespace(false)
	, m_no_comment(false)
	, m_validating(false)
	, m_preserve_cdata(false)
	, m_doc(doc)
	, m_cur(nil)
	, m_cdata(nil)
{
}

document_imp::~document_imp()
{
}

string document_imp::prefix_for_namespace(const string& ns)
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

istream* document_imp::external_entity_ref(const string& base, const string& pubid, const string& sysid)
{
	istream* result = nil;
	
	if (m_doc->external_entity_ref_handler)
		result = m_doc->external_entity_ref_handler(base, pubid, sysid);
	
	if (result == nil and not sysid.empty())
	{
		fs::path path;
		
		if (base.empty())
			path = sysid;
		else
			path = base + '/' + sysid;
		
		if (not fs::exists(path))
			path = m_dtd_dir / path;

		if (fs::exists(path))
			result = new fs::ifstream(m_dtd_dir / path, ios::binary);
	}
	
	return result;
}

// --------------------------------------------------------------------

struct zeep_document_imp : public document_imp
{
					zeep_document_imp(document* doc);

	void			StartElementHandler(const string& name, const string& uri,
						const parser::attr_list_type& atts);

	void			EndElementHandler(const string& name, const string& uri);

	void			CharacterDataHandler(const string& data);

	void			ProcessingInstructionHandler(const string& target, const string& data);

	void			CommentHandler(const string& comment);

	void			StartCdataSectionHandler();

	void			EndCdataSectionHandler();

	void			StartNamespaceDeclHandler(const string& prefix, const string& uri);

	void			EndNamespaceDeclHandler(const string& prefix);
	
	void			NotationDeclHandler(const string& name, const string& sysid, const string& pubid);

	virtual void	parse(istream& data);
};

// --------------------------------------------------------------------

zeep_document_imp::zeep_document_imp(document* doc)
	: document_imp(doc)
{
}

void zeep_document_imp::StartElementHandler(const string& name, const string& uri,
	const parser::attr_list_type& atts)
{
	string qname = name;
	if (not uri.empty())
	{
		string prefix = prefix_for_namespace(uri);
		if (not prefix.empty())
			qname = prefix + ':' + name;
	}

	auto_ptr<element> n(new element(qname));

	if (m_cur == nil)
		m_root.child_element(n.get());
	else
		m_cur->append(n.get());

	m_cur = n.release();
	
	foreach (const parser::attr_type& a, atts)
	{
		qname = a.m_name;
		if (not a.m_ns.empty())
			qname = prefix_for_namespace(a.m_ns) + ':' + a.m_name;
		
		m_cur->set_attribute(qname, a.m_value, a.m_id);
	}

	for (vector<pair<string,string> >::iterator ns = m_namespaces.begin(); ns != m_namespaces.end(); ++ns)
		m_cur->set_name_space(ns->first, ns->second);
	
	m_namespaces.clear();
}

void zeep_document_imp::EndElementHandler(const string& name, const string& uri)
{
	if (m_cur == nil)
		throw exception("Empty stack");
	
	if (m_cdata != nil)
		throw exception("CDATA section not closed");
	
	m_cur = dynamic_cast<element*>(m_cur->parent());
}

void zeep_document_imp::CharacterDataHandler(const string& data)
{
	if (m_cur == nil)
		throw exception("Empty stack");
	
	if (m_cdata != nil)
		m_cdata->append(data);
	else
		m_cur->add_text(data);
}

void zeep_document_imp::ProcessingInstructionHandler(const string& target, const string& data)
{
	if (m_cur != nil)
		m_cur->append(new processing_instruction(target, data));
	else
		m_root.append(new processing_instruction(target, data));
}

void zeep_document_imp::CommentHandler(const string& s)
{
	if (m_cur != nil)
		m_cur->append(new comment(s));
	else
		m_root.append(new comment(s));
}

void zeep_document_imp::StartCdataSectionHandler()
{
	if (m_cur == nil)
		throw exception("empty stack");
	
	if (m_cdata != nil)
		throw exception("Nested CDATA?");
	
	m_cdata = new cdata();
	m_cur->append(m_cdata);
}

void zeep_document_imp::EndCdataSectionHandler()
{
	m_cdata = nil;
}

void zeep_document_imp::StartNamespaceDeclHandler(const string& prefix, const string& uri)
{
	m_namespaces.push_back(make_pair(prefix, uri));
}
	
void zeep_document_imp::EndNamespaceDeclHandler(const string& prefix)
{
}

void zeep_document_imp::NotationDeclHandler(
	const string& name, const string& sysid, const string& pubid)
{
	notation n = { name, sysid, pubid };
	
	list<notation>::iterator i = find_if(m_notations.begin(), m_notations.end(),
		boost::bind(&notation::m_name, _1) >= name);
	
	m_notations.insert(i, n);
}

// --------------------------------------------------------------------

void zeep_document_imp::parse(
	istream&		data)
{
	parser p(data);

	p.start_element_handler = boost::bind(&zeep_document_imp::StartElementHandler, this, _1, _2, _3);
	p.end_element_handler = boost::bind(&zeep_document_imp::EndElementHandler, this, _1, _2);
	p.character_data_handler = boost::bind(&zeep_document_imp::CharacterDataHandler, this, _1);
	if (m_preserve_cdata)
	{
		p.start_cdata_section_handler = boost::bind(&zeep_document_imp::StartCdataSectionHandler, this);
		p.end_cdata_section_handler = boost::bind(&zeep_document_imp::EndCdataSectionHandler, this);
	}
	p.start_namespace_decl_handler = boost::bind(&zeep_document_imp::StartNamespaceDeclHandler, this, _1, _2);
	p.processing_instruction_handler = boost::bind(&zeep_document_imp::ProcessingInstructionHandler, this, _1, _2);
	p.comment_handler = boost::bind(&zeep_document_imp::CommentHandler, this, _1);
	p.notation_decl_handler = boost::bind(&zeep_document_imp::NotationDeclHandler, this, _1, _2, _3);
	p.external_entity_ref_handler = boost::bind(&document_imp::external_entity_ref, this, _1, _2, _3);

	p.parse(m_validating);
}

// --------------------------------------------------------------------

parser_type document::s_parser_type = parser_zeep;

document_imp* document::create_imp(document* doc)
{
	document_imp* impl;
	
#if SOAP_XML_HAS_EXPAT_SUPPORT
	if (s_parser_type == parser_expat)
		impl = new expat_doc_imp(doc);
	else if (s_parser_type == parser_zeep)
		impl = new zeep_document_imp(doc);
	else
		throw zeep::exception("invalid parser type specified");
#else
	impl = new zeep_document_imp(doc);
#endif
	
	return impl;
}

void document::set_parser_type(parser_type type)
{
	s_parser_type = type;
}

document::document()
	: m_impl(create_imp(this))
{
}

document::document(const string& s)
	: m_impl(create_imp(this))
{
	istringstream is(s);
	read(is);
}

document::document(std::istream& is)
	: m_impl(create_imp(this))
{
	read(is);
}

document::document(document_imp* impl)
	: m_impl(impl)
{
}

document::~document()
{
	delete m_impl;
}

void document::read(const string& s)
{
	istringstream is(s);
	read(is);
}

void document::read(istream& is)
{
	m_impl->parse(is);
}

void document::read(istream& is, const boost::filesystem::path& base_dir)
{
	m_impl->m_dtd_dir = base_dir;
	m_impl->parse(is);
}

void document::write(writer& w) const
{
//	w.set_xml_decl(true);
//	w.set_indent(m_impl->m_indent);
//	w.set_wrap(m_impl->m_wrap);
//	w.set_trim(m_impl->m_trim);
//	w.set_escape_whitespace(m_impl->m_escape_whitespace);
//	w.set_no_comment(m_impl->m_no_comment);
	
	element* e = m_impl->m_root.child_element();
	
	if (e == nil)
		throw exception("cannot write an empty XML document");
	
	w.xml_decl(m_impl->m_standalone);

	if (not m_impl->m_notations.empty())
	{
		w.start_doctype(e->qname(), "");
		foreach (const document_imp::notation& n, m_impl->m_notations)
			w.notation(n.m_name, n.m_sysid, n.m_pubid);
		w.end_doctype();
	}
	
	m_impl->m_root.write(w);
}

root_node* document::root() const
{
	return &m_impl->m_root;
}

element* document::child() const
{
	return m_impl->m_root.child_element();
}

void document::child(element* e)
{
	return m_impl->m_root.child_element(e);
}

element_set document::find(const std::string& path)
{
	return m_impl->m_root.find(path);
}

element* document::find_first(const std::string& path)
{
	return m_impl->m_root.find_first(path);
}

void document::base_dir(const fs::path& path)
{
	m_impl->m_dtd_dir = path;
}

encoding_type document::encoding() const
{
	return m_impl->m_encoding;
}

void document::encoding(encoding_type enc)
{
	m_impl->m_encoding = enc;
}

int document::indent() const
{
	return m_impl->m_indent;
}

void document::indent(int indent)
{
	m_impl->m_indent = indent;
}

bool document::wrap() const
{
	return m_impl->m_wrap;
}

void document::wrap(bool wrap)
{
	m_impl->m_wrap = wrap;
}

bool document::trim() const
{
	return m_impl->m_trim;
}

void document::trim(bool trim)
{
	m_impl->m_trim = trim;
}

bool document::no_comment() const
{
	return m_impl->m_no_comment;
}

void document::no_comment(bool no_comment)
{
	m_impl->m_no_comment = no_comment;
}

void document::set_validating(bool validate)
{
	m_impl->m_validating = validate;
}

void document::set_preserve_cdata(bool preserve_cdata)
{
	m_impl->m_preserve_cdata = preserve_cdata;
}

bool document::operator==(const document& other) const
{
	return m_impl->m_root.equals(&other.m_impl->m_root);
}

istream& operator>>(istream& lhs, document& rhs)
{
	rhs.read(lhs);
	return lhs;
}

ostream& operator<<(ostream& lhs, const document& rhs)
{
	writer w(lhs);
	
	rhs.write(w);
	return lhs;
}

} // xml
} // zeep
