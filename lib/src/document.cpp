// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <stack>
#include <deque>
#include <map>

#include <zeep/config.hpp>
#include "document-imp.hpp"
#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
#include <zeep/xml/writer.hpp>
#include <zeep/xml/xpath.hpp>

namespace zeep
{
namespace xml
{

// --------------------------------------------------------------------

document_imp::document_imp(document *doc)
	: m_encoding(encoding_type::enc_UTF8), m_standalone(false), m_indent(2), m_empty(true)
	, m_wrap(true), m_trim(true), m_escape_whitespace(false), m_no_comment(false)
	, m_validating(false), m_preserve_cdata(false), m_doc(doc), m_cur(nullptr), m_cdata(nullptr)
{
}

document_imp::~document_imp()
{
}

std::string document_imp::prefix_for_namespace(const std::string& ns)
{
	auto i = std::find_if(m_namespaces.begin(), m_namespaces.end(), [ns](auto& n) { return n.second == ns; });

	std::string result;
	if (i != m_namespaces.end())
		result = i->first;
	else if (m_cur != nullptr)
		result = m_cur->prefix_for_namespace(ns);
	else
		throw exception("namespace not found: %s", ns.c_str());

	return result;
}

std::istream *document_imp::external_entity_ref(const std::string& base, const std::string& pubid, const std::string& sysid)
{
	std::istream *result = nullptr;

	if (m_doc->external_entity_ref_handler)
		result = m_doc->external_entity_ref_handler(base, pubid, sysid);

	if (result == nullptr and not sysid.empty())
	{
		std::string path;

		if (base.empty())
			path = sysid;
		else
			path = base + '/' + sysid;

		auto fs = new std::ifstream(path, std::ios::binary);
		if (not fs->is_open())
			fs->open(m_dtd_dir + '/' + path, std::ios::binary);

		if (fs->is_open())
			result = fs;
		else
			delete fs;
	}

	return result;
}

// --------------------------------------------------------------------

struct zeep_document_imp : public document_imp
{
	zeep_document_imp(document *doc);

	virtual void StartElementHandler(const std::string& name, const std::string& uri,
									 const parser::attr_list_type& atts);

	virtual void EndElementHandler(const std::string& name, const std::string& uri);

	virtual void CharacterDataHandler(const std::string& data);

	virtual void ProcessingInstructionHandler(const std::string& target, const std::string& data);

	virtual void CommentHandler(const std::string& comment);

	virtual void StartCdataSectionHandler();

	virtual void EndCdataSectionHandler();

	virtual void StartNamespaceDeclHandler(const std::string& prefix, const std::string& uri);

	virtual void EndNamespaceDeclHandler(const std::string& prefix);

	virtual void NotationDeclHandler(const std::string& name, const std::string& sysid, const std::string& pubid);

