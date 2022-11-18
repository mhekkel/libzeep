//        Copyright Maarten L. Hekkelman, 2019-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <unordered_set>

#include <zeep/xml/xpath.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/template-processor.hpp>
#include <zeep/http/tag-processor.hpp>

namespace fs = std::filesystem;

namespace zeep::http
{

std::unordered_set<std::string> kFixedValueBooleanAttributes{
	"async", "autofocus", "autoplay", "checked", "controls", "declare",
	"default", "defer", "disabled", "formnovalidate", "hidden", "ismap",
	"loop", "multiple", "novalidate", "nowrap", "open", "pubdate", "readonly",
	"required", "reversed", "scoped", "seamless", "selected"
};

// --------------------------------------------------------------------

int attribute_precedence(const xml::attribute& attr)
{
		 if (attr.name() == "insert" or attr.name() == "replace")	return -10;
	else if (attr.name() == "each")									return -9;
	else if (attr.name() == "if" or attr.name() == "unless" or
			 attr.name() == "switch" or attr.name() == "case")		return -8;
	else if (attr.name() == "object" or attr.name() == "with")		return -7;
	else if (attr.name() == "attr" or
			 attr.name() == "attrappend" or attr.name() == "attrprepend" or
			 attr.name() == "classappend" or attr.name() == "styleappend")
			 														return -6;

	else if (attr.name() == "text" or attr.name() == "utext")		return 1;
	else if (attr.name() == "fragment")								return 2;
	else if (attr.name() == "remove")								return 3;

	else															return 0;
};

// --------------------------------------------------------------------
//

tag_processor_v2::tag_processor_v2(const char* ns)
	: tag_processor(ns)
{
	using namespace std::placeholders;

	register_attr_handler("assert",			std::bind(&tag_processor_v2::process_attr_assert,		this, _1, _2, _3, _4, _5));
	register_attr_handler("attr",			std::bind(&tag_processor_v2::process_attr_attr,			this, _1, _2, _3, _4, _5));
	register_attr_handler("classappend",	std::bind(&tag_processor_v2::process_attr_classappend,	this, _1, _2, _3, _4, _5));
	register_attr_handler("each",			std::bind(&tag_processor_v2::process_attr_each,			this, _1, _2, _3, _4, _5));
	register_attr_handler("if",				std::bind(&tag_processor_v2::process_attr_if,			this, _1, _2, _3, _4, _5, false));
	register_attr_handler("include",		std::bind(&tag_processor_v2::process_attr_include,		this, _1, _2, _3, _4, _5, TemplateIncludeAction::include));
	register_attr_handler("inline",			std::bind(&tag_processor_v2::process_attr_inline,		this, _1, _2, _3, _4, _5));
	register_attr_handler("insert",			std::bind(&tag_processor_v2::process_attr_include,		this, _1, _2, _3, _4, _5, TemplateIncludeAction::insert));
	register_attr_handler("replace",		std::bind(&tag_processor_v2::process_attr_include,		this, _1, _2, _3, _4, _5, TemplateIncludeAction::replace));
	register_attr_handler("styleappend",	std::bind(&tag_processor_v2::process_attr_styleappend,	this, _1, _2, _3, _4, _5));
	register_attr_handler("switch",			std::bind(&tag_processor_v2::process_attr_switch,		this, _1, _2, _3, _4, _5));
	register_attr_handler("text",			std::bind(&tag_processor_v2::process_attr_text,			this, _1, _2, _3, _4, _5, true));
	register_attr_handler("unless",			std::bind(&tag_processor_v2::process_attr_if,			this, _1, _2, _3, _4, _5, true));
	register_attr_handler("utext",			std::bind(&tag_processor_v2::process_attr_text,			this, _1, _2, _3, _4, _5, false));
	register_attr_handler("with",			std::bind(&tag_processor_v2::process_attr_with,			this, _1, _2, _3, _4, _5));
	// register_attr_handler("remove",  std::bind(&tag_processor_v2::process_attr_remove,	this, _1, _2, _3, _4, _5));
}

void tag_processor_v2::process_xml(xml::node* node, const scope& parentScope, fs::path dir, basic_template_processor& loader)
{
	m_template.clear();
	m_template.emplace_back(*static_cast<const xml::element*>(node));

	process_node(node, parentScope, dir, loader);

	auto e = dynamic_cast<xml::element*>(node);
	if (e != nullptr)
		post_process(e, parentScope, dir, loader);
}

// --------------------------------------------------------------------
// post processing: remove blocks, remove attributes with ns = ns(), process remove 

void tag_processor_v2::post_process(xml::element* e, const scope& parentScope, fs::path dir, basic_template_processor& loader)
{
	auto parent = e->parent();

	for (auto ai = e->attributes().begin(); ai != e->attributes().end();)
	{
		if (ai->get_ns() == m_ns and ai->name() == "remove" and parent != nullptr)
		{
			scope sub(parentScope);
			auto action = process_attr_remove(e, ai, sub, dir, loader);

			if (action == AttributeAction::remove)
			{
				parent->erase(e);
				return;
			}
		}

		if (ai->get_ns() == m_ns)
			ai = e->attributes().erase(ai);
		else
			++ai;
	}

	if (e->get_ns() == m_ns and e->name() == "block" and parent != nullptr)
	{
		for (auto& ci: e->nodes())
			parent->nodes().insert(e, std::move(ci));

		e = parent->erase(e);
		return;
	}

	// take a copy since iterators might get invalid
	std::vector<xml::element*> children;
	std::transform(e->begin(), e->end(), std::back_inserter(children), [](auto& c) { return &c; });

	for (auto& c: children)
		post_process(c, parentScope, dir, loader);

	// postpone removing namespaces until all children have been processed
	for (auto ai = e->attributes().begin(); ai != e->attributes().end();)
	{
		if (ai->is_namespace() and ai->value() == m_ns)
			ai = e->attributes().erase(ai);
		else
			++ai;
	}
}

// -----------------------------------------------------------------------

void tag_processor_v2::process_text(xml::text& text, const scope& scope)
{
	xml::element* parent = text.parent();

	auto next = std::find_if(parent->nodes().begin(), parent->nodes().end(), [&text](auto& n) { return &n == &text; });
	assert(next != parent->nodes().end());
	++next;

	std::string s = text.get_text();

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

		if (not process_el(scope, m))
			m = "Error processing " + m;
		
		if (c2 == '(' and m.find('<') != std::string::npos)		// 'unescaped' text, but since we're an xml library reverse this by parsing the result and putting the 
		{
			xml::document subDoc("<foo>" + m + "</foo>");
			auto foo = subDoc.front();

			for (auto& n: foo.nodes())
				parent->nodes().emplace(next, std::move(n));
			
			text.set_text(s.substr(0, i - 2));

			s = s.substr(j + 2);
			b = 0;
			parent->nodes().emplace(next, xml::text(s));
		}
		else
		{
			s.replace(i - 2, j - i + 4, m);
			b = i + m.length();
		}
	}

