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

#include "zeep/xml/document.hpp"
#include "zeep/exception.hpp"

#include "zeep/xml/parser.hpp"

using namespace std;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct document_imp
{
					document_imp();

	void			StartElementHandler(const string& name, const string& uri, const list<pair<string,string> >& atts);

	void			EndElementHandler(const string& name, const string& uri);

	void			CharacterDataHandler(const string& data);

	void			ProcessingInstructionHandler(const string& target, const string& data);

	void			CommentHandler(const string& comment);

	void			StartCdataSectionHandler();

	void			EndCdataSectionHandler();

	void			StartNamespaceDeclHandler(const string& prefix, const string& uri);

	void			EndNamespaceDeclHandler(const string& prefix);

	void			parse(istream& data);

	void			parse_name(const string& name, string& element, string& ns, string& prefix);
	
	string			prefix_for_ns(const string& ns);
	
	bool			find_external_dtd(const string& uri, fs::path& path);

	void			write(ostream& os);

	node_ptr		m_root;
	fs::path		m_dtd_dir;
	
	// some content information
	encoding_type	m_encoding;
	int				m_indent;
	bool			m_empty;
	bool			m_wrap;
	bool			m_trim;
	bool			m_escape_whitespace;

	stack<node_ptr>	m_cur;		// construction
	vector<pair<string,string> >
					m_namespaces;
};

// --------------------------------------------------------------------

document_imp::document_imp()
	: m_encoding(enc_UTF8)
	, m_indent(2)
	, m_empty(true)
	, m_wrap(true)
	, m_trim(true)
	, m_escape_whitespace(false)
{
}

void document_imp::parse_name(
	const string&		name,
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
		
		if (ns.empty() == false and not m_cur.empty())
			prefix = m_cur.top()->find_prefix(ns);
	}
	else
	{
		element = n3[0];
		ns.clear();
		prefix.clear();
	}
}

string document_imp::prefix_for_ns(const string& ns)
{
	vector<pair<string,string> >::iterator i = find_if(m_namespaces.begin(), m_namespaces.end(),
		boost::bind(&pair<string,string>::second, _1) == ns);
	
	string result;
	if (i != m_namespaces.end())
		result = i->first;
	return result;
}

void document_imp::StartElementHandler(const string& name, const string& uri, const list<pair<string,string> >& atts)
{
	node_ptr n;
	
	string prefix;
	if (not uri.empty())
		prefix = prefix_for_ns(uri);
	
	n.reset(new node(name, uri, prefix));

	if (m_cur.empty())
	{
		m_cur.push(n);
		m_root = m_cur.top();
	}
	else
	{
		m_cur.top()->add_child(n);
		m_cur.push(n);
	}
	
	for (list<pair<string,string> >::const_iterator ai = atts.begin(); ai != atts.end(); ++ai)
	{
		string element, ns, prefix;
		
		parse_name(ai->first, element, ns, prefix);
		if (not prefix.empty())
			element = prefix + ':' + element;
		
		attribute_ptr attr(new xml::attribute(element, ai->second));
		m_cur.top()->add_attribute(attr);
	}

	const string name_prefix("xmlns:");

	for (vector<pair<string,string> >::iterator ns = m_namespaces.begin(); ns != m_namespaces.end(); ++ns)
	{
		string name;
		if (ns->first.empty())
			name = "xmlns";
		else
			name = name_prefix + ns->first;

		attribute_ptr attr(new xml::attribute(name, ns->second));

		m_cur.top()->add_attribute(attr);
	}
	
	m_namespaces.clear();
}

void document_imp::EndElementHandler(const string& name, const string& uri)
{
	if (m_cur.empty())
		throw exception("Empty stack");
	
	m_cur.pop();
}

void document_imp::CharacterDataHandler(const string& data)
{
	if (m_cur.empty())
		throw exception("Empty stack");
	
	m_cur.top()->add_content(data);
}

void document_imp::ProcessingInstructionHandler(const string& target, const string& data)
{
//	cerr << "processing instruction, target: " << target << ", data: " << data << endl;
}

void document_imp::CommentHandler(const string& comment)
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

void document_imp::StartNamespaceDeclHandler(const string& prefix, const string& uri)
{
	m_namespaces.push_back(make_pair(prefix, uri));
}
	
void document_imp::EndNamespaceDeclHandler(const string& prefix)
{
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

	p.parse();
}

void document_imp::write(ostream& os)
{
	switch (m_encoding)
	{
		case enc_UTF8:
			os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
			break;
		default:
			assert(false);
	}
	
	if (m_wrap)
		os << endl;
	
	m_root->write(os, 0, m_indent, m_empty, m_wrap, m_trim, m_escape_whitespace);
}

// --------------------------------------------------------------------

document::document()
	: m_impl(new document_imp)
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

void document::write(ostream& os) const
{
	m_impl->write(os);
}

node_ptr document::root() const
{
	return m_impl->m_root;
}

void document::root(node_ptr root)
{
	m_impl->m_root = root;
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

istream& operator>>(istream& lhs, document& rhs)
{
	rhs.read(lhs);
	return lhs;
}

ostream& operator<<(ostream& lhs, const document& rhs)
{
	rhs.write(lhs);
	return lhs;
}
	
} // xml
} // zeep
