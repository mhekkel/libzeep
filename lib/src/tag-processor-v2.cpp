//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <unordered_set>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/filesystem/operations.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/el/process.hpp>
#include <zeep/http/tag-processor.hpp>

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace zeep
{
namespace http
{

std::unordered_set<std::string> kFixedValueBooleanAttributes{
	"async", "autofocus", "autoplay", "checked", "controls", "declare",
	"default", "defer", "disabled", "formnovalidate", "hidden", "ismap",
	"loop", "multiple", "novalidate", "nowrap", "open", "pubdate", "readonly",
	"required", "reversed", "scoped", "seamless", "selected"
};

// --------------------------------------------------------------------

int attribute_precedence(const xml::attribute* attr)
{
		 if (attr->name() == "insert" or attr->name() == "replace")		return -10;
	else if (attr->name() == "each")									return -9;
	else if (attr->name() == "if" or attr->name() == "unless" or
			 attr->name() == "switch" or attr->name() == "case")		return -8;
	else if (attr->name() == "object" or attr->name() == "with")		return -7;
	else if (attr->name() == "attr" or attr->name() == "attrappend" or
			 attr->name() == "attrprepend")								return -6;

	else if (attr->name() == "text" or attr->name() == "utext")			return 1;
	else if (attr->name() == "fragment")								return 2;
	else if (attr->name() == "remove")									return 3;

	else																return 0;
};

// --------------------------------------------------------------------
//

tag_processor_v2::tag_processor_v2(const char* ns)
	: tag_processor(ns)
{
	using namespace std::placeholders;

	register_attr_handler("if", std::bind(&tag_processor_v2::process_attr_if, this, _1, _2, _3, _4, _5, false));
	register_attr_handler("unless", std::bind(&tag_processor_v2::process_attr_if, this, _1, _2, _3, _4, _5, true));
	register_attr_handler("switch", std::bind(&tag_processor_v2::process_attr_switch, this, _1, _2, _3, _4, _5));
	register_attr_handler("text", std::bind(&tag_processor_v2::process_attr_text, this, _1, _2, _3, _4, _5, true));
	register_attr_handler("utext", std::bind(&tag_processor_v2::process_attr_text, this, _1, _2, _3, _4, _5, false));
	register_attr_handler("each", std::bind(&tag_processor_v2::process_attr_each, this, _1, _2, _3, _4, _5));
	register_attr_handler("attr", std::bind(&tag_processor_v2::process_attr_attr, this, _1, _2, _3, _4, _5));
	register_attr_handler("with", std::bind(&tag_processor_v2::process_attr_with, this, _1, _2, _3, _4, _5));
	register_attr_handler("inline", std::bind(&tag_processor_v2::process_attr_inline, this, _1, _2, _3, _4, _5));

	register_attr_handler("insert", std::bind(&tag_processor_v2::process_attr_include, this, _1, _2, _3, _4, _5, TemplateIncludeAction::insert));
	register_attr_handler("include", std::bind(&tag_processor_v2::process_attr_include, this, _1, _2, _3, _4, _5, TemplateIncludeAction::include));
	register_attr_handler("replace", std::bind(&tag_processor_v2::process_attr_include, this, _1, _2, _3, _4, _5, TemplateIncludeAction::replace));
}

void tag_processor_v2::process_xml(xml::node *node, const el::scope& parentScope, fs::path dir, basic_webapp& webapp)
{
	xml::text *text = dynamic_cast<xml::text *>(node);
	if (text != nullptr)
	{
		xml::container *parent = text->parent();

		std::string s = text->str();
		auto next = text->next();

		size_t b = 0;

		while (b < s.length())
		{
			auto i = s.find('[', b);
			if (i == std::string::npos)
				break;
			
			char c2 = s[i + 1];
			if (c2 != '[' and c2 != '(')
			{
				b = i + 1;
				continue;
			}

			i += 2;

			auto j = s.find(c2 == '(' ? ")]" : "]]", i);
			if (j == std::string::npos)
				break;
			
			auto m = s.substr(i, j - i);

			if (not el::process_el(parentScope, m))
				m.clear();
			else if (c2 == '(' and m.find('<') != std::string::npos)		// 'unescaped' text, but since we're an xml library reverse this by parsing the result and putting the 
			{
				xml::document subDoc("<foo>" + m + "</foo>");
				auto foo = subDoc.find_first("//foo");

				for (auto n: foo->children<xml::node>())
					parent->insert(next, n->clone());
				
				text->str(s.substr(0, i - 2));

				s = s.substr(j + 2);
				b = 0;
				text = new xml::text(s);
				parent->insert(next, text);
			}
			else
			{
				s.replace(i - 2, j - i + 4, m);
				b = i + m.length();
			}
		}

		text->str(s);
		return;
	}

	xml::element *e = dynamic_cast<xml::element *>(node);
	if (e == nullptr)
		return;

	xml::container *parent = e->parent();
	el::scope scope(parentScope);
	bool inlined = false;

	try
	{
		std::vector<xml::attribute*> attributes;
		for (auto& attr: e->attributes())
			attributes.push_back(attr);
		
		sort(attributes.begin(), attributes.end(), [](auto a, auto b) { return attribute_precedence(a) < attribute_precedence(b); });

		for (auto attr: attributes)
		{
			if (attr->ns() != m_ns)
				continue;

			AttributeAction action = AttributeAction::none;
			
			if (attr->name() == "object")
				scope.select_object(el::evaluate_el(scope, attr->value()));
			else if (attr->name() == "inline")
			{
				action = process_attr_inline(e, attr, scope, dir, webapp);
				inlined = true;
			}
			else
			{
				auto h = m_attr_handlers.find(attr->name());

				if (h != m_attr_handlers.end())
					action = h->second(e, attr, scope, dir, webapp);
				else if (kFixedValueBooleanAttributes.count(attr->name()))
					action = process_attr_boolean_value(e, attr, scope, dir, webapp);
				else //if (kGenericAttributes.count(attr->name()))
					action = process_attr_generic(e, attr, scope, dir, webapp);
			}

			if (action == AttributeAction::remove)
			{
				parent->remove(node);
				node = nullptr;
				break;
			}

			e->remove_attribute(attr);
		}

		// el::scope nested(parentScope);

		// processor_map::iterator p = m_processor_table.find(e->name());
		// if (p != m_processor_table.end())
		// 	p->second(e, scope, dir);
		// else
		// 	throw exception((boost::format("unimplemented <mrs:%1%> tag") % e->name()).str());
	}
	catch (exception& ex)
	{
		xml::node *replacement = new xml::text(
			"Error processing directive '" + e->prefix() + ':' + e->name() + "': " + ex.what());

		parent->insert(e, replacement);
	}

	if (node != nullptr)
	{
		// make a copy of the list since process_xml might remove elements
		std::list<xml::node *> nodes;
		copy(e->node_begin(), e->node_end(), back_inserter(nodes));

		for (xml::node *n : nodes)
		{
			if (inlined and dynamic_cast<xml::text*>(n) != nullptr)
				continue;
			process_xml(n, scope, dir, webapp);
		}
	}
}

// -----------------------------------------------------------------------

auto tag_processor_v2::process_attr_if(xml::element* element, xml::attribute* attr, el::scope& scope, fs::path dir, basic_webapp& webapp, bool unless) ->AttributeAction
{
	return ((not el::evaluate_el(scope, attr->value()) == unless)) ? AttributeAction::none : AttributeAction::remove;
}

// -----------------------------------------------------------------------

auto tag_processor_v2::process_attr_text(xml::element* element, xml::attribute* attr, el::scope& scope, fs::path dir, basic_webapp& webapp, bool escaped) ->AttributeAction
{
	el::element obj = el::evaluate_el(scope, attr->value());

	if (escaped)
		element->set_text(obj.as<std::string>());
	else
	{
		auto children = element->children<xml::node>();
		for (auto child: children)
			element->remove(child);

		xml::document subDoc("<foo>" + obj.as<std::string>() + "</foo>");
		auto foo = subDoc.find_first("//foo");

		for (auto n: foo->children<xml::node>())
			element->append(n->clone());
	}

	return AttributeAction::none;
}

// --------------------------------------------------------------------

auto tag_processor_v2::process_attr_switch(xml::element* element, xml::attribute* attr, el::scope& scope, fs::path dir, basic_webapp& webapp) -> AttributeAction
{
	auto v = el::evaluate_el(scope, attr->value()).as<std::string>();

	auto prefix = element->prefix_for_namespace(ns());

	auto cases = element->find(".//*[@case]");
	xml::element* selected = nullptr;
	xml::element* wildcard = nullptr;
	for (auto c: cases)
	{
		auto ca = c->get_attribute(prefix + ":case");

		if (ca == "*")
			wildcard = c;
		else if (v == ca or (el::process_el(scope, ca) and v == ca))
		{
			selected = c;
			break;
		}
	}

	if (selected == nullptr)
		selected = wildcard;

	// be very careful here,, c might be nested in another c

	for (auto c: cases)
	{
		if (c != selected)
			c->parent()->remove(c);
	}

	for (auto c: cases)
	{
		if (c != selected)
			delete c;
	}

	if (selected)
		selected->remove_attribute(prefix + ":case");

	return AttributeAction::none;
}

// -----------------------------------------------------------------------

auto tag_processor_v2::process_attr_with(xml::element* element, xml::attribute* attr, el::scope& scope, fs::path dir, basic_webapp& webapp) -> AttributeAction
{
	std::regex kEachRx(R"(^\s*(\w+)\s*=\s*(.+)$)");

	std::smatch m;
	auto s = attr->str();

	if (not std::regex_match(s, m, kEachRx))
		throw std::runtime_error("Invalid attribute value for :with");

	std::string var = m[1];
	std::string val = m[2];

	scope.put(var, el::evaluate_el(scope, val));

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_each(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	std::regex kEachRx(R"(^\s*(\w+)(?:\s*,\s*(\w+))?\s*:\s*(.+)$)");

	std::smatch m;
	auto s = attr->str();

	if (not std::regex_match(s, m, kEachRx))
		throw std::runtime_error("Invalid attribute value for :each");

	std::string var = m[1];
	std::string stat = m[2];
	
	el::object collection = el::evaluate_el(scope, m[3]);

	if (collection.is_array())
	{
		xml::container *parent = node->parent();
		assert(parent);

		size_t collectionSize = collection.size();
		size_t i = 0;

		for (auto v: collection)
		{
			el::scope s(scope);
			s.put(var, v);

			if (not stat.empty())
				s.put(stat, el::object{
					{ "index", i},
					{ "count", i + 1},
					{ "size", collectionSize },
					{ "current", v},
					{ "even", i % 2 == 1 },
					{ "odd", i % 2 == 0 },
					{ "first", i == 0 },
					{ "last", i + 1 == collectionSize }
				});

			xml::element* clone = static_cast<xml::element*>(node->clone());

			clone->remove_attribute(attr->qname());

			parent->insert(node, clone); // insert before processing, to assign namespaces
			process_xml(clone, s, dir, webapp);

			++i;
		}
	}

	return AttributeAction::remove;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_attr(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	auto v = el::evaluate_el_attr(scope, attr->str());
	for (auto vi: v)
		node->set_attribute(vi.first, vi.second);

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_generic(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	auto s = attr->str();

	el::process_el(scope, s);
	node->set_attribute(attr->name(), s);

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_boolean_value(
	xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	auto s = attr->str();

	if (el::evaluate_el(scope, s))
		node->set_attribute(attr->name(), attr->name());
	else
		node->remove_attribute(attr->name());

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_inline(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	auto type = attr->value();

	if (type == "javascript" or type == "css")
	{
		std::regex r = std::regex(R"(/\*\[\[(.+?)\]\]\*/\s*('([^'\\]|\\.)*'|"([^"\\]|\\.)*"|[^;\n])*|\[\[(.+?)\]\])");

		for (auto n: node->children<xml::node>())
		{
			xml::text* text = dynamic_cast<xml::text*>(n);
			if (text == nullptr)
				continue;

			std::string s = text->str();
			std::string t;		

			auto b = std::sregex_iterator(s.begin(), s.end(), r);
			auto e = std::sregex_iterator();

			auto i = s.begin();

			for (auto ri = b; ri != e; ++ri)
			{
				auto m = *ri;

				t.append(i, s.begin() + m.position());
				i = s.begin() + m.position() + m.length();

				auto v = m[1].matched ? m.str(1) : m.str(5);
				
				el::object obj = el::evaluate_el(scope, v);
				std::stringstream ss;
				ss << obj;
				v = ss.str();
				
				t.append(v.begin(), v.end());
			}

			t.append(i, s.end());

			text->str(t);
		}
	}
	else if (type != "none")
	{
		for (auto text: node->children<xml::text>())
		{
			std::string s = text->str();
			auto next = text->next();

			size_t b = 0;

			while (b < s.length())
			{
				auto i = s.find('[', b);
				if (i == std::string::npos)
					break;
				
				char c2 = s[i + 1];
				if (c2 != '[' and c2 != '(')
				{
					b = i + 1;
					continue;
				}

				i += 2;

				auto j = s.find(c2 == '(' ? ")]" : "]]", i);
				if (j == std::string::npos)
					break;
				
				auto m = s.substr(i, j - i);

				if (not el::process_el(scope, m))
					m.clear();
				else if (c2 == '(' and m.find('<') != std::string::npos)		// 'unescaped' text, but since we're an xml library reverse this by parsing the result and putting the 
				{
					xml::document subDoc("<foo>" + m + "</foo>");
					auto foo = subDoc.find_first("//foo");

					for (auto n: foo->children<xml::node>())
						node->insert(next, n->clone());
					
					text->str(s.substr(0, i - 2));

					s = s.substr(j + 2);
					b = 0;
					text = new xml::text(s);
					node->insert(next, text);
				}
				else
				{
					s.replace(i - 2, j - i + 4, m);
					b = i + m.length();
				}
			}

			text->str(s);
		}
	}

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_include(xml::element* node, xml::attribute* attr, el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp, TemplateIncludeAction tia)
{
	auto s = attr->str();

	const std::regex kTemplateRx(R"(^\s*(\S*)\s*::\s*(#?[-_[:alnum:]]+)$)");

	std::smatch m;

	if (not std::regex_match(s, m, kTemplateRx))
		throw std::runtime_error("Invalid attribute value for :include/insert/replace");

	std::string templateFile = m[1];
	std::string templateID = m[2];

	xml::element* replacement = nullptr;
	xml::document doc;
	xml::root_node* root = nullptr;

	if (templateFile.empty() or templateFile == "this")
		root = node->root();
	else
	{
		doc.set_preserve_cdata(true);

		bool loaded = false;

		for (const char* ext: { "", ".xhtml", ".html", ".xml" })
		{
			fs::path file = dir / (templateFile + ext);
			if (not fs::exists(webapp.get_docroot() / file))
				continue;

			webapp.load_template(file.string(), doc);
			loaded = true;
			break;
		}

		if (not loaded)
			throw std::runtime_error("Could not locate template file " + templateFile);

		root = doc.root();
	}

	if (templateID[0] == '#')	// by ID
		replacement = root->find_first("//*[@id='" + templateID.substr(1) + "']");
	else
		replacement = root->find_first("//*[@fragment='" + templateID + "']");

	if (not replacement)
		throw std::runtime_error("Could not find template with id " + templateID);
	
	if (root == doc.root())
		replacement->parent()->remove(replacement);
	else
		replacement = static_cast<xml::element*>(replacement->clone());

	AttributeAction result = AttributeAction::none;
	
	if (tia == TemplateIncludeAction::include)
	{
		for (auto child: replacement->children<xml::node>())
		{
			replacement->remove(child);
			node->push_back(child);

			process_xml(child, scope, dir, webapp);
		}
		delete replacement;
	}
	else
	{
		if (tia == TemplateIncludeAction::insert)
			node->push_back(replacement);
		else
		{
			node->parent()->insert(node, replacement);
			result = AttributeAction::remove;
		}

		if (templateID[0] == '#')	// by ID
			replacement->remove_attribute("id");
		else
			replacement->remove_attribute(replacement->prefix_for_namespace(ns()) + ":fragment");

		process_xml(replacement, scope, dir, webapp);
	}

	return result;
}

}
}