	text.set_text(s);
}

// --------------------------------------------------------------------

std::vector<std::unique_ptr<xml::node>> tag_processor_v2::resolve_fragment_spec(
	xml::element* node, fs::path dir, basic_template_processor& loader, const json::element& spec, const scope& scope)
{
	if (spec.contains("is-node-set") and spec["is-node-set"])
		return scope.get_nodeset(spec["node-set-name"].as<std::string>());

	if (spec.is_object() and spec["template"].is_string() and spec["selector"].is_object() and spec["selector"]["xpath"].is_string())
	{
		auto file = spec["template"].as<std::string>();
		auto selector = spec["selector"]["xpath"].as<std::string>();

		if (not (spec.is_null() or selector.empty()))
			return resolve_fragment_spec(node, dir, loader, file, selector, true);
	}
	else if (spec.is_string())
	{
		const std::regex kTemplateRx(R"(^\s*(\S*)\s*::\s*(#?[-_[:alnum:]]+)$)");

		std::smatch m;

		std::string s = spec.as<std::string>();
		if (not std::regex_match(s, m, kTemplateRx))
			throw std::runtime_error("Invalid attribute value for :include/insert/replace");

		std::string file = m[1];
		std::string id = m[2];
		bool byID = false;
		std::string selector;

		if (id[0] == '#')	// by ID
		{
			byID = true;
			selector = "//*[@id='" + id.substr(1) + "']";
		}
		else
			selector = "//*[name()='"+id+"' or attribute::*[namespace-uri() = $ns and (local-name() = 'ref' or local-name() = 'fragment') and starts-with(string(), '"+id+"')]]";

		return resolve_fragment_spec(node, dir, loader, file, selector, byID);
	}

	return {};
}

