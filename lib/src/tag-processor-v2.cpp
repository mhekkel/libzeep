//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

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

    register_attr_handler("if", std::bind(&tag_processor_v2::process_attr_if, this, _1, _2, _3, _4, _5));
    register_attr_handler("text", std::bind(&tag_processor_v2::process_attr_text, this, _1, _2, _3, _4, _5, true));
    register_attr_handler("utext", std::bind(&tag_processor_v2::process_attr_text, this, _1, _2, _3, _4, _5, false));
    register_attr_handler("each", std::bind(&tag_processor_v2::process_attr_each, this, _1, _2, _3, _4, _5));
    register_attr_handler("attr", std::bind(&tag_processor_v2::process_attr_attr, this, _1, _2, _3, _4, _5));
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

	try
	{
		el::scope scope(parentScope);

		std::vector<xml::attribute*> attributes;
		for (auto& attr: e->attributes())
			attributes.push_back(attr);
		
		sort(attributes.begin(), attributes.end(), [](auto a, auto b) { return attribute_precedence(a) < attribute_precedence(b); });

		for (auto attr: attributes)
		{
			if (attr->ns() != m_ns)
				continue;
			
			if (attr->name() == "inline")
			{

			}
			else
			{
				auto h = m_attr_handlers.find(attr->name());

				AttributeAction action;
				if (h != m_attr_handlers.end())
					action = h->second(e, attr, scope, dir, webapp);
				else
					action = process_attr_generic(e, attr, scope, dir, webapp);

				if (action == AttributeAction::remove)
				{
					parent->remove(node);
					node = nullptr;
					break;
				}
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
		// make a copy of the list since process_xml might remove eleemnts
		std::list<xml::node *> nodes;
		copy(e->node_begin(), e->node_end(), back_inserter(nodes));

		for (xml::node *n : nodes)
			process_xml(n, parentScope, dir, webapp);
	}
}

auto tag_processor_v2::process_attr_if(xml::element* element, xml::attribute* attr, const el::scope& scope, fs::path dir, basic_webapp& webapp) -> AttributeAction
{
	return el::evaluate_el(scope, attr->value()) ? AttributeAction::none : AttributeAction::remove;
}

auto tag_processor_v2::process_attr_text(xml::element* element, xml::attribute* attr, const el::scope& scope, fs::path dir, basic_webapp& webapp, bool escaped) ->AttributeAction
{
	el::element obj;
	el::evaluate_el(scope, attr->value(), obj);

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

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_each(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	std::regex kEachRx(R"(^\s*(\w+)(?:\s*,\s*(\w+))?\s*:\s*(.+)$)");

	std::smatch m;
	auto s = attr->str();

	if (not std::regex_match(s, m, kEachRx))
		throw std::runtime_error("Invalid attribute value for :each");

	std::string var = m[1];
	std::string stat = m[2];
	
	el::object collection;
	el::evaluate_el(scope, m[3], collection);

	if (not collection.is_array())
		throw std::runtime_error("Collection is not an array in :each");

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

	return AttributeAction::remove;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_attr(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	auto v = el::evaluate_el_attr(scope, attr->str());
	for (auto vi: v)
		node->set_attribute(vi.first, vi.second);

	return AttributeAction::none;
}

// --------------------------------------------------------------------

tag_processor_v2::AttributeAction tag_processor_v2::process_attr_generic(xml::element* node, xml::attribute* attr, const el::scope& scope, boost::filesystem::path dir, basic_webapp& webapp)
{
	auto s = attr->str();

	el::process_el(scope, s);
	node->set_attribute(attr->name(), s);

	return AttributeAction::none;
}


}
}