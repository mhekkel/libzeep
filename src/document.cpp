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
	node_ptr		m_root;
	fs::path		m_dtd_dir;

	stack<node_ptr>	cur;		// construction
	vector<pair<string,string> >
					namespaces;

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
};

// --------------------------------------------------------------------

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

string document_imp::prefix_for_ns(const string& ns)
{
	vector<pair<string,string> >::iterator i = find_if(namespaces.begin(), namespaces.end(),
		boost::bind(&pair<string,string>::second, _1) == ns);
	
	string result;
	if (i != namespaces.end())
		result = i->first;
	return result;
}

void document_imp::StartElementHandler(const string& name, const string& uri, const list<pair<string,string> >& atts)
{
cout << "name: " << name << endl
	 << "uri: " << uri << endl
	 << endl;

	node_ptr n;
	
	string prefix;
	if (not uri.empty())
		prefix = prefix_for_ns(uri);
	
	n.reset(new node(name, uri, prefix));

	if (cur.empty())
	{
		cur.push(n);
		m_root = cur.top();
	}
	else
	{
		cur.top()->add_child(n);
		cur.push(n);
	}
	
	for (list<pair<string,string> >::const_iterator ai = atts.begin(); ai != atts.end(); ++ai)
	{
		string element, ns, prefix;
		
		parse_name(ai->first, element, ns, prefix);
		if (not prefix.empty())
			element = prefix + ':' + element;
		
		attribute_ptr attr(new xml::attribute(element, ai->second));
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

void document_imp::EndElementHandler(const string& name, const string& uri)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.pop();
}

void document_imp::CharacterDataHandler(const string& data)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.top()->add_content(data);
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
	namespaces.push_back(make_pair(prefix, uri));
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

// --------------------------------------------------------------------

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
	stringstream s;
	s.str(data);
	impl->parse(s);
}

document::document(
	node_ptr		data)
	: impl(new document_imp)
{
	impl->m_root = data;
}
					
document::~document()
{
	delete impl;
}

node_ptr document::root() const
{
	return impl->m_root;
}

ostream& operator<<(ostream& lhs, const document& rhs)
{
	lhs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	
	if (rhs.root())
		rhs.root()->write(lhs, 0);

	return lhs;
}
	
} // xml
} // zeep
