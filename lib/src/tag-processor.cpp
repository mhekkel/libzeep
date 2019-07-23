//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

// #include <cstdlib>

// #include <map>
// #include <set>

#include <boost/format.hpp>
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

tag_processor::tag_processor(template_loader& tl, const std::string& ns)
    : m_template_loader(tl), m_ns(ns)
{
	using std::placeholders::_1;
	using std::placeholders::_2;
	using std::placeholders::_3;

	m_processor_table["include"] = bind(&tag_processor::process_include, this, _1, _2, _3);
	m_processor_table["if"] = bind(&tag_processor::process_if, this, _1, _2, _3);
	m_processor_table["iterate"] = bind(&tag_processor::process_iterate, this, _1, _2, _3);
	m_processor_table["for"] = bind(&tag_processor::process_for, this, _1, _2, _3);
	m_processor_table["number"] = bind(&tag_processor::process_number, this, _1, _2, _3);
	m_processor_table["options"] = bind(&tag_processor::process_options, this, _1, _2, _3);
	m_processor_table["option"] = bind(&tag_processor::process_option, this, _1, _2, _3);
	m_processor_table["checkbox"] = bind(&tag_processor::process_checkbox, this, _1, _2, _3);
	m_processor_table["url"] = bind(&tag_processor::process_url, this, _1, _2, _3);
	m_processor_table["param"] = bind(&tag_processor::process_param, this, _1, _2, _3);
	m_processor_table["embed"] = bind(&tag_processor::process_embed, this, _1, _2, _3);
}

tag_processor::~tag_processor()
{
}

void tag_processor::process_xml(xml::node *node, const el::scope& scope, fs::path dir)
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

			processor_map::iterator p = m_processor_table.find(e->name());
			if (p != m_processor_table.end())
				p->second(e, scope, dir);
			else
				throw exception((boost::format("unimplemented <mrs:%1%> tag") % e->name()).str());
		}
		catch (exception& ex)
		{
			xml::node *replacement = new xml::text(
				(boost::format("Error processing directive 'mrs:%1%': %2%") %
				 e->name() % ex.what())
					.str());

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
			process_xml(n, scope, dir);
		}
	}
}

void tag_processor::add_processor(const std::string& name, processor_type processor)
{
	m_processor_table[name] = processor;
}

void tag_processor::process_include(xml::element *node, const el::scope& scope, fs::path dir)
{
	// an include directive, load file and include resulting content
	std::string file = node->get_attribute("file");
	process_el(scope, file);

	if (file.empty())
		throw exception("missing file attribute");

	xml::document doc;
	doc.set_preserve_cdata(true);
	m_template_loader.load_template(dir / file, doc);

	xml::element *replacement = doc.child();
	doc.root()->remove(replacement);

	xml::container *parent = node->parent();
	parent->insert(node, replacement);

	process_xml(replacement, scope, (dir / file).parent_path());
}

void tag_processor::process_if(xml::element *node, const el::scope& scope, fs::path dir)
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
			process_xml(clone, scope, dir);
		}
	}
}

void tag_processor::process_iterate(xml::element *node, const el::scope& scope, fs::path dir)
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
			process_xml(clone, s, dir);
		}
	}
}

void tag_processor::process_for(xml::element *node, const el::scope& scope, fs::path dir)
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
			process_xml(clone, s, dir);
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

void tag_processor::process_number(xml::element *node, const el::scope& scope, fs::path dir)
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

void tag_processor::process_options(xml::element *node, const el::scope& scope, fs::path dir)
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

void tag_processor::process_option(xml::element *node, const el::scope& scope, fs::path dir)
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
		process_xml(clone, scope, dir);
	}
}

void tag_processor::process_checkbox(xml::element *node, const el::scope& scope, fs::path dir)
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
		process_xml(clone, scope, dir);
	}
}

void tag_processor::process_url(xml::element *node, const el::scope& scope, fs::path dir)
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

void tag_processor::process_param(xml::element *node, const el::scope& scope, fs::path dir)
{
	throw exception("Invalid XML, cannot have a stand-alone mrs:param element");
}

void tag_processor::process_embed(xml::element *node, const el::scope& scope, fs::path dir)
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

	process_xml(replacement, scope, dir);
}

}
}