//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <zeep/http/tag-processor.hpp>
#include <zeep/http/template-processor.hpp>

namespace fs = std::filesystem;

namespace zeep::http
{

#if ZEEP_SUPPORT_TAG_PROCESSOR_V1

// --------------------------------------------------------------------
//

tag_processor_v1::tag_processor_v1(const char *ns)
	: tag_processor(ns)
{
}

bool tag_processor_v1::process_el(const scope &scope, std::string &s)
{
	bool replaced = false;

	size_t b = 0;

	while (b < s.length())
	{
		auto i = s.find('$', b);
		if (i == std::string::npos)
			break;

		char c2 = s[i + 1];
		if (c2 != '{')
		{
			b = i + 1;
			continue;
		}

		auto j = s.find('}', i);
		if (j == std::string::npos)
			break;

		j += 1;

		replaced = true;

		auto m = s.substr(i, j - i);

		if (process_el(scope, m))
			s.replace(i, j - i, m);
		else
			s.erase(i, j - i);

		b = j;
	}

	return replaced;
}

void tag_processor_v1::process_xml(xml::node *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	xml::text *text = dynamic_cast<xml::text *>(node);

	if (text != nullptr)
	{
		std::string s = text->get_text();

		if (process_el(scope, s))
			text->set_text(s);

		return;
	}

	xml::element *e = dynamic_cast<xml::element *>(node);
	if (e == nullptr)
		return;

	// if node is one of our special nodes, we treat it here
	if (e->get_ns() == m_ns)
	{
		xml::element *parent = e->parent();

		try
		{
			auto nested(scope);

			process_tag(e->name(), e, scope, dir, loader);
		}
		catch (exception &ex)
		{
			parent->nodes().push_back(
				xml::text("Error processing directive '" + e->get_qname() + "': " + ex.what()));
		}

		try
		{
			parent->erase(e);
		}
		catch (exception &ex)
		{
			std::clog << "exception: " << ex.what() << '\n'
					  << *e << '\n';
		}
	}
	else
	{
		for (auto &a : e->attributes())
		{
			std::string s = a.value();
			if (process_el(scope, s))
				a.value(s);
		}

		std::vector<xml::element *> nodes{ e->begin(), e->end() };
		for (auto n : nodes)
			process_xml(n, scope, dir, loader);
	}
}

void tag_processor_v1::process_tag(const std::string &tag, xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	if (tag == "include")
		process_include(node, scope, dir, loader);
	else if (tag == "if")
		process_if(node, scope, dir, loader);
	else if (tag == "iterate")
		process_iterate(node, scope, dir, loader);
	else if (tag == "for")
		process_for(node, scope, dir, loader);
	else if (tag == "number")
		process_number(node, scope, dir, loader);
	else if (tag == "options")
		process_options(node, scope, dir, loader);
	else if (tag == "option")
		process_option(node, scope, dir, loader);
	else if (tag == "checkbox")
		process_checkbox(node, scope, dir, loader);
	// else if (tag == "url")		process_url(node, scope, dir, loader);
	else if (tag == "param")
		process_param(node, scope, dir, loader);
	else if (tag == "embed")
		process_embed(node, scope, dir, loader);
	else
		throw exception("unimplemented <m1:" + tag + "> tag");
}

void tag_processor_v1::process_include(xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	// an include directive, load file and include resulting content
	std::string file = node->get_attribute("file");

	process_el(scope, file);

	if (file.empty())
		throw exception("missing file attribute");

	xml::document doc;
	doc.set_preserve_cdata(true);
	loader.load_template((dir / file).string(), doc);

	process_xml(&doc.front(), scope, (dir / file).parent_path(), loader);

	auto parent = node->parent();
	parent->insert(node, std::move(doc.front()));
}

void tag_processor_v1::process_if(xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	std::string test = node->get_attribute("test");
	if (evaluate_el(scope, test))
	{
		auto parent = node->parent();
		assert(parent);

		for (auto &c : *node)
		{
			auto copy = parent->emplace(node, std::move(c)); // insert before processing, to assign namespaces
			process_xml(copy, scope, dir, loader);
		}
	}
}

void tag_processor_v1::process_iterate(xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	using json::detail::value_type;

	object collection = scope[node->get_attribute("collection")];
	if (collection.type() == value_type::string or collection.type() == value_type::null)
		collection = evaluate_el(scope, node->get_attribute("collection"));

	std::string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	auto parent = node->parent();
	assert(parent);

	for (object &o : collection)
	{
		auto s(scope);
		s.put(var, o);

		for (auto &c : *node)
		{
			auto i = parent->emplace(node, c); // insert before processing, to assign namespaces
			process_xml(&*i, s, dir, loader);
		}
	}
}

void tag_processor_v1::process_for(xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	object b = evaluate_el(scope, node->get_attribute("begin"));
	object e = evaluate_el(scope, node->get_attribute("end"));

	std::string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	for (int32_t i = b.as<int32_t>(); i <= e.as<int32_t>(); ++i)
	{
		auto s(scope);
		s.put(var, object(i));

		auto parent = node->parent();
		assert(parent);

		for (auto &c : *node)
		{
			auto i2 = parent->emplace(node, c); // insert before processing, to assign namespaces
			process_xml(i2, s, dir, loader);
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

void tag_processor_v1::process_number(xml::element *node, const scope &scope, fs::path /*dir*/, basic_template_processor & /*loader*/)
{
	std::string number = node->get_attribute("n");
	std::string format = node->get_attribute("f");

	if (format == "#,##0B") // bytes, convert to a human readable form
	{
		const char kBase[] = { 'B', 'K', 'M', 'G', 'T', 'P', 'E' }; // whatever

		uint64_t nr = evaluate_el(scope, number).as<uint64_t>();
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
	else if (format.empty() or starts_with(format, "#,##0"))
	{
		uint64_t nr = evaluate_el(scope, number).as<uint64_t>();

		std::locale mylocale(std::locale(), new with_thousands);

		std::ostringstream s;
		s.imbue(mylocale);
		s << nr;
		number = s.str();
	}

	auto parent = node->parent();
	parent->nodes().emplace(node, zeep::xml::text(number));
}

void tag_processor_v1::process_options(xml::element *node, const scope &scope, fs::path /*dir*/, basic_template_processor & /*loader*/)
{
	using ::zeep::json::detail::value_type;

	object collection = scope[node->get_attribute("collection")];
	if (collection.type() == value_type::string or collection.type() == value_type::null)
		collection = evaluate_el(scope, node->get_attribute("collection"));

	if (collection.is_array())
	{
		std::string value = node->get_attribute("value");
		std::string label = node->get_attribute("label");

		std::string selected = node->get_attribute("selected");
		if (not selected.empty())
			process_el(scope, selected);

		for (object &o : collection)
		{
			zeep::xml::element option("option");

			if (not(value.empty() or label.empty()))
			{
				option.set_attribute("value", o[value].as<std::string>());
				if (selected == o[value].as<std::string>())
					option.set_attribute("selected", "selected");
				option.add_text(o[label].as<std::string>());
			}
			else
			{
				option.set_attribute("value", o.as<std::string>());
				if (selected == o.as<std::string>())
					option.set_attribute("selected", "selected");
				option.add_text(o.as<std::string>());
			}

			auto parent = node->parent();
			assert(parent);
			parent->emplace(node, std::move(option));
		}
	}
}

void tag_processor_v1::process_option(xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	std::string value = node->get_attribute("value");
	if (not value.empty())
		process_el(scope, value);

	std::string selected = node->get_attribute("selected");
	if (not selected.empty())
		process_el(scope, selected);

	zeep::xml::element option("option");

	option.set_attribute("value", value);
	if (selected == value)
		option.set_attribute("selected", "selected");

	auto parent = node->parent();
	assert(parent);
	parent->emplace(node, std::move(option));

	for (auto &c : *node)
	{
		auto i = option.emplace(option.end(), c);
		process_xml(i, scope, dir, loader);
	}
}

void tag_processor_v1::process_checkbox(xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	std::string name = node->get_attribute("name");
	if (not name.empty())
		process_el(scope, name);

	bool checked = false;
	if (evaluate_el(scope, node->get_attribute("checked")))
		checked = true;

	zeep::xml::element checkbox("input");
	checkbox.set_attribute("type", "checkbox");
	checkbox.set_attribute("name", name);
	checkbox.set_attribute("value", "true");
	if (checked)
		checkbox.set_attribute("checked", "true");

	auto parent = node->parent();
	assert(parent);
	parent->emplace(node, std::move(checkbox));

	for (auto &c : *node)
	{
		auto i = checkbox.emplace(checkbox.end(), c);
		process_xml(i, scope, dir, loader);
	}
}

// void tag_processor_v1::process_url(xml::element *node, const scope& scope, fs::path dir, basic_template_processor& loader)
// {
// 	std::string var = node->get_attr("var");

// 	parameter_map parameters;
// 	get_parameters(scope, parameters);

// 	for (zeep::xml::element *e : *node)
// 	{
// 		if (e->ns() == m_ns and e->name() == "param")
// 		{
// 			std::string name = e->get_attr("name");
// 			std::string value = e->get_attr("value");

// 			process_el(scope, value);
// 			parameters.replace(name, value);
// 		}
// 	}

// 	std::string url = scope["baseuri"].as<std::string>();

// 	bool first = true;
// 	for (parameter_map::value_type p : parameters)
// 	{
// 		url += (first ? '?' : '&');
// 		first = false;

// 		url += zeep::http::encode_url(p.first) + '=' + zeep::http::encode_url(p.second.as<std::string>());
// 	}

// 	scope& s(const_cast<scope& >(scope));
// 	s.put(var, url);
// }

void tag_processor_v1::process_param(xml::element * /*node*/, const scope & /*scope*/, fs::path /*dir*/, basic_template_processor & /*loader*/)
{
	throw exception("Invalid XML, cannot have a stand-alone mrs:param element");
}

void tag_processor_v1::process_embed(xml::element *node, const scope &scope, fs::path dir, basic_template_processor &loader)
{
	// an embed directive, load xml from attribute and include parsed content
	std::string xml = scope[node->get_attribute("var")].as<std::string>();

	if (xml.empty())
		throw exception("Missing var attribute in embed tag");

	zeep::xml::document doc;
	doc.set_preserve_cdata(true);

	std::istringstream os(xml);
	os >> doc;

	auto parent = node->parent();
	auto i = parent->emplace(node, std::move(doc.front()));

	process_xml(i, scope, dir, loader);
}

#endif

// --------------------------------------------------------------------
} // namespace zeep::http