	virtual void parse(std::istream& data);
};

// --------------------------------------------------------------------

zeep_document_imp::zeep_document_imp(document *doc)
	: document_imp(doc)
{
}

void zeep_document_imp::StartElementHandler(const std::string& name, const std::string& uri,
											const parser::attr_list_type& atts)
{
	std::string qname = name;
	if (not uri.empty())
	{
		std::string prefix = prefix_for_namespace(uri);
		if (not prefix.empty())
			qname = prefix + ':' + name;
	}

	std::unique_ptr<element> n(new element(qname));

	if (m_cur == nullptr)
		m_root.child_element(n.get());
	else
		m_cur->append(n.get());

	m_cur = n.release();

	for (const parser::attr_type& a : atts)
	{
		qname = a.m_name;
		if (not a.m_ns.empty())
			qname = prefix_for_namespace(a.m_ns) + ':' + a.m_name;

		m_cur->set_attribute(qname, a.m_value, a.m_id);
	}

	for (auto ns = m_namespaces.begin(); ns != m_namespaces.end(); ++ns)
		m_cur->set_name_space(ns->first, ns->second);

	m_namespaces.clear();
}

void zeep_document_imp::EndElementHandler(const std::string& name, const std::string& uri)
{
	if (m_cur == nullptr)
		throw exception("Empty stack");

	if (m_cdata != nullptr)
		throw exception("CDATA section not closed");

	m_cur = dynamic_cast<element *>(m_cur->parent());
}

void zeep_document_imp::CharacterDataHandler(const std::string& data)
{
	if (m_cur == nullptr)
		throw exception("Empty stack");

	if (m_cdata != nullptr)
		m_cdata->append(data);
	else
		m_cur->add_text(data);
}

void zeep_document_imp::ProcessingInstructionHandler(const std::string& target, const std::string& data)
{
	if (m_cur != nullptr)
		m_cur->append(new processing_instruction(target, data));
	else
		m_root.append(new processing_instruction(target, data));
}

void zeep_document_imp::CommentHandler(const std::string& s)
{
	if (m_cur != nullptr)
		m_cur->append(new comment(s));
	else
		m_root.append(new comment(s));
}

void zeep_document_imp::StartCdataSectionHandler()
{
	if (m_cur == nullptr)
		throw exception("empty stack");

	if (m_cdata != nullptr)
		throw exception("Nested CDATA?");

	m_cdata = new cdata();
	m_cur->append(m_cdata);
}

void zeep_document_imp::EndCdataSectionHandler()
{
	m_cdata = nullptr;
}

void zeep_document_imp::StartNamespaceDeclHandler(const std::string& prefix, const std::string& uri)
{
	m_namespaces.push_back(make_pair(prefix, uri));
}

void zeep_document_imp::EndNamespaceDeclHandler(const std::string& prefix)
{
}

void zeep_document_imp::NotationDeclHandler(
	const std::string& name, const std::string& sysid, const std::string& pubid)
{
	notation n = {name, sysid, pubid};

	auto i = find_if(m_notations.begin(), m_notations.end(),
					 [name](auto& nt) { return nt.m_name >= name; });

	m_notations.insert(i, n);
}

// --------------------------------------------------------------------

void zeep_document_imp::parse(
	std::istream& data)
{
	parser p(data);

	using std::placeholders::_1;
	using std::placeholders::_2;
	using std::placeholders::_3;

	p.start_element_handler = std::bind(&zeep_document_imp::StartElementHandler, this, _1, _2, _3);
	p.end_element_handler = std::bind(&zeep_document_imp::EndElementHandler, this, _1, _2);
	p.character_data_handler = std::bind(&zeep_document_imp::CharacterDataHandler, this, _1);
	if (m_preserve_cdata)
	{
		p.start_cdata_section_handler = std::bind(&zeep_document_imp::StartCdataSectionHandler, this);
		p.end_cdata_section_handler = std::bind(&zeep_document_imp::EndCdataSectionHandler, this);
	}
	p.start_namespace_decl_handler = std::bind(&zeep_document_imp::StartNamespaceDeclHandler, this, _1, _2);
	p.processing_instruction_handler = std::bind(&zeep_document_imp::ProcessingInstructionHandler, this, _1, _2);
	p.comment_handler = std::bind(&zeep_document_imp::CommentHandler, this, _1);
	p.notation_decl_handler = std::bind(&zeep_document_imp::NotationDeclHandler, this, _1, _2, _3);
	p.external_entity_ref_handler = std::bind(&document_imp::external_entity_ref, this, _1, _2, _3);

	p.parse(m_validating);
}

// --------------------------------------------------------------------

document::document()
	: m_impl(new zeep_document_imp(this))
{
}

document::document(document&& other)
	: m_impl(other.m_impl)
{
	other.m_impl = nullptr;
}

document& document::operator=(document&& other)
{
	std::swap(m_impl, other.m_impl);
	return *this;
}

document::document(const std::string& s)
	: m_impl(new zeep_document_imp(this))
{
	std::istringstream is(s);
	read(is);
}

document::document(std::istream& is)
	: m_impl(new zeep_document_imp(this))
{
	read(is);
}

document::document(std::istream& is, const std::string& base_dir)
	: m_impl(new zeep_document_imp(this))
{
	read(is, base_dir);
}

document::document(document_imp *impl)
	: m_impl(impl)
{
}

document::~document()
{
	delete m_impl;
}

void document::read(const std::string& s)
{
	std::istringstream is(s);
	read(is);
}

void document::read(std::istream& is)
{
	m_impl->parse(is);
}

void document::read(std::istream& is, const std::string& base_dir)
{
	set_validating(true);
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

	element *e = m_impl->m_root.child_element();

	if (e == nullptr)
		throw exception("cannot write an empty XML document");

	w.xml_decl(m_impl->m_standalone);

	if (not m_impl->m_doctype.m_root.empty())
		w.doctype(m_impl->m_doctype.m_root, m_impl->m_doctype.m_pubid, m_impl->m_doctype.m_dtd);

	if (not m_impl->m_notations.empty())
	{
		w.start_doctype(e->qname(), "");
		for (const document_imp::notation& n : m_impl->m_notations)
			w.notation(n.m_name, n.m_sysid, n.m_pubid);
		w.end_doctype();
	}

	m_impl->m_root.write(w);
}

root_node *document::root() const
{
	return& m_impl->m_root;
}

element *document::child() const
{
	return m_impl->m_root.child_element();
}

void document::child(element *e)
{
	return m_impl->m_root.child_element(e);
}

element_set document::find(const char *path) const
{
	return m_impl->m_root.find(path);
}

element *document::find_first(const char *path) const
{
	return m_impl->m_root.find_first(path);
}

void document::find(const char *path, node_set& nodes) const
{
	m_impl->m_root.find(path, nodes);
}

void document::find(const char *path, element_set& elements) const
{
	m_impl->m_root.find(path, elements);
}

node *document::find_first_node(const char *path) const
{
	return m_impl->m_root.find_first_node(path);
}

void document::base_dir(const std::string& path)
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

void document::set_doctype(const std::string& root, const std::string& pubid, const std::string& dtd)
{
	m_impl->m_doctype.m_root = root;
	m_impl->m_doctype.m_pubid = pubid;
	m_impl->m_doctype.m_dtd = dtd;
}

bool document::operator==(const document& other) const
{
	return m_impl->m_root.equals(&other.m_impl->m_root);
}

std::istream& operator>>(std::istream& lhs, document& rhs)
{
	rhs.read(lhs);
	return lhs;
}

std::ostream& operator<<(std::ostream& lhs, const document& rhs)
{
	writer w(lhs);

	rhs.write(w);
	return lhs;
}

// --------------------------------------------------------------------
//
//	A visitor for elements that match the element_xpath.
//

struct visitor_imp : public zeep_document_imp
{
	visitor_imp(document *doc, const std::string& element_xpath,
				std::function<bool(node *doc_root, element *e)> cb);

