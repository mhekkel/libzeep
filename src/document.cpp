//  Copyright Maarten L. Hekkelman, Radboud University 2008.
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
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "zeep/xml/document.hpp"
#include "zeep/exception.hpp"

#include "zeep/xml/parser.hpp"
#include "zeep/xml/writer.hpp"

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct document_imp
{
					document_imp(document* doc);

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

	void			parse(istream& data);

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

	document*		m_doc;
	element*		m_cur;		// construction
	vector<pair<string,string> >
					m_namespaces;
	list<notation>	m_notations;
};

// --------------------------------------------------------------------

document_imp::document_imp(document* doc)
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
{
}

string document_imp::prefix_for_namespace(const string& ns)
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

void document_imp::StartElementHandler(const string& name, const string& uri,
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

	if (m_cur == NULL)
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

	const string name_prefix("xmlns:");

	for (vector<pair<string,string> >::iterator ns = m_namespaces.begin(); ns != m_namespaces.end(); ++ns)
		m_cur->set_name_space(ns->first, ns->second);
	
	m_namespaces.clear();
	
	n.release();
}

void document_imp::EndElementHandler(const string& name, const string& uri)
{
	if (m_cur == NULL)
		throw exception("Empty stack");
	
	m_cur = dynamic_cast<element*>(m_cur->parent());
}

void document_imp::CharacterDataHandler(const string& data)
{
	if (m_cur == NULL)
		throw exception("Empty stack");
	
	m_cur->add_text(data);
}

void document_imp::ProcessingInstructionHandler(const string& target, const string& data)
{
	if (m_cur != NULL)
		m_cur->append(new processing_instruction(target, data));
	else
		m_root.append(new processing_instruction(target, data));
}

void document_imp::CommentHandler(const string& s)
{
	if (m_cur != NULL)
		m_cur->append(new comment(s));
	else
		m_root.append(new comment(s));
}

void document_imp::StartCdataSectionHandler()
{
//	cerr << "start cdata" << endl;
}

void document_imp::EndCdataSectionHandler()
{
//	cerr << "end cdata" << endl;
}

void document_imp::StartNamespaceDeclHandler(const string& prefix, const string& uri)
{
	m_namespaces.push_back(make_pair(prefix, uri));
}
	
void document_imp::EndNamespaceDeclHandler(const string& prefix)
{
}

void document_imp::NotationDeclHandler(	const string& name, const string& sysid, const string& pubid)
{
	notation n = { name, sysid, pubid };
	
	list<notation>::iterator i = find_if(m_notations.begin(), m_notations.end(),
		boost::bind(&notation::m_name, _1) >= name);
	
	m_notations.insert(i, n);
}

bool document_imp::find_external_dtd(const string& uri, fs::path& path)
{
	bool result = false;
	if (not m_dtd_dir.empty() and fs::exists(m_dtd_dir))
	{
		path = m_dtd_dir / uri;
		result = fs::exists(path);
	}
	return result;
}

// --------------------------------------------------------------------

void document_imp::parse(
	istream&		data)
{
	parser p(data);

	p.start_element_handler = boost::bind(&document_imp::StartElementHandler, this, _1, _2, _3);
	p.end_element_handler = boost::bind(&document_imp::EndElementHandler, this, _1, _2);
	p.character_data_handler = boost::bind(&document_imp::CharacterDataHandler, this, _1);
	p.start_namespace_decl_handler = boost::bind(&document_imp::StartNamespaceDeclHandler, this, _1, _2);
	p.processing_instruction_handler = boost::bind(&document_imp::ProcessingInstructionHandler, this, _1, _2);
	p.notation_decl_handler = boost::bind(&document_imp::NotationDeclHandler, this, _1, _2, _3);

	p.parse(m_validating);
}

// --------------------------------------------------------------------

document::document()
	: m_impl(new document_imp(this))
{
}

document::document(const string& s)
	: m_impl(new document_imp(this))
{
	istringstream is(s);
	read(is);
}

document::document(std::istream& is)
	: m_impl(new document_imp(this))
{
	read(is);
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
	element* e = m_impl->m_root.child_element();
	
	if (e == NULL)
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

void document::set_validating(bool validate)
{
	m_impl->m_validating = validate;
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
