// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem/fstream.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/http/webapp/el.hpp>

using namespace std;
namespace ba = boost::algorithm;
namespace io = boost::iostreams;
namespace fs = boost::filesystem;

namespace zeep { namespace http {

// --------------------------------------------------------------------
//

webapp::webapp(
	const std::string&	ns,
	const fs::path&		docroot)
	: m_ns(ns)
	, m_docroot(docroot)
{
	m_processor_table["include"] =	boost::bind(&webapp::process_include,	this, _1, _2, _3);
	m_processor_table["if"] =		boost::bind(&webapp::process_if,		this, _1, _2, _3);
	m_processor_table["iterate"] =	boost::bind(&webapp::process_iterate,	this, _1, _2, _3);
	m_processor_table["for"] =		boost::bind(&webapp::process_for,		this, _1, _2, _3);
	m_processor_table["number"] =	boost::bind(&webapp::process_number,	this, _1, _2, _3);
	m_processor_table["options"] =	boost::bind(&webapp::process_options,	this, _1, _2, _3);
	m_processor_table["option"] =	boost::bind(&webapp::process_option,	this, _1, _2, _3);
	m_processor_table["checkbox"] =	boost::bind(&webapp::process_checkbox,	this, _1, _2, _3);
	m_processor_table["url"] =		boost::bind(&webapp::process_url, 		this, _1, _2, _3);
	m_processor_table["param"] =	boost::bind(&webapp::process_param,		this, _1, _2, _3);
	m_processor_table["embed"] =	boost::bind(&webapp::process_embed,		this, _1, _2, _3);
}

webapp::~webapp()
{
}

void webapp::set_docroot(
	const fs::path&		path)
{
	m_docroot = path;
}

void webapp::handle_request(
	const request&		req,
	reply&				rep)
{
	string uri = req.uri;

	// shortcut, only handle GET, POST and PUT
	if (req.method != "GET" and req.method != "POST" and req.method != "PUT")
	{
		rep = reply::stock_reply(bad_request);
		return;
	}

	try
	{
		// start by sanitizing the request's URI, first parse the parameters
		string ps = req.payload;
		if (req.method != "POST")
		{
			string::size_type d = uri.find('?');
			if (d != string::npos)
			{
				ps = uri.substr(d + 1);
				uri.erase(d, string::npos);
			}
		}
		
		// strip off the http part including hostname and such
		if (ba::starts_with(uri, "http://"))
		{
			string::size_type s = uri.find_first_of('/', 7);
			if (s != string::npos)
				uri.erase(0, s);
		}
		
		// now make the path relative to the root
		while (uri.length() > 0 and uri[0] == '/')
			uri.erase(uri.begin());
		
		// decode the path elements
		string action = uri;
		string::size_type s = action.find('/');
		if (s != string::npos)
			action.erase(s, string::npos);
		
		// set up the scope by putting some globals in it
		el::scope scope(req);
		scope.put("action", el::object(action));
		scope.put("uri", el::object(uri));
		s = uri.find('?');
		if (s != string::npos)
			uri.erase(s, string::npos);
		scope.put("baseuri", uri);
		
		init_scope(scope);
		
		handler_map::iterator handler = m_dispatch_table.find(uri);
		if (handler == m_dispatch_table.end())
			handler = m_dispatch_table.find(action);

		if (handler != m_dispatch_table.end())
			handler->second(req, scope, rep);
		else
			throw not_found;
	}
	catch (status_type& s)
	{
		rep = reply::stock_reply(s);
	}
	catch (std::exception& e)
	{
		el::scope scope(req);
		scope.put("errormsg", el::object(e.what()));

		create_reply_from_template("error.html", scope, rep);
	}
}

void webapp::mount(const std::string& path, handler_type handler)
{
	m_dispatch_table[path] = handler;
}

void webapp::handle_file(
	const zeep::http::request&	request,
	const el::scope&			scope,
	zeep::http::reply&			reply)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;
	
	fs::path file = get_docroot() / scope["baseuri"].as<string>();

	string ifModifiedSince;
	foreach (const zeep::http::header& h, request.headers)
	{
		if (h.name == "If-Modified-Since")
		{
			local_date_time modifiedSince(local_sec_clock::local_time(time_zone_ptr()));

			local_time_input_facet* lif1(new local_time_input_facet("%a, %d %b %Y %H:%M:%S GMT"));
			
			stringstream ss;
			ss.imbue(std::locale(std::locale::classic(), lif1));
			ss.str(h.value);
			ss >> modifiedSince;
			
			local_date_time fileDate(from_time_t(fs::last_write_time(file)), time_zone_ptr());
			
			if (fileDate <= modifiedSince)
			{
				reply = zeep::http::reply::stock_reply(zeep::http::not_modified);
				return;
			}
			
			break;
		}
	}
	
