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
#include <typeindex>

#include <zeep/config.hpp>
#include <zeep/streambuf.hpp>
#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
#include <zeep/xml/document.hpp>

namespace zeep
{
namespace xml
{

// --------------------------------------------------------------------

document::document()
	: m_validating(false)
	, m_preserve_cdata(false)
	, m_has_xml_decl(false)
	, m_encoding(encoding_type::UTF8)
	, m_version(1.0f)
	, m_standalone(false)
{
}

document::document(const document& doc)
	: element(doc)
	, m_doctype(doc.m_doctype)
	, m_validating(doc.m_validating)
	, m_preserve_cdata(doc.m_preserve_cdata)
	, m_has_xml_decl(doc.m_has_xml_decl)
	, m_encoding(doc.m_encoding)
	, m_version(doc.m_version)
	, m_standalone(doc.m_standalone)
	, m_fmt(doc.m_fmt)
{
}

document::document(document&& doc)
	: element(doc)
	, m_doctype(doc.m_doctype)
	, m_validating(doc.m_validating)
	, m_preserve_cdata(doc.m_preserve_cdata)
	, m_has_xml_decl(doc.m_has_xml_decl)
	, m_encoding(doc.m_encoding)
	, m_version(doc.m_version)
	, m_standalone(doc.m_standalone)
	, m_fmt(doc.m_fmt)
{
}

document& document::operator=(const document& doc)
{
	element::operator=(doc);

	m_doctype = doc.m_doctype;
	m_validating = doc.m_validating;
	m_preserve_cdata = doc.m_preserve_cdata;
	m_fmt = doc.m_fmt;
	m_has_xml_decl = doc.m_has_xml_decl;
	m_encoding = doc.m_encoding;
	m_version = doc.m_version;
	m_standalone = doc.m_standalone;

	return *this;
}

document& document::operator=(document&& doc)
{
	element::operator=(doc);

	m_doctype = doc.m_doctype;
	m_validating = doc.m_validating;
	m_preserve_cdata = doc.m_preserve_cdata;
	m_fmt = doc.m_fmt;
	m_has_xml_decl = doc.m_has_xml_decl;
	m_encoding = doc.m_encoding;
	m_version = doc.m_version;
	m_standalone = doc.m_standalone;

	return *this;
}

document::document(const std::string& s)
	: document()
{
	std::istringstream is(s);
	parse(is);
}

document::document(std::istream& is)
	: document()
{
	parse(is);
}

document::document(std::istream& is, const std::string& base_dir)
	: document()
{
	m_validating = true;
	m_dtd_dir = base_dir;
	parse(is);
}

document::~document()
{
}

// --------------------------------------------------------------------

element::node_iterator document::insert_impl(const_iterator pos, node* n)
{
	if (not empty())
		throw zeep::exception("Cannot add a node to a non-empty document");
	return element::insert_impl(pos, n);
}

// --------------------------------------------------------------------

bool document::is_html5() const
{
	return
		m_doctype.m_root == "html" and
		m_nodes.front().name() == "html" and
		m_doctype.m_pubid == "" and
		m_doctype.m_dtd == "about:legacy-compat";
}

// --------------------------------------------------------------------

bool document::operator==(const document& other) const
{
	return equals(&other);
}

std::ostream& operator<<(std::ostream& os, const document& doc)
{
	auto fmt = doc.m_fmt;

	if (os.width() > 0)
	{
		fmt.indent_width = os.width();
		fmt.indent = true;
	}

	doc.write(os, fmt);
	return os;
}


std::istream& operator>>(std::istream& is, document& doc)
{
	doc.parse(is);
	return is;
}

void document::write(std::ostream& os, format_info fmt) const
{
	if (m_version > 1.0f)
	{
		assert(m_encoding == encoding_type::UTF8);

		if (m_version == 1.0f)
			os << "<?xml version=\"1.0\"";
		else if (m_version == 1.1f)
			os << "<?xml version=\"1.1\"";
		else
			throw exception("don't know how to write this version of XML");
		
		// os << " encoding=\"UTF-8\"";
		
		if (m_standalone)
			os << " standalone=\"yes\"";
		
		os << "?>";
		
		if (m_wrap_prolog)
			os << std::endl;
	}

	if (not m_notations.empty() or m_write_doctype)
	{
		os << "<!DOCTYPE " << (empty() ? "" : front().get_qname());
		if (m_write_doctype and not m_doctype.m_dtd.empty())
		{
			if (m_doctype.m_pubid.empty())
				os << " SYSTEM \"";
			else
				os << " PUBLIC \"" << m_doctype.m_pubid << "\" \"";
			os << m_doctype.m_dtd << '"';
		}
		
		if (not m_notations.empty())
		{
			os << " [" << std::endl;

			for (auto& [name, sysid, pubid]: m_notations)
			{
				os << "<!NOTATION " << name;
				if (not pubid.empty())
				{
					os << " PUBLIC \'" << pubid << '\'';
					if (not sysid.empty())
						os << " \'" << sysid << '\'';
				}
				else
					os << " SYSTEM \'" << sysid << '\'';
				os << '>' << std::endl;
			}
			os << "]";
		}

		os << ">" << std::endl;
	}

	for (auto& n: nodes())
		n.write(os, fmt);
}

// --------------------------------------------------------------------

void document::XmlDeclHandler(encoding_type encoding, bool standalone, float version)
{
	m_has_xml_decl = true;
	// m_encoding = encoding;
	m_standalone = standalone;
	m_version = version;

	m_fmt.version = version;
}

void document::StartElementHandler(const std::string& name, const std::string& uri, const parser::attr_list_type& atts)
{
	std::string qname = name;
	if (not uri.empty())
	{
		std::string prefix;
		bool found;

		auto i = std::find_if(m_namespaces.begin(), m_namespaces.end(),
			[uri](auto& ns) { return ns.second == uri; });

		if (i != m_namespaces.end())
		{
			prefix = i->first;
			found = true;
		}
		else
			std::tie(prefix, found) = m_cur->prefix_for_namespace(uri);
	
		if (prefix.empty() and not found)
			throw exception("namespace not found: " + uri);

		if (not prefix.empty())
			qname = prefix + ':' + name;
	}

	m_cur = &m_cur->emplace_back(qname);

	for (auto ns = m_namespaces.begin(); ns != m_namespaces.end(); ++ns)
	{
		if (ns->first.empty())
			m_cur->attributes().emplace("xmlns", ns->second);
		else
			m_cur->attributes().emplace("xmlns:" + ns->first, ns->second);
	}

	for (const parser::attr_type& a : atts)
	{
		qname = a.m_name;
		if (not a.m_ns.empty())
		{
			auto p = m_cur->prefix_for_namespace(a.m_ns);
			if (not p.second)
				throw exception("namespace not found: " + a.m_ns);
			 
			qname = p.first.empty() ? a.m_name : p.first + ':' + a.m_name;
		}

		m_cur->attributes().emplace(qname, a.m_value, a.m_id);
	}

	m_namespaces.clear();
}

void document::EndElementHandler(const std::string& name, const std::string& uri)
{
	if (m_cdata != nullptr)
		throw exception("CDATA section not closed");

	m_cur = m_cur->parent();
}

void document::CharacterDataHandler(const std::string& data)
{
	if (m_cdata != nullptr)
		m_cdata->append(data);
	else
		m_cur->add_text(data);
}

void document::ProcessingInstructionHandler(const std::string& target, const std::string& data)
{
	m_cur->nodes().emplace_back(processing_instruction(target, data));
}

void document::CommentHandler(const std::string& s)
{
	m_cur->nodes().emplace_back(comment(s));
}

void document::StartCdataSectionHandler()
{
	m_cdata = &m_cur->nodes().emplace_back(cdata());
}

void document::EndCdataSectionHandler()
{
	m_cdata = nullptr;
}

void document::StartNamespaceDeclHandler(const std::string& prefix, const std::string& uri)
{
	m_namespaces.push_back(make_pair(prefix, uri));
}

void document::EndNamespaceDeclHandler(const std::string& prefix)
{
}

void document::DoctypeDeclHandler(const std::string& root, const std::string& publicId, const std::string& uri)
{
	m_doctype.m_root = root;
	m_doctype.m_pubid = publicId;
	m_doctype.m_dtd = uri;
}

void document::NotationDeclHandler(const std::string& name, const std::string& sysid, const std::string& pubid)
{
	if (m_notations.empty())
		m_root_size_at_first_notation = nodes().size();

	notation n = {name, sysid, pubid};

	auto i = find_if(m_notations.begin(), m_notations.end(),
					 [name](auto& nt) { return nt.m_name >= name; });

	m_notations.insert(i, n);
}

std::istream* document::external_entity_ref(const std::string& base, const std::string& pubid, const std::string& sysid)
{
	std::istream* result = nullptr;
	
	if (m_external_entity_ref_loader)
		result = m_external_entity_ref_loader(base, pubid, sysid);
	
	if (result == nullptr and not sysid.empty())
	{
		std::string path;
		
		if (base.empty())
			path = sysid;
		else
			path = base + '/' + sysid;

		std::unique_ptr<std::ifstream> file(new std::ifstream(path, std::ios::binary));
		if (not file->is_open())
			file->open(m_dtd_dir + '/' + path, std::ios::binary);
		
		if (file->is_open())
			result = file.release();
	}
	
	return result;
}

void document::parse(std::istream& is)
{
	parser p(is);

	using std::placeholders::_1;
	using std::placeholders::_2;
	using std::placeholders::_3;

	p.xml_decl_handler = std::bind(&document::XmlDeclHandler, this, _1, _2, _3);
	p.doctype_decl_handler = std::bind(&document::DoctypeDeclHandler, this, _1, _2, _3);
	p.start_element_handler = std::bind(&document::StartElementHandler, this, _1, _2, _3);
	p.end_element_handler = std::bind(&document::EndElementHandler, this, _1, _2);
	p.character_data_handler = std::bind(&document::CharacterDataHandler, this, _1);
	if (m_preserve_cdata)
	{
		p.start_cdata_section_handler = std::bind(&document::StartCdataSectionHandler, this);
		p.end_cdata_section_handler = std::bind(&document::EndCdataSectionHandler, this);
	}
	p.start_namespace_decl_handler = std::bind(&document::StartNamespaceDeclHandler, this, _1, _2);
	p.processing_instruction_handler = std::bind(&document::ProcessingInstructionHandler, this, _1, _2);
	p.comment_handler = std::bind(&document::CommentHandler, this, _1);
	p.notation_decl_handler = std::bind(&document::NotationDeclHandler, this, _1, _2, _3);
	p.external_entity_ref_handler = std::bind(&document::external_entity_ref, this, _1, _2, _3);

	m_cur = this;

 	p.parse(m_validating);
	
	assert(m_cur == this);
}

namespace literals
{

document operator""_xml(const char* text, size_t length)
{
	zeep::xml::document doc;
	doc.set_preserve_cdata(true);

	zeep::char_streambuf buffer(text, length);
	std::istream is(&buffer);
	is >> doc;
	
	return doc;
}

}

} // namespace xml
} // namespace zeep