std::vector<std::unique_ptr<xml::node>> tag_processor_v2::resolve_fragment_spec(
	xml::element* node, fs::path dir, basic_template_processor& loader, const std::string& file, const std::string& selector, bool byID)
{
	xml::context ctx;
	ctx.set("ns", ns());

	xml::xpath xp(selector);
	// xp.dump();

	xml::document doc;
	xml::element* root = nullptr;

	if (file.empty() or file == "this")
		root = m_template.root();
	else
	{
		doc.set_preserve_cdata(true);

		bool loaded = false;

		for (const char* ext: { "", ".xhtml", ".html", ".xml" })
		{
			std::error_code ec;

			fs::path template_file = dir / (file + ext);

			(void)loader.file_time(template_file.string(), ec);
			if (ec)
				continue;

			loader.load_template(template_file.string(), doc);
			loaded = true;
			
			break;
		}

		if (not loaded)
			throw std::runtime_error("Could not locate template file " + file);

		root = doc.root();
	}

	std::vector<std::unique_ptr<xml::node>> result;

	for (auto n: xp.evaluate<xml::node>(*root, ctx))
	{
		auto copy = n->clone();

		auto e = dynamic_cast<xml::element*>(copy);

		if (e != nullptr)
		{
			if (root != node->root())
				fix_namespaces(*e, static_cast<xml::element&>(*n), *node);

			auto& attr = e->attributes();

			if (byID)	// remove the copied ID
				attr.erase("id");
			
			attr.erase(e->prefix_tag("ref", ns()));
			attr.erase(e->prefix_tag("fragment", ns()));
		}

		result.emplace_back(copy);
	}

	return result;
}

// -----------------------------------------------------------------------

void tag_processor_v2::process_node(xml::node* node, const scope& parentScope, std::filesystem::path dir, basic_template_processor& loader)
{
	for (;;)
	{
		xml::text* text = dynamic_cast<xml::text*>(node);
		if (text != nullptr)
		{
			process_text(*text, parentScope);
			break;
		}

		xml::element* e = dynamic_cast<xml::element *>(node);
		if (e == nullptr)
			break;

		xml::element* parent = e->parent();
		scope scope(parentScope);
		bool inlined = false;

		try
		{
			auto& attributes = e->attributes();

			attributes.sort([](auto a, auto b) { return attribute_precedence(a) < attribute_precedence(b); });

			auto attr = attributes.begin();
			while (attr != attributes.end())
			{
				if (attr->get_ns() != m_ns or attr->name() == "remove" or attr->name() == "ref" or attr->name() == "fragment")
				{
					++attr;
					continue;
				}

				AttributeAction action = AttributeAction::none;
				
				if (attr->name() == "object")
					scope.select_object(evaluate_el(scope, attr->value()));
				else if (attr->name() == "inline")
				{
					action = process_attr_inline(e, attr, scope, dir, loader);
					inlined = true;
				}
				else
				{
					auto h = m_attr_handlers.find(attr->name());

					if (h != m_attr_handlers.end())
						action = h->second(e, attr, scope, dir, loader);
					else if (kFixedValueBooleanAttributes.count(attr->name()))
						action = process_attr_boolean_value(e, attr, scope, dir, loader);
					else
						action = process_attr_generic(e, attr, scope, dir, loader);
				}

				if (action == AttributeAction::remove)
				{
					parent->erase(node);
					node = nullptr;
					break;
				}

				attr = e->attributes().erase(attr);
			}
		}
		catch (const std::exception& ex)
		{
			parent->nodes().insert(e, xml::text("Error processing element '" + e->get_qname() + "': " + ex.what()));
			// parent->erase(e);
		}

		if (node != nullptr)
		{
			auto i = e->nodes().begin();
			while (i != e->nodes().end())
			{
				auto& n = *i;
				++i;

				if (inlined and dynamic_cast<xml::text*>(&n) != nullptr)
					continue;

				process_node(&n, scope, dir, loader);
			}
		}

		break;
	}
}