	fs::ifstream in(file, ios::binary);
	stringstream out;

	io::copy(in, out);

	string mimetype = "text/plain";

	if (file.extension() == ".css")
		mimetype = "text/css";
	else if (file.extension() == ".js")
		mimetype = "text/javascript";
	else if (file.extension() == ".png")
		mimetype = "image/png";
	else if (file.extension() == ".svg")
		mimetype = "image/svg+xml";
	else if (file.extension() == ".html" or file.extension() == ".htm")
		mimetype = "text/html";
	else if (file.extension() == ".xhtml")
		mimetype = "application/xhtml+xml";

	reply.set_content(out.str(), mimetype);

	local_date_time t(local_sec_clock::local_time(time_zone_ptr()));
	local_time_facet* lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));
	
	stringstream s;
	s.imbue(std::locale(std::cout.getloc(), lf));
	
	ptime pt = from_time_t(boost::filesystem::last_write_time(file));
	local_date_time t2(pt, time_zone_ptr());
	s << t2;

	reply.set_header("Last-Modified", s.str());
}

void webapp::load_template(
	const std::string&	file,
	xml::document&		doc)
{
	fs::ifstream data(m_docroot / file, ios::binary);
	if (not data.is_open())
		throw exception((boost::format("file not found: '%1%'") % (m_docroot / file)).str());
	doc.read(data);
}

void webapp::create_reply_from_template(
	const std::string&	file,
	const el::scope&	scope,
	reply&				reply)
{
	xml::document doc;
	doc.set_preserve_cdata(true);
	
	load_template(file, doc);
	
	xml::element* root = doc.child();
	process_xml(root, scope, "/");
	reply.set_content(doc);
}

void webapp::process_xml(
	xml::node*			node,
	const el::scope&	scope,
	fs::path			dir)
{
	xml::text* text = dynamic_cast<xml::text*>(node);
	
	if (text != nullptr)
	{
		string s = text->str();
		if (el::process_el(scope, s))
			text->str(s);
		return;
	}
	
	xml::element* e = dynamic_cast<xml::element*>(node);
	if (e == nullptr)
		return;
	
	// if node is one of our special nodes, we treat it here
	if (e->ns() == m_ns)
	{
		xml::container* parent = e->parent();

		try
		{
			el::scope nested(scope);
			
			processor_map::iterator p = m_processor_table.find(e->name());
			if (p != m_processor_table.end())
				p->second(e, scope, dir);
			else
				throw exception((boost::format("unimplemented <mrs:%1%> tag") % e->name()).str());
		}
		catch (std::exception& ex)
		{
			xml::node* replacement = new xml::text(
				(boost::format("Error processing directive 'mrs:%1%': %2%") %
					e->name() % ex.what()).str());
			
			parent->insert(e, replacement);
		}

		try
		{
//			assert(parent == e->parent());
//			assert(find(parent->begin(), parent->end(), e) != parent->end());

			parent->remove(e);
			delete e;
		}
		catch (exception& ex)
		{
			cerr << "exception: " << ex.what() << endl;
			cerr << *e << endl;
		}
	}
	else
	{
		foreach (xml::attribute& a, boost::iterator_range<xml::element::attribute_iterator>(e->attr_begin(), e->attr_end()))
		{
			string s = a.value();
			if (process_el(scope, s))
				a.value(s);
		}

		list<xml::node*> nodes;
		copy(e->node_begin(), e->node_end(), back_inserter(nodes));
	
		foreach (xml::node* n, nodes)
		{
			process_xml(n, scope, dir);
		}
	}
}

void webapp::add_processor(
	const std::string&	name,
	processor_type		processor)
{
	m_processor_table[name] = processor;
}

void webapp::process_include(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	// an include directive, load file and include resulting content
	string file = node->get_attribute("file");
	process_el(scope, file);

	if (file.empty())
		throw exception("missing file attribute");
	
	xml::document doc;
	doc.set_preserve_cdata(true);
	load_template(dir / file, doc);

	xml::element* replacement = doc.child();
	doc.root()->remove(replacement);

	xml::container* parent = node->parent();
	parent->insert(node, replacement);
	
	process_xml(replacement, scope, (dir / file).parent_path());
}