	void EndElementHandler(const std::string& name, const std::string& uri);

	xpath mElementXPath;
	std::function<bool(node *doc_root, element *e)>
		mCallback;
};

// --------------------------------------------------------------------

visitor_imp::visitor_imp(document *doc, const std::string& element_xpath,
						 std::function<bool(node *doc_root, element *e)> cb)
	: zeep_document_imp(doc), mElementXPath(element_xpath), mCallback(cb)
{
}

void visitor_imp::EndElementHandler(const std::string& name, const std::string& uri)
{
	if (m_cur == nullptr)
		throw exception("Empty stack");

	if (m_cdata != nullptr)
		throw exception("CDATA section not closed");

	// see if we need to process this one
	if (mElementXPath.matches(m_cur))
	{
		if (not mCallback(m_root.child(), m_cur))
			; // TODO stop processing, how?

		element *e = m_cur;
		m_cur = dynamic_cast<element *>(m_cur->parent());
		if (m_cur == nullptr)
			m_root.child_element(nullptr);
		else
		{
			m_cur->remove(e);
			delete e;
		}
	}
	else
		m_cur = dynamic_cast<element *>(m_cur->parent());
}

void process_document_elements(std::istream& data, const std::string& element_xpath,
							   std::function<bool(node *doc_root, element *e)> cb)
{
	document doc(new visitor_imp(nullptr, element_xpath, cb));
	doc.m_impl->m_doc = &doc;

	doc.m_impl->parse(data);
}

} // namespace xml
} // namespace zeep
