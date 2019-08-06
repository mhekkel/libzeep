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
//

tag_processor_v1::tag_processor_v1(const std::string& ns)
    : tag_processor(ns)
{
}

void tag_processor_v1::process_xml(xml::node *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	xml::text *text = dynamic_cast<xml::text *>(node);

	if (text != nullptr)
	{
		std::string s = text->str();
		if (el::process_el(scope, s))
			text->str(s);
		return;
	}

	xml::element *e = dynamic_cast<xml::element *>(node);
	if (e == nullptr)
		return;

	// if node is one of our special nodes, we treat it here
	if (e->ns() == m_ns)
	{
		xml::container *parent = e->parent();

		try
		{
			el::scope nested(scope);

			process_tag(e->name(), e, scope, dir, webapp);
		}
		catch (exception& ex)
		{
			xml::node *replacement = new xml::text(
				"Error processing directive '" + e->prefix() + ":" + e->name() + "': " + ex.what());

			parent->insert(e, replacement);
		}

		try
		{
			parent->remove(e);
			delete e;
		}
		catch (exception& ex)
		{
			std::cerr << "exception: " << ex.what() << std::endl
					  << *e << std::endl;
		}
	}
	else
	{
		for (xml::attribute& a : boost::iterator_range<xml::element::attribute_iterator>(e->attr_begin(), e->attr_end()))
		{
			std::string s = a.value();
			if (process_el(scope, s))
				a.value(s);
		}

		std::list<xml::node *> nodes;
		copy(e->node_begin(), e->node_end(), back_inserter(nodes));

		for (xml::node *n : nodes)
		{
			process_xml(n, scope, dir, webapp);
		}
	}
}

void tag_processor_v1::process_tag(const std::string& tag, xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
		 if (tag == "include")	process_include(node, scope, dir, webapp);
	else if (tag == "if")		process_if(node, scope, dir, webapp);
	else if (tag == "iterate")	process_iterate(node, scope, dir, webapp);
	else if (tag == "for")		process_for(node, scope, dir, webapp);
	else if (tag == "number")	process_number(node, scope, dir, webapp);
	else if (tag == "options")	process_options(node, scope, dir, webapp);
	else if (tag == "option")	process_option(node, scope, dir, webapp);
	else if (tag == "checkbox")	process_checkbox(node, scope, dir, webapp);
	// else if (tag == "url")		process_url(node, scope, dir, webapp);
	else if (tag == "param")	process_param(node, scope, dir, webapp);
	else if (tag == "embed")	process_embed(node, scope, dir, webapp);
	else throw exception("unimplemented <m1:" + tag + "> tag");
}

void tag_processor_v1::process_include(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	// an include directive, load file and include resulting content
	std::string file = node->get_attribute("file");
	process_el(scope, file);

	if (file.empty())
		throw exception("missing file attribute");

	xml::document doc;
	doc.set_preserve_cdata(true);
	webapp.load_template((dir / file).string(), doc);

	xml::element *replacement = doc.child();
	doc.root()->remove(replacement);

	xml::container *parent = node->parent();
	parent->insert(node, replacement);

	process_xml(replacement, scope, (dir / file).parent_path(), webapp);
}

void tag_processor_v1::process_if(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	std::string test = node->get_attribute("test");
	if (evaluate_el(scope, test))
	{
		for (xml::node *c : node->nodes())
		{
			xml::node *clone = c->clone();

			xml::container *parent = node->parent();
			assert(parent);

			parent->insert(node, clone); // insert before processing, to assign namespaces
			process_xml(clone, scope, dir, webapp);
		}
	}
}

void tag_processor_v1::process_iterate(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	using el::detail::value_type;

	el::object collection = scope[node->get_attribute("collection")];
	if (collection.type() != value_type::array)
		evaluate_el(scope, node->get_attribute("collection"), collection);

	std::string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	for (el::object& o : collection)
	{
		el::scope s(scope);
		s.put(var, o);

		for (xml::node *c : node->nodes())
		{
			xml::node *clone = c->clone();

			xml::container *parent = node->parent();
			assert(parent);

			parent->insert(node, clone); // insert before processing, to assign namespaces
			process_xml(clone, s, dir, webapp);
		}
	}
}

void tag_processor_v1::process_for(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	el::object b, e;

	evaluate_el(scope, node->get_attribute("begin"), b);
	evaluate_el(scope, node->get_attribute("end"), e);

	std::string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	for (int32_t i = b.as<int32_t>(); i <= e.as<int32_t>(); ++i)
	{
		el::scope s(scope);
		s.put(var, el::object(i));

		for (xml::node *c : node->nodes())
		{
			xml::container *parent = node->parent();
			assert(parent);
			xml::node *clone = c->clone();

			parent->insert(node, clone); // insert before processing, to assign namespaces
			process_xml(clone, s, dir, webapp);
		}
	}
}

class with_thousands : public std::numpunct<char>
{
protected:
	//	char_type do_thousands_sep() const	{ return tsp; }
	std::string do_grouping() const { return "\03"; }
	//	char_type do_decimal_point() const	{ return dsp; }
};