void webapp::process_if(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	string test = node->get_attribute("test");
	if (evaluate_el(scope, test))
	{
		foreach (xml::node* c, node->nodes())
		{
			xml::node* clone = c->clone();

			xml::container* parent = node->parent();
			assert(parent);

			parent->insert(node, clone);	// insert before processing, to assign namespaces
			process_xml(clone, scope, dir);
		}
	}
}

void webapp::process_iterate(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	el::object collection = scope[node->get_attribute("collection")];
	if (collection.type() != el::object::array_type)
		evaluate_el(scope, node->get_attribute("collection"), collection);
	
	string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	foreach (el::object& o, collection)
	{
		el::scope s(scope);
		s.put(var, o);
		
		foreach (xml::node* c, node->nodes())
		{
			xml::node* clone = c->clone();

			xml::container* parent = node->parent();
			assert(parent);

			parent->insert(node, clone);	// insert before processing, to assign namespaces
			process_xml(clone, s, dir);
		}
	}
}

void webapp::process_for(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	el::object b, e;
	
	evaluate_el(scope, node->get_attribute("begin"), b);
	evaluate_el(scope, node->get_attribute("end"), e);
	
	string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	for (int32 i = b.as<int32>(); i <= e.as<int32>(); ++i)
	{
		el::scope s(scope);
		s.put(var, el::object(i));
		
		foreach (xml::node* c, node->nodes())
		{
			xml::container* parent = node->parent();
			assert(parent);
			xml::node* clone = c->clone();

			parent->insert(node, clone);	// insert before processing, to assign namespaces
			process_xml(clone, s, dir);
		}
	}
}

class with_thousands : public numpunct<char>
{
  protected:
//	char_type do_thousands_sep() const	{ return tsp; }
	string do_grouping() const			{ return "\03"; }
//	char_type do_decimal_point() const	{ return dsp; }
};

void webapp::process_number(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	string number = node->get_attribute("n");
	string format = node->get_attribute("f");
	
	if (format == "#,##0B")	// bytes, convert to a human readable form
	{
		const char kBase[] = { 'B', 'K', 'M', 'G', 'T', 'P', 'E' };		// whatever

		el::object n;
		evaluate_el(scope, number, n);

		uint64 nr = n.as<uint64>();
		int base = 0;

		while (nr > 1024)
		{
			nr /= 1024;
			++base;
		}
		
		locale mylocale(locale(), new with_thousands);
		
		ostringstream s;
		s.imbue(mylocale);
		s.setf(ios::fixed, ios::floatfield);
		s.precision(1);
		s << nr << ' ' << kBase[base];
		number = s.str();
	}
	else if (format.empty() or ba::starts_with(format, "#,##0"))
	{
		el::object n;
		evaluate_el(scope, number, n);
		
		uint64 nr = n.as<uint64>();
		
		locale mylocale(locale(), new with_thousands);
		
		ostringstream s;
		s.imbue(mylocale);
		s << nr;
		number = s.str();
	}

	zeep::xml::node* replacement = new zeep::xml::text(number);
			
	zeep::xml::container* parent = node->parent();
	parent->insert(node, replacement);
}

void webapp::process_options(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	el::object collection = scope[node->get_attribute("collection")];
	if (collection.type() != el::object::array_type)
		evaluate_el(scope, node->get_attribute("collection"), collection);
	
	string value = node->get_attribute("value");
	string label = node->get_attribute("label");
	
	string selected = node->get_attribute("selected");
	if (not selected.empty())
	{
		el::object o;
		evaluate_el(scope, selected, o);
		selected = o.as<string>();
	}
	
	foreach (el::object& o, collection)
	{
		zeep::xml::element* option = new zeep::xml::element("option");

		if (not (value.empty() or label.empty()))
		{
			option->set_attribute("value", o[value].as<string>());
			if (selected == o[value].as<string>())
				option->set_attribute("selected", "selected");
			option->add_text(o[label].as<string>());
		}
		else
		{
			option->set_attribute("value", o.as<string>());
			if (selected == o.as<string>())
				option->set_attribute("selected", "selected");
			option->add_text(o.as<string>());
		}

		zeep::xml::container* parent = node->parent();
		assert(parent);
		parent->insert(node, option);
	}
}

void webapp::process_option(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	string value = node->get_attribute("value");
	string selected = node->get_attribute("selected");
	if (not selected.empty())
	{
		el::object o;
		evaluate_el(scope, selected, o);
		selected = o.as<string>();
	}

	zeep::xml::element* option = new zeep::xml::element("option");

	option->set_attribute("value", value);
	if (selected == value)
		option->set_attribute("selected", "selected");

	zeep::xml::container* parent = node->parent();
	assert(parent);
	parent->insert(node, option);

	foreach (zeep::xml::node* c, node->nodes())
	{
		zeep::xml::node* clone = c->clone();
		option->push_back(clone);
		process_xml(clone, scope, dir);
	}
}