// -----------------------------------------------------------------------

auto tag_processor_v2::process_attr_if(xml::element* element, xml::attribute* attr, scope& scope, fs::path dir, basic_template_processor& loader, bool unless) ->AttributeAction
{
	return ((not evaluate_el(scope, attr->value()) == unless)) ? AttributeAction::none : AttributeAction::remove;
}

// -----------------------------------------------------------------------

auto tag_processor_v2::process_attr_assert(xml::element* element, xml::attribute* attr, scope& scope, fs::path dir, basic_template_processor& loader) ->AttributeAction
{
	if (not evaluate_el_assert(scope, attr->value()))
		throw zeep::exception("Assertion failed for '" + attr->value() + "'");
	return AttributeAction::none;
}

// -----------------------------------------------------------------------

auto tag_processor_v2::process_attr_text(xml::element* element, xml::attribute* attr, scope& scope, fs::path dir, basic_template_processor& loader, bool escaped) ->AttributeAction
{
	json::element obj = evaluate_el(scope, attr->value());

	if (not obj.is_null())
	{
		std::string text;

		if (obj.is_object() and obj.contains("is-node-set") and obj["is-node-set"])
		{
			auto s = scope.get_nodeset(obj["node-set-name"].as<std::string>());

			for (auto& n: s)
				text += n->str();
		}
		else
			text = obj.as<std::string>();

		if (escaped)
			element->set_text(text);
		else
		{
			element->set_text("");

			xml::document subDoc("<foo>" + text + "</foo>");
			auto foo = subDoc.front();
			for (auto& n: foo.nodes())
				element->nodes().emplace(element->end(), std::move(n));
		}
	}

	return AttributeAction::none;
}

// --------------------------------------------------------------------

auto tag_processor_v2::process_attr_switch(xml::element* element, xml::attribute* attr, scope& scope, fs::path dir, basic_template_processor& loader) -> AttributeAction
{
	auto vo = evaluate_el(scope, attr->value());
	std::string v;
	if (not vo.is_null())
		v = vo.as<std::string>();

	xml::element e2(*element);
	element->nodes().clear();

	auto cases = e2.find(".//*[@case]");

	xml::element* selected = nullptr;
	xml::element* wildcard = nullptr;
	for (auto c: cases)
	{
		auto ca = c->get_attribute(element->prefix_tag("case", ns()));

		if (ca == "*")
			wildcard = c;
		else if (v == ca or (process_el(scope, ca) and v == ca))
		{
			selected = c;
			break;
		}
	}

	if (selected == nullptr)
		selected = wildcard;

	if (selected != nullptr)
	{
		selected->attributes().erase(element->prefix_tag("case", ns()));
		element->emplace_back(std::move(*selected));
	}

	return AttributeAction::none;
}

// -----------------------------------------------------------------------

