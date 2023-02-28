// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <set>
#include <stack>

#include <zeep/xml/document.hpp>
#include <zeep/xml/xpath.hpp>

namespace zeep::xml
{

const std::set<std::string> kEmptyHTMLElements{
	"area", "base", "br", "col", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr"
};

// --------------------------------------------------------------------

void write_string(std::ostream& os, const std::string& s, bool escape_whitespace, bool escape_quot, bool trim, float version)
{
	bool last_is_space = false;

	auto sp = s.begin();
	auto se = s.end();

	while (sp < se)
	{
		auto sb = sp;

		unicode c;
		std::tie(c, sp) = get_first_char(sp, se);

		switch (c)
		{
			case '&':	os << "&amp;";			last_is_space = false; break;
			case '<':	os << "&lt;";			last_is_space = false; break;
			case '>':	os << "&gt;";			last_is_space = false; break;
			case '\"':	if (escape_quot)		os << "&quot;"; else os << static_cast<char>(c); last_is_space = false; break;
			case '\n':	if (escape_whitespace)	os << "&#10;"; else os << static_cast<char>(c); last_is_space = true; break;
			case '\r':	if (escape_whitespace)	os << "&#13;"; else os << static_cast<char>(c); last_is_space = false; break;
			case '\t':	if (escape_whitespace)	os << "&#9;"; else os << static_cast<char>(c); last_is_space = false; break;
			case ' ':	if (not trim or not last_is_space) os << ' '; last_is_space = true; break;
			case 0:		throw exception("Invalid null character in XML content");
			default:	if (c >= 0x0A0 or (version == 1.0 ? is_valid_xml_1_0_char(c) : is_valid_xml_1_1_char(c)))
							for (auto ci = sb; ci < sp; ++ci)
								os << *ci;
						else
							os << "&#" << static_cast<int>(c) << ';';
						last_is_space = false;
						break;
		}

		sb = sp;
	}
}

// --------------------------------------------------------------------

node::~node()
{
	// avoid deep recursion and stack overflows
	while (m_next != nullptr)
	{
		node* n = m_next;
		m_next = n->m_next;
		n->m_next = nullptr;
		delete n;
	}
}

element* node::root()
{
	element* result = nullptr;
	if (m_parent != nullptr)
		result = m_parent->root();
	return result;
}

const element* node::root() const
{
	const element* result = nullptr;
	if (m_parent != nullptr)
		result = m_parent->root();
	return result;
}

bool node::equals(const node* n) const
{
	assert(false);
	return n == this;
}

std::string node::lang() const
{
	std::string result;
	if (m_parent != nullptr)
		result = m_parent->lang();
	return result;
}

void node::insert_sibling(node* n, node* before)
{
	node* p = this;
	while (p->m_next != nullptr and p->m_next != before)
		p = p->m_next;

	if (p->m_next != before and before != nullptr)
		throw zeep::exception("before argument in insert_sibling is not valid");
	
	p->m_next = n;
	n->m_prev = p;
	n->m_parent = m_parent;
	n->m_next = before;
	
	if (before != nullptr)
		before->m_prev = n;

// #if DEBUG
// validate();
// n->validate();
// if (before) before->validate();
// #endif
}

void node::remove_sibling(node* n)
{
	assert (this != n);
	if (this == n)
		throw exception("inconsistent node tree");

	node* p = this;
	while (p != nullptr and p->m_next != n)
		p = p->m_next;

	if (p != nullptr and p->m_next == n)
	{
		p->m_next = n->m_next;
		if (p->m_next != nullptr)
			p->m_next->m_prev = p;
		n->m_next = n->m_prev = n->m_parent = nullptr;
	}
	else
		throw exception("remove for a node not found in the list");
}

void node::parent(element* n)
{
	assert(m_parent == nullptr);
	m_parent = n;
}

std::string node::get_qname() const
{
	return "";
}

std::string node::name() const
{
	std::string qn = get_qname();
	std::string::size_type s = qn.find(':');
	if (s != std::string::npos)
		qn.erase(0, s + 1);
	return qn;
}

std::string node::get_prefix() const
{
	std::string qn = get_qname();
	std::string::size_type s = qn.find(':');

	std::string p;

	if (s != std::string::npos)
		p = qn.substr(0, s);

	return p;
}

std::string node::get_ns() const
{
	std::string p = get_prefix();
	return namespace_for_prefix(p);
}

std::string node::namespace_for_prefix(const std::string& prefix) const
{
	std::string result;
	if (m_parent != nullptr)
		result = m_parent->namespace_for_prefix(prefix);
	return result;
}

std::pair<std::string,bool> node::prefix_for_namespace(const std::string& uri) const
{
	std::pair<std::string,bool> result{};
	if (m_parent != nullptr)
		result = m_parent->prefix_for_namespace(uri);
	return result;
}

std::string node::prefix_tag(const std::string& tag, const std::string& uri) const
{
	auto prefix = prefix_for_namespace(uri);
	return prefix.second ? prefix.first + ':' + tag : tag;
}

void node::validate()
{
	if (m_parent and dynamic_cast<element*>(this) != nullptr and
			(std::find_if(m_parent->m_nodes.begin(), m_parent->m_nodes.end(), [this](auto& i) { return &i == this; }) == m_parent->m_nodes.end()))
		throw exception("validation error: parent does not know node");
	if (m_next and m_next->m_prev != this)
		throw exception("validation error: m_next->m_prev != this");
	if (m_prev and m_prev->m_next != this)
		throw exception("validation error: m_prev->m_next != this");
	
	node* n = this;
	while (n != nullptr and n->m_next != this)
		n = n->m_next;
	if (n == this)
		throw exception("cycle in node list");

	n = this;
	while (n != nullptr and n->m_prev != this)
		n = n->m_prev;
	if (n == this)
		throw exception("cycle in node list");
	
	if (m_next)
		m_next->validate();
}

// --------------------------------------------------------------------
// comment

bool comment::equals(const node* n) const
{
	return
		dynamic_cast<const comment*>(n) != nullptr and
		m_text == static_cast<const comment*>(n)->m_text;
}

node* comment::clone() const
{
	return new comment(m_text);
}

node* comment::move()
{
	return new comment(std::move(*this));
}

void comment::write(std::ostream& os, format_info fmt) const
{
	// if (fmt.indent_width != 0)
	// 	os << std::endl << std::string(fmt.indent_width, ' ');

	if (not fmt.suppress_comments)
	{
		os << "<!--";
		
		bool lastWasHyphen = false;
		for (char ch: m_text)
		{
			if (ch == '-' and lastWasHyphen)
				os << ' ';
			
			os << ch;
			lastWasHyphen = ch == '-';

			// if (ch == '\n')
			// {
			// 	for (size_t i = 0; i < indent; ++i)
			// 		os << ' ';
			// }
		}
		
		os << "-->";
		
		if (fmt.indent_width != 0)
			os << std::endl;
	}
}

// --------------------------------------------------------------------
// processing_instruction

bool processing_instruction::equals(const node* n) const
{
	return
		dynamic_cast<const processing_instruction*>(n) != nullptr and
		m_text == static_cast<const processing_instruction*>(n)->m_text;
}

node* processing_instruction::clone() const
{
	return new processing_instruction(m_target, m_target);
}

node* processing_instruction::move()
{
	return new processing_instruction(std::move(*this));
}

void processing_instruction::write(std::ostream& os, format_info fmt) const
{
	if (fmt.indent)
		os << std::endl << std::string(fmt.indent_level * fmt.indent_width, ' ');

	os << "<?" << m_target << ' ' << m_text << "?>";
	
	if (fmt.indent != 0)
		os << std::endl;
}

// --------------------------------------------------------------------
// text

bool text::equals(const node* n) const
{
	bool result = false;
	auto t = dynamic_cast<const text*>(n);

	if (t != nullptr)
	{
		std::string text = m_text;
		trim(text);

		std::string ttext = t->m_text;
		trim(ttext);

		result = text == ttext;
	}

	return result;
}

bool text::is_space() const
{
	bool result = true;
	for (auto ch: m_text)
	{
		if (not (ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r'))
		{
			result = false;
			break;
		}
	}
	return result;
}

node* text::clone() const
{
	return new text(m_text);
}

node* text::move()
{
	return new text(std::move(*this));
}

void text::write(std::ostream& os, format_info fmt) const
{
	write_string(os, m_text, fmt.escape_white_space, fmt.escape_double_quote, false, fmt.version);
}

// --------------------------------------------------------------------
// cdata

bool cdata::equals(const node* n) const
{
	return
		dynamic_cast<const cdata*>(n) != nullptr and
		m_text == static_cast<const cdata*>(n)->m_text;
}

node* cdata::clone() const
{
	return new cdata(m_text);
}

node* cdata::move()
{
	return new cdata(std::move(*this));
}

void cdata::write(std::ostream& os, format_info fmt) const
{
	if (fmt.indent)
		os << std::endl << std::string(fmt.indent_level * fmt.indent_width, ' ');

	os << "<![CDATA[" << m_text << "]]>";
	
	if (fmt.indent)
		os << std::endl;
}

// --------------------------------------------------------------------
// attribute

bool attribute::equals(const node* n) const
{
	bool result = false;
	const attribute* a = dynamic_cast<const attribute*>(n);

	if (a != nullptr)
	{
		result = m_qname == a->m_qname and
				 m_value == a->m_value;
	}
	return result;
}

std::string attribute::uri() const
{
	assert(is_namespace());
	if (not is_namespace())
		throw exception("Attribute is not a namespace");
	return m_value;
}

node* attribute::clone() const
{
	return new attribute(m_qname, m_value, m_id);
}

node* attribute::move()
{
	return new attribute(std::move(*this));
}

void attribute::write(std::ostream& os, format_info fmt) const
{
	if (fmt.indent_width != 0)
		os << std::endl << std::string(fmt.indent_width, ' ');
	else
		os << ' ';

	os << m_qname << "=\"";
	
	write_string(os, m_value, fmt.escape_white_space, true, false, fmt.version);
	
	os << '"';
}

// // --------------------------------------------------------------------
// // name_space

// bool name_space::equals(const node* n) const
// {
// 	bool result = false;
// 	const name_space* ns = dynamic_cast<const name_space*>(n);

// 	if (ns != nullptr)
// 		result = m_uri == ns->m_uri;

// 	return result;
// }

// node* name_space::clone() const
// {
// 	return new name_space(m_prefix, m_uri);
// }

// node* name_space::move()
// {
// 	return new name_space(std::move(*this));
// }

// void name_space::write(std::ostream& os, format_info fmt) const
// {
// 	if (fmt.indent_width != 0)
// 		os << std::endl << std::string(fmt.indent_width, ' ');
// 	else
// 		os << ' ';

// 	if (m_prefix.empty())
// 		os << "xmlns";
// 	else
// 		os << "xmlns:" << m_prefix;
// 	os << "=\"";
	
// 	write_string(os, m_uri, fmt.escape_white_space, false, fmt.version);
	
// 	os << '"';
// }

// --------------------------------------------------------------------
// element

element::element()
	: m_nodes(*this)
	, m_attributes(*this)
{
}

element::element(const std::string& qname)
	: m_qname(qname)
	, m_nodes(*this)
	, m_attributes(*this)
{
}

element::element(const std::string& qname, std::initializer_list<zeep::xml::attribute> attributes)
	: m_qname(qname)
	, m_nodes(*this)
	, m_attributes(*this)
{
	for (auto& a: attributes)
		set_attribute(a.get_qname(), a.value());
}

// copy constructor. Copy data and children, but not parent and sibling
element::element(const element& e)
	: node()
	, m_qname(e.m_qname)
	, m_nodes(*this, e.m_nodes)
	, m_attributes(*this, e.m_attributes)
{
}

element::element(element&& e)
	: element()
{
	swap(e);
}

element& element::operator=(const element& e)
{
	if (this != &e)
	{
		m_qname = e.m_qname;
		m_nodes = e.m_nodes;
		m_attributes = e.m_attributes;
	}

	return *this;
}

element& element::operator=(element&& e)
{
	if (this != &e)
	{
		clear();
		swap(e);
	}

	return *this;
}

element::~element()
{
}

void element::swap(element& e) noexcept
{
	std::swap(m_qname, e.m_qname);
	m_nodes.swap(e.m_nodes);
	m_attributes.swap(e.m_attributes);
}

node* element::clone() const
{
	return new element(*this);
}

node* element::move()
{
	return new element(std::move(*this));
}

std::string element::lang() const
{
	std::string result;

	auto i = m_attributes.find("xml:lang");
	if (i != m_attributes.end())
		result = i->value();
	else if (m_parent != nullptr)
		result = m_parent->lang();

	return result;
}

std::string element::id() const
{
	std::string result;
	
	for (auto& a: m_attributes)
	{
		if (a.is_id())
		{
			result = a.value();
			break;
		}
	}
	
	return result;
}

std::string element::get_attribute(const std::string& qname) const
{
	std::string result;

	auto a = m_attributes.find(qname);
	if (a != m_attributes.end())
		result = a->value();

	return result;
}

void element::set_attribute(const std::string& qname, const std::string& value)
{
	m_attributes.emplace(qname, value);
}

bool element::equals(const node* n) const
{
	bool result = false;
	const element* e = dynamic_cast<const element*>(n);

	if (e != nullptr)
	{
		result = name() == e->name() and get_ns() == e->get_ns();

		const node* a = m_nodes.m_head;;
		const node* b = e->m_nodes.m_head;
		
		while (a != nullptr or b != nullptr)
		{
			if (a != nullptr and b != nullptr and a->equals(b))
			{
				a = a->m_next;
				b = b->m_next;
				continue;
			}

			const text* t;
			
			t = dynamic_cast<const text*>(a);
			if (t != nullptr and t->is_space())
			{
				a = a->m_next;
				continue;
			}

			t = dynamic_cast<const text*>(b);
			if (t != nullptr and t->is_space())
			{
				b = b->m_next;
				continue;
			}

			result = false;
			break;
		}

		result = result and ((a == nullptr) == (b == nullptr));

		if (result)
		{
			result = m_attributes == e->m_attributes;
			if (not result)
			{
				std::set<attribute> as(m_attributes.begin(), m_attributes.end());
				std::set<attribute> bs(e->m_attributes.begin(), e->m_attributes.end());

				std::set<std::string> nsa, nsb;

				auto ai = as.begin(), bi = bs.begin();
				for (;;)
				{
					if (ai == as.end() and bi == bs.end())
						break;
					
					if (ai != as.end() and ai->is_namespace())
					{
						nsa.insert(ai->value());
						++ai;
						continue;
					}

					if (bi != bs.end() and bi->is_namespace())
					{
						nsb.insert(bi->value());
						++bi;
						continue;
					}

					if (ai == as.end() or bi == bs.end() or *ai++ != *bi++)
					{
						result = false;
						break;
					}
				}

				result = ai == as.end() and bi == bs.end() and nsa == nsb;
			}
		}
	}

	return result;
}

void element::clear()
{
	m_nodes.clear();
	m_attributes.clear();
}

std::string element::get_content() const
{
	std::string result;

	for (auto& n: m_nodes)
	{
		auto t = dynamic_cast<const text*>(&n);
		if (t != nullptr)
			result += t->get_text();
	}

	return result;
}

void element::set_content(const std::string& s)
{
	// remove all existing text nodes (including cdata ones)
	for (auto n = m_nodes.begin(); n != m_nodes.end(); ++n)
	{
		if (dynamic_cast<text*>(&*n) != nullptr)
			n = m_nodes.erase(n);
	}

	// and add a new text node with the content
	m_nodes.insert(m_nodes.end(), text(s));
}

std::string element::str() const
{
	std::string result;
	
	for (auto& n: m_nodes)
		result += n.str();
	
	return result;
}

void element::add_text(const std::string& s)
{
	text* textNode = dynamic_cast<text*>(m_nodes.m_tail);
	
	if (textNode != nullptr and dynamic_cast<cdata*>(textNode) == nullptr)
		textNode->append(s);
	else
		m_nodes.emplace_back(text(s));
}

void element::set_text(const std::string& s)
{
	set_content(s);
}

void element::flatten_text()
{
	auto n = m_nodes.m_head;
	while (n != m_nodes.m_tail)
	{
		auto tn = dynamic_cast<text*>(n);
		if (tn == nullptr)
		{
			n = n->m_next;
			continue;
		}

		if (n->m_next == nullptr)	// should never happen
			break;
		
		auto ntn = dynamic_cast<text*>(n->m_next);
		if (ntn == nullptr)
		{
			n = n->m_next;
			continue;
		}

		tn->append(ntn->get_text());
		m_nodes.erase(n->m_next);
	}
}

void element::write(std::ostream& os, format_info fmt) const
{
	// if width is set, we wrap and indent the file
	size_t indentation = fmt.indent_level * fmt.indent_width;

	if (fmt.indent)
	{
		if (fmt.indent_level > 0)
			os << std::endl;
		os << std::string(indentation, ' ');
	}

	os << '<' << m_qname;

	// if the left flag is set, wrap and indent attributes as well
	auto attr_fmt = fmt;
	attr_fmt.indent_width = 0;

	for (auto& attr: attributes())
	{
		attr.write(os, attr_fmt);
		if (attr_fmt.indent_width == 0 and fmt.indent_attributes)
			attr_fmt.indent_width = indentation + 1 + m_qname.length() + 1;
	}
	
	if ((fmt.html and kEmptyHTMLElements.count(m_qname)) or
		(not fmt.html and fmt.collapse_tags and nodes().empty()))
		os << "/>";
	else
	{
		os << '>';
		auto sub_fmt = fmt;
		++sub_fmt.indent_level;

		bool wrote_element = false;
		for (auto& n: nodes())
		{
			n.write(os, sub_fmt);
			wrote_element = dynamic_cast<const element*>(&n) != nullptr;
		}

		if (wrote_element and fmt.indent != 0)
			os << std::endl << std::string(indentation, ' ');

		os << "</" << m_qname << '>';
	}
}

std::ostream& operator<<(std::ostream& os, const element& e)
{
	auto flags = os.flags({});
	auto width = os.width(0);

	format_info fmt;
	fmt.indent = width > 0;
	fmt.indent_width = width;
	fmt.indent_attributes = flags & std::ios_base::left;

	e.write(os, fmt);

	return os;
}

std::string element::namespace_for_prefix(const std::string& prefix) const
{
	std::string result;
	
	for (auto& a: m_attributes)
	{
		if (not a.is_namespace())
			continue;
		
		if (a.name() == "xmlns")
		{
			if (prefix.empty())
			{
				result = a.value();
				break;
			}
			continue;
		}

		if (a.name() == prefix)
		{
			result = a.value();
			break;
		}
	}
	
	if (result.empty() and dynamic_cast<element*>(m_parent) != nullptr)
		result = static_cast<element*>(m_parent)->namespace_for_prefix(prefix);
	
	return result;
}

std::pair<std::string,bool> element::prefix_for_namespace(const std::string& uri) const
{
	std::string result;
	bool found = false;

	for (auto& a: m_attributes)
	{
		if (not a.is_namespace())
			continue;
		
		if (a.value() == uri)
		{
			found = true;
			if (a.get_qname().length() > 6)
				result = a.get_qname().substr(6);
			break;
		}
	}
	
	if (not found and dynamic_cast<element*>(m_parent) != nullptr)
		std::tie(result, found) = static_cast<element*>(m_parent)->prefix_for_namespace(uri);
	
	return make_pair(result, found);
}

void element::move_to_name_space(const std::string& prefix, const std::string& uri,
	bool recursive, bool including_attributes)
{
	// first some sanity checks
	auto p = prefix_for_namespace(uri);
	if (p.second)
	{
		if (p.first != prefix)
			throw exception("Invalid prefix in move_to_name_space, already known as '" + p.first + "'");
	}
	else
	{
		bool set = false;
		for (auto& a: m_attributes)
		{
			if (not a.is_namespace())
				continue;
			
			if (a.get_qname().length() > 6 and a.get_qname().substr(6) == prefix)
			{
				set = true;
				a.value(uri);
				break;
			}
		}

		if (not set)
			m_attributes.emplace(prefix.empty() ? "xmlns" : "xmlns:" + prefix, uri);
	}	

	set_qname(prefix, name());

	if (including_attributes)
	{
		// first process the namespace attributes...
		for (auto& attr: m_attributes)
		{
			if (not attr.is_namespace())
				continue;
			
			auto nsp = prefix_for_namespace(attr.uri());
			if (not nsp.second)
				attr.set_qname("xmlns", nsp.first);
		}

		// ... and then the others, makes sure the namespaces are known
		for (auto& attr: m_attributes)
		{
			if (attr.is_namespace())
				continue;

			auto ns = attr.get_ns();

			if (ns.empty())
				attr.set_qname(prefix + ':' + attr.name());
			else
			{
				auto nsp = prefix_for_namespace(ns);
				if (not nsp.second)
					throw exception("Cannot move element to new namespace, namespace not found: " + ns);
				attr.set_qname(nsp.first, attr.name());
			}
		}
	}

	if (recursive)
	{
		for (element& e: *this)
			e.move_to_name_space(prefix, uri, true, including_attributes);
	}
}

element_set element::find(const char *path) const
{
	return xpath(path).evaluate<element>(*this);
}

element::iterator element::find_first(const char *path)
{
	element_set s = xpath(path).evaluate<element>(*this);

	return s.empty() ? end() : iterator(*this, s.front());
}

void element::validate()
{
	node::validate();

	for (auto& n: nodes())
		n.validate();

	for (auto& a: attributes())
	{
		if (a.parent() != this)
			throw exception("validation error: attribute has incorrect parent");
	}
}

// --------------------------------------------------------------------

void fix_namespaces(element& e, element& source, element& dest)
{
	std::stack<xml::node*> s;

	s.push(&e);

	std::map<std::string,std::string> mapped;

	while (not s.empty())
	{
		auto n = s.top();
		s.pop();

		auto p = n->get_prefix();
		if (not p.empty())
		{
			if (mapped.count(p))
			{
				if (mapped[p] != p)
					n->set_qname(mapped[p], n->name());
			}
			else
			{
				auto ns = n->namespace_for_prefix(p);
				if (ns.empty())
					ns = source.namespace_for_prefix(p);

				auto dp = dest.prefix_for_namespace(ns);
				if (dp.second)
				{
					mapped[p] = dp.first;
					n->set_qname(dp.first, n->name());
				}
				else
				{
					mapped[p] = p;
					dest.attributes().emplace({ "xmlns:" + p, ns });
				}
			}
		}

		auto el = dynamic_cast<element*>(n);
		if (el == nullptr)
			continue;

		for (auto& c: *el)
			s.push(&c);
		
		for (auto& a: el->attributes())
			s.push(&a);
	}
}

}