void tag_processor_v1::process_number(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	std::string number = node->get_attribute("n");
	std::string format = node->get_attribute("f");

	if (format == "#,##0B") // bytes, convert to a human readable form
	{
		const char kBase[] = {'B', 'K', 'M', 'G', 'T', 'P', 'E'}; // whatever

		el::object n;
		evaluate_el(scope, number, n);

		uint64_t nr = n.as<uint64_t>();
		int base = 0;

		while (nr > 1024)
		{
			nr /= 1024;
			++base;
		}

		std::locale mylocale(std::locale(), new with_thousands);

		std::ostringstream s;
		s.imbue(mylocale);
		s.setf(std::ios::fixed, std::ios::floatfield);
		s.precision(1);
		s << nr << ' ' << kBase[base];
		number = s.str();
	}
	else if (format.empty() or ba::starts_with(format, "#,##0"))
	{
		el::object n;
		evaluate_el(scope, number, n);

		uint64_t nr = n.as<uint64_t>();

		std::locale mylocale(std::locale(), new with_thousands);

		std::ostringstream s;
		s.imbue(mylocale);
		s << nr;
		number = s.str();
	}

	zeep::xml::node *replacement = new zeep::xml::text(number);

	zeep::xml::container *parent = node->parent();
	parent->insert(node, replacement);
}

void tag_processor_v1::process_options(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	using ::zeep::el::detail::value_type;

	el::object collection = scope[node->get_attribute("collection")];
	if (collection.type() != value_type::array)
		evaluate_el(scope, node->get_attribute("collection"), collection);

	std::string value = node->get_attribute("value");
	std::string label = node->get_attribute("label");

	std::string selected = node->get_attribute("selected");
	if (not selected.empty())
	{
		el::object o;
		evaluate_el(scope, selected, o);
		selected = o.as<std::string>();
	}

	for (el::object& o : collection)
	{
		zeep::xml::element *option = new zeep::xml::element("option");

		if (not(value.empty() or label.empty()))
		{
			option->set_attribute("value", o[value].as<std::string>());
			if (selected == o[value].as<std::string>())
				option->set_attribute("selected", "selected");
			option->add_text(o[label].as<std::string>());
		}
		else
		{
			option->set_attribute("value", o.as<std::string>());
			if (selected == o.as<std::string>())
				option->set_attribute("selected", "selected");
			option->add_text(o.as<std::string>());
		}

		zeep::xml::container *parent = node->parent();
		assert(parent);
		parent->insert(node, option);
	}
}

void tag_processor_v1::process_option(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	std::string value = node->get_attribute("value");
	if (not value.empty())
	{
		el::object o;
		evaluate_el(scope, value, o);
		value = o.as<std::string>();
	}

	std::string selected = node->get_attribute("selected");
	if (not selected.empty())
	{
		el::object o;
		evaluate_el(scope, selected, o);
		selected = o.as<std::string>();
	}

	zeep::xml::element *option = new zeep::xml::element("option");

	option->set_attribute("value", value);
	if (selected == value)
		option->set_attribute("selected", "selected");

	zeep::xml::container *parent = node->parent();
	assert(parent);
	parent->insert(node, option);

	for (zeep::xml::node *c : node->nodes())
	{
		zeep::xml::node *clone = c->clone();
		option->push_back(clone);
		process_xml(clone, scope, dir, webapp);
	}
}

void tag_processor_v1::process_checkbox(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	std::string name = node->get_attribute("name");
	if (not name.empty())
	{
		el::object o;
		evaluate_el(scope, name, o);
		name = o.as<std::string>();
	}

	bool checked = false;
	if (not node->get_attribute("checked").empty())
	{
		el::object o;
		evaluate_el(scope, node->get_attribute("checked"), o);
		checked = o.as<bool>();
	}

	zeep::xml::element *checkbox = new zeep::xml::element("input");
	checkbox->set_attribute("type", "checkbox");
	checkbox->set_attribute("name", name);
	checkbox->set_attribute("value", "true");
	if (checked)
		checkbox->set_attribute("checked", "true");

	zeep::xml::container *parent = node->parent();
	assert(parent);
	parent->insert(node, checkbox);

	for (zeep::xml::node *c : node->nodes())
	{
		zeep::xml::node *clone = c->clone();
		checkbox->push_back(clone);
		process_xml(clone, scope, dir, webapp);
	}
}

void tag_processor_v1::process_url(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	// std::string var = node->get_attribute("var");

	// parameter_map parameters;
	// get_parameters(scope, parameters);

	// for (zeep::xml::element *e : *node)
	// {
	// 	if (e->ns() == m_ns and e->name() == "param")
	// 	{
	// 		std::string name = e->get_attribute("name");
	// 		std::string value = e->get_attribute("value");

	// 		process_el(scope, value);
	// 		parameters.replace(name, value);
	// 	}
	// }

	// std::string url = scope["baseuri"].as<std::string>();

	// bool first = true;
	// for (parameter_map::value_type p : parameters)
	// {
	// 	url += (first ? '?' : '&');
	// 	first = false;

	// 	url += zeep::http::encode_url(p.first) + '=' + zeep::http::encode_url(p.second.as<std::string>());
	// }

	// el::scope& s(const_cast<el::scope& >(scope));
	// s.put(var, url);
}

void tag_processor_v1::process_param(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	throw exception("Invalid XML, cannot have a stand-alone mrs:param element");
}

void tag_processor_v1::process_embed(xml::element *node, const el::scope& scope, fs::path dir, basic_webapp& webapp)
{
	// an embed directive, load xml from attribute and include parsed content
	std::string xml = scope[node->get_attribute("var")].as<std::string>();

	if (xml.empty())
		throw exception("Missing var attribute in embed tag");

	zeep::xml::document doc;
	doc.set_preserve_cdata(true);
	doc.read(xml);

	zeep::xml::element *replacement = doc.child();
	doc.root()->remove(replacement);

	zeep::xml::container *parent = node->parent();
	parent->insert(node, replacement);

	process_xml(replacement, scope, dir, webapp);
}

}
}