void webapp::process_checkbox(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	string name = node->get_attribute("name");

	bool checked = false;
	if (not node->get_attribute("checked").empty())
	{
		el::object o;
		evaluate_el(scope, node->get_attribute("checked"), o);
		checked = o.as<bool>();
	}
	
	zeep::xml::element* checkbox = new zeep::xml::element("input");
	checkbox->set_attribute("type", "checkbox");
	checkbox->set_attribute("name", name);
	checkbox->set_attribute("value", "true");
	if (checked)
		checkbox->set_attribute("checked", "true");

	zeep::xml::container* parent = node->parent();
	assert(parent);
	parent->insert(node, checkbox);
	
	foreach (zeep::xml::node* c, node->nodes())
	{
		zeep::xml::node* clone = c->clone();
		checkbox->push_back(clone);
		process_xml(clone, scope, dir);
	}
}

void webapp::process_url(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	string var = node->get_attribute("var");
	
	parameter_map parameters;
	get_parameters(scope, parameters);
	
	foreach (zeep::xml::element* e, *node)
	{
		if (e->ns() == m_ns and e->name() == "param")
		{
			string name = e->get_attribute("name");
			string value = e->get_attribute("value");

			process_el(scope, value);
			parameters.replace(name, value);
		}
	}

	string url = scope["baseuri"].as<string>();

	bool first = true;
	foreach (parameter_map::value_type p, parameters)
	{
		if (first)
			url += '?';
		else
			url += '&';
		first = false;

		url += zeep::http::encode_url(p.first) + '=' + zeep::http::encode_url(p.second.as<string>());
	}

	el::scope& s(const_cast<el::scope&>(scope));
	s.put(var, url);
}

void webapp::process_param(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	throw exception("Invalid XML, cannot have a stand-alone mrs:param element");
}

void webapp::process_embed(
	xml::element*		node,
	const el::scope&	scope,
	fs::path			dir)
{
	// an embed directive, load xml from attribute and include parsed content
	string xml = scope[node->get_attribute("var")].as<string>();

	if (xml.empty())
		throw exception("Missing var attribute in embed tag");

	zeep::xml::document doc;
	doc.set_preserve_cdata(true);
	doc.read(xml);

	zeep::xml::element* replacement = doc.child();
	doc.root()->remove(replacement);

	zeep::xml::container* parent = node->parent();
	parent->insert(node, replacement);
	
	process_xml(replacement, scope, dir);
}

void webapp::init_scope(
	el::scope&			scope)
{
}		

void webapp::get_cookies(
	const el::scope&	scope,
	parameter_map&		cookies)
{
	const request& req = scope.get_request();
	foreach (const header& h, req.headers)
	{
		if (h.name != "Cookie")
			continue;
		
		vector<string> rawCookies;
		ba::split(rawCookies, h.value, ba::is_any_of(";"));
		
		foreach (string& cookie, rawCookies)
		{
			ba::trim(cookie);
			cookies.add(cookie);
		}
	}
}

void webapp::get_parameters(
	const el::scope&	scope,
	parameter_map&		parameters)
{
	const request& req = scope.get_request();
	
	string ps;
	
	if (req.method == "POST")
		ps = req.payload;
	else if (req.method == "GET" or req.method == "PUT")
	{
		string::size_type d = req.uri.find('?');
		if (d != string::npos)
			ps = req.uri.substr(d + 1);
	}

	while (not ps.empty())
	{
		string::size_type e = ps.find('&');
		string param;
		
		if (e != string::npos)
		{
			param = ps.substr(0, e);
			ps.erase(0, e + 1);
		}
		else
			swap(param, ps);
		
		if (not param.empty())
			parameters.add(param);
	}
}

// --------------------------------------------------------------------
//

// add a name/value pair as a string formatted as 'name=value'
void parameter_map::add(
	const string&		param)
{
	string name, value;
	
	string::size_type d = param.find('=');
	if (d != string::npos)
	{
		name = param.substr(0, d);
		value = param.substr(d + 1);
	}
	
	add(name, value);
}

void parameter_map::add(
	string		name,
	string		value)
{
	name = decode_url(name);
	if (not value.empty())
		value = decode_url(value);
	
	insert(make_pair(name, parameter_value(value, false)));
}

void parameter_map::replace(
	string			name,
	string			value)
{
	if (count(name))
		erase(lower_bound(name), upper_bound(name));
	add(name, value);
}

}
}