auto tag_processor_v2::process_attr_with(xml::element* element, xml::attribute* attr, scope& scope, fs::path dir, basic_template_processor& loader) -> AttributeAction
{
	evaluate_el_with(scope, attr->value());
	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_each(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	std::regex kEachRx(R"(^\s*(\w+)(?:\s*,\s*(\w+))?\s*:\s*(.+)$)");

	std::smatch m;
	auto s = attr->value();

	if (not std::regex_match(s, m, kEachRx))
		throw std::runtime_error("Invalid attribute value for :each");

	std::string var = m[1];
	std::string stat = m[2];
	
	object collection = evaluate_el(scope, m[3]);

	if (collection.is_array())
	{
		xml::element *parent = node->parent();
		assert(parent);

		size_t collectionSize = collection.size();
		size_t ix = 0;

		for (auto v: collection)
		{
			auto subscope(scope);
			subscope.put(var, v);

			if (not stat.empty())
				subscope.put(stat, object{
					{ "index", ix },
					{ "count", ix + 1 },
					{ "size", collectionSize },
					{ "current", v},
					{ "even", ix % 2 == 1 },
					{ "odd", ix % 2 == 0 },
					{ "first", ix == 0 },
					{ "last", ix + 1 == collectionSize }
				});

			xml::element clone(*node);
			clone.attributes().erase(attr->get_qname());

			auto i = parent->emplace(node, std::move(clone)); // insert before processing, to assign namespaces
			process_node(i, subscope, dir, loader);

			++ix;
		}
	}

	return AttributeAction::remove;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_attr(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	auto v = evaluate_el_attr(scope, attr->value());
	for (auto vi: v)
		node->set_attribute(vi.first, vi.second);

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_generic(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	auto s = attr->value();

	process_el(scope, s);
	node->set_attribute(attr->name(), s);

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_boolean_value(
	xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	auto s = attr->value();

	if (evaluate_el(scope, s))
		node->set_attribute(attr->name(), attr->name());
	else
		node->attributes().erase(attr->name());

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_inline(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	auto type = attr->value();

	if (type == "javascript" or type == "css")
	{
		std::regex r = std::regex(R"(/\*\[\[(.+?)\]\]\*/\s*('([^'\\]|\\.)*'|"([^"\\]|\\.)*"|[^;\n])*|\[\[(.+?)\]\])");

		for (auto& n: node->nodes())
		{
			xml::text* text = dynamic_cast<xml::text*>(&n);
			if (text == nullptr)
				continue;

			std::string s = text->get_text();
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
				
				object obj = evaluate_el(scope, v);
				std::stringstream ss;
				ss << obj;
				v = ss.str();
				
				t.append(v.begin(), v.end());
			}

			t.append(i, s.end());

			text->set_text(t);
		}
	}
	else if (type != "none")
	{
		for (auto& n: node->nodes())
		{
			xml::text* text_p = dynamic_cast<xml::text*>(&n);
			if (text_p == nullptr)
				continue;
			
			auto& text = *text_p;

			std::string s = text.get_text();
			auto next = text.next();

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

				if (not process_el(scope, m))
					m = "Error processing " + m;
				
				if (c2 == '(' and m.find('<') != std::string::npos)		// 'unescaped' text, but since we're an xml library reverse this by parsing the result and putting the 
				{
					xml::document subDoc("<foo>" + m + "</foo>");
					for (auto& subnode: subDoc.front().nodes())
						node->nodes().emplace(next, std::move(subnode));
					
					text.set_text(s.substr(0, i - 2));

					s = s.substr(j + 2);
					b = 0;
					node->nodes().insert(next, xml::text(s));
				}
				else
				{
					s.replace(i - 2, j - i + 4, m);
					b = i + m.length() - 2;
				}
			}

			text.set_text(s);
		}
	}

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_include(xml::element* node, xml::attribute* attr, scope& parentScope, std::filesystem::path dir, basic_template_processor& loader, TemplateIncludeAction tia)
{
	AttributeAction result = AttributeAction::none;

	auto av = attr->value();

	auto o = evaluate_el_link(parentScope, av);
	json::element params;

	if (o.is_object())
		params = o["selector"]["params"];

	auto templates = resolve_fragment_spec(node, dir, loader, o, parentScope);

	for (auto& templ: templates)
	{
		xml::element* el = dynamic_cast<xml::element*>(templ.get());

		if (el == nullptr)
		{
			if (tia == TemplateIncludeAction::include)
			{
				auto i = node->nodes().emplace(node->end(), *templ);
				process_node(i, parentScope, dir, loader);
			}
			else
			{
				xml::element::node_iterator i;

				if (tia == TemplateIncludeAction::insert)
					i = node->nodes().emplace(node->end(), *templ);
				else
				{
					i = node->parent()->nodes().emplace(node, *templ);
					result = AttributeAction::remove;
				}

				process_node(i, parentScope, dir, loader);
			}

			continue;
		}

		// take a full copy, and fix up the prefixes for the namespaces, if required
		xml::element& replacement(*el);

		scope scope(parentScope);

		for (auto& f: el->attributes())
		{
			// the copy lost its namespace info
			if (node->namespace_for_prefix(f.get_prefix()) != ns() or f.name() != "fragment")
				continue;

			auto v = f.value();
			auto s = v.find('(');
			auto p = params.begin();

			while (s != std::string::npos and p != params.end())
			{
				s += 1;
				auto e = v.find_first_of(",)", s);
				if (e == std::string::npos)
					break;
				
				auto argname = v.substr(s, e - s);

				auto& po = *p;

				if (po.is_object())
				{
					json::element pe{
						{ "is-node-set", true },
						{ "node-set-name", argname }
					};
					scope.put(argname, pe);

					auto ns = resolve_fragment_spec(node, dir, loader, po, parentScope);
					if (ns.empty())
						scope.put(argname, po);
					else
						scope.set_nodeset(argname, std::move(ns));
				}
				else
					scope.put(argname, po);

				++p;
				s = e;
			}
			
			break;
		}

		if (tia == TemplateIncludeAction::include)
		{
			for (auto& child: replacement.nodes())
			{
				auto i = node->nodes().emplace(node->end(), std::move(child));
				process_node(i, scope, dir, loader);
			}
		}
		else
		{
			xml::element::node_iterator i;

			if (tia == TemplateIncludeAction::insert)
				i = node->emplace(node->end(), std::move(replacement));
			else
			{
				i = node->parent()->emplace(node, std::move(replacement));
				result = AttributeAction::remove;
			}

			auto e2 = dynamic_cast<xml::element*>(&*i);
			if (e2 != nullptr)
			{
				auto& attrs = e2->attributes();

				// if (templateID[0] == '#')	// remove the copied ID
				// 	attr.erase("id");
				
				attrs.erase(i->prefix_tag("ref", ns()));
				attrs.erase(i->prefix_tag("fragment", ns()));
			}

			process_node(i, scope, dir, loader);
		}
	}

	if (result == AttributeAction::remove)
		node->parent()->flatten_text();
	else
		node->flatten_text();

	return result;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_remove(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	auto mode = attr->value();

	AttributeAction result = AttributeAction::none;

	if (mode == "all")
		result = AttributeAction::remove;
	else if (mode == "body")
		node->erase(node->begin(), node->end());
	else if (mode == "all-but-first")
	{
		if (node->size() > 1)
			node->erase(node->begin() + 1, node->end());
	}
	else if (mode == "tag")
	{
		auto i = xml::element::iterator(node);

		for (auto& c: *node)
		{
			i = node->parent()->emplace(i, std::move(c));
			// process_node(i, scope, dir, loader);
			++i;
		}

		result = AttributeAction::remove;
	}
		
	return result;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_classappend(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	for (;;)
	{
		auto s = attr->value();

		s = process_el_2(scope, s);

		trim(s);

		if (s.empty())
			break;
		
		auto c = node->attributes().find("class");

		if (c == node->attributes().end())
		{
			node->attributes().emplace({"class", s});
			break;
		}
		
		auto cs = c->value();
		trim(cs);

		if (cs.empty())
			c->set_text(s);
		else
			c->set_text(cs + ' ' + s);

		break;
	}

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_styleappend(xml::element* node, xml::attribute* attr, scope& scope, std::filesystem::path dir, basic_template_processor& loader)
{
	for (;;)
	{
		auto s = attr->value();

		s = process_el_2(scope, s);

		trim(s);

		if (s.empty())
			break;
		
		if (s.back() != ';')
			s += ';';
		
		auto c = node->attributes().find("style");

		if (c == node->attributes().end())
		{
			node->attributes().emplace({"style", s});
			break;
		}
		
		auto cs = c->value();
		trim(cs);

		if (cs.empty())
		{
			c->set_text(s);
			break;
		}

		if (cs.back() == ';')
			c->set_text(cs + ' ' + s);
		else
			c->set_text(cs + "; " + s);

		break;
	}

	return AttributeAction::none;
}

}