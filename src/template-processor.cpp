//        Copyright Maarten L. Hekkelman, 2014-2025
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <fstream>
#include <iostream>

#include "zeep/el/object.hpp"
#include "zeep/http/scope.hpp"
#include "zeep/http/template-processor.hpp"

namespace fs = std::filesystem;

namespace zeep::http
{

// --------------------------------------------------------------------
//

file_loader::file_loader(std::filesystem::path docroot)
	: resource_loader()
	, m_docroot(std::move(docroot))
{
	if (not m_docroot.empty() and not std::filesystem::exists(m_docroot))
		throw std::runtime_error("Docroot '" + m_docroot.string() + "' does not seem to exist");
}

/// return last_write_time of \a file
std::filesystem::file_time_type file_loader::file_time(std::filesystem::path file, std::error_code &ec) noexcept
{
	if (file.has_root_path())
		file = fs::relative(file, file.root_path());

	return fs::last_write_time(m_docroot / file, ec);
}

/// return last_write_time of \a file
std::istream *file_loader::load_file(std::string file, std::error_code &ec) noexcept
{
	fs::path path(file);
	if (path.has_root_path())
		path = fs::relative(path, path.root_path());

	std::ifstream *result = nullptr;

	if (not fs::is_regular_file(m_docroot / path))
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
	else
	{
		result = new std::ifstream(m_docroot / path, std::ios::binary);
		if (not result->is_open())
		{
			delete result;
			result = nullptr;
			ec = std::make_error_code(std::errc::no_such_file_or_directory);
		}
	}

	return result;
}

// --------------------------------------------------------------------
//

reply basic_template_processor::create_reply_for_get_file(const scope &scope)
{
	// TODO: The time used here is local, not GMT. Needs fix?

	std::error_code ec;
	auto ft = file_time(scope["baseuri"].get<std::string>(), ec);

	if (ec)
		return reply::stock_reply(not_found);

	using namespace std::chrono;
	auto fileDate = 
		floor<seconds>(time_point_cast<system_clock::duration>(ft - decltype(ft)::clock::now() + system_clock::now()));

	std::string ifModifiedSince;
	for (const header &h : scope.get_headers())
	{
		if (iequals(h.name, "If-Modified-Since"))
		{
			std::istringstream ss{ h.value };
			std::tm tm;
			ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");

			auto modifiedSince = system_clock::from_time_t(std::mktime(&tm));

			if (fileDate <= modifiedSince)
				return reply::stock_reply(not_modified);

			break;
		}
	}

	fs::path file = scope["baseuri"].get<std::string>();

	std::unique_ptr<std::istream> in(load_file(file.string(), ec));
	if (ec)
		return reply::stock_reply(not_found);

	std::string mimetype = "text/plain";

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
	else if (file.extension() == ".xml" or file.extension() == ".xsl" or file.extension() == ".xslt")
		mimetype = "text/xml";
	else if (file.extension() == ".xhtml")
		mimetype = "application/xhtml+xml";
	else if (file.extension() == ".ico")
		mimetype = "image/x-icon";
	else if (file.extension() == ".json" or file.extension() == ".schema")
		mimetype = "application/json";
	else if (file.extension() == ".bz2")
		mimetype = "application/x-bzip2";
	else if (file.extension() == ".gz")
		mimetype = "application/gzip";

	reply result(ok);
	result.set_content(in.release(), mimetype);

	using namespace std::chrono;

	result.set_header("Last-Modified",
		std::format("{0:%a}, {0:%d} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} GMT", fileDate));

	return result;
}

void basic_template_processor::set_docroot(fs::path path)
{
	m_docroot = std::move(path);
}

std::optional<std::filesystem::path> basic_template_processor::get_template_file(const std::string &file)
{
	std::optional<fs::path> result;

	for (const char *ext : { "", ".xhtml", ".html", ".xml" })
	{
		std::error_code ec;

		fs::path p = file + ext;

		(void)file_time(p.string(), ec);

		if (ec)
			continue;

		result = p;
		break;
	}

	return result;
}

void basic_template_processor::load_template(const std::string &file, mxml::document &doc)
{
	std::string templateSelector;

	object spec;
	std::unique_ptr<std::istream> data;
	std::error_code ec;

	auto templateFile = get_template_file(file);

	if (templateFile.has_value())
		data.reset(load_file(templateFile->string(), ec));
	else
	{
		auto espec = evaluate_el_link({}, file);

		if (espec.is_object()) // reset the content, saves having to add another method
		{
			templateFile = get_template_file(espec["template"].get<std::string>());

			if (templateFile.has_value())
				data.reset(load_file(templateFile->string(), ec));

			templateSelector = espec["selector"]["xpath"].get<std::string>();
		}
	}

	if (not data)
	{
#if defined(_WIN32)
		char msg[1024] = "";

		DWORD dw = ::GetLastError();
		if (dw != NO_ERROR)
		{
			char *lpMsgBuf = nullptr;
			int m = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);

			if (lpMsgBuf != nullptr)
			{
				// strip off the trailing whitespace characters
				while (m > 0 and isspace(lpMsgBuf[m - 1]))
					--m;
				lpMsgBuf[m] = 0;

				strncpy(msg, lpMsgBuf, sizeof(msg));

				::LocalFree(lpMsgBuf);
			}
		}

		throw exception("error opening: " + (m_docroot / file).string() + " (" + msg + ")");
#else
		throw exception("error opening: " + (m_docroot / file).string() + " (" + strerror(errno) + ")");
#endif
	}

	doc.set_preserve_cdata(true);
	try
	{
		*data >> doc;
	}
	catch (const std::exception &ex)
	{
		std::clog << "Error parsing template: " << ex.what() << '\n';
		throw;
	}

	if (not templateSelector.empty())
	{
		// tricky? Find first matching fragment and make it the root node of the document

		mxml::context ctx;

		// this is problematic, take the first processor namespace for now.
		// TODO fix this
		std::string ns;

		for (auto &tp : m_tag_processor_creators)
		{
			std::unique_ptr<tag_processor_base> ptp(tp.second(tp.first));
			if (dynamic_cast<tag_processor *>(ptp.get()) == nullptr)
				continue;

			ns = tp.first;
			ctx.set("ns", ns);
			break;
		}
		mxml::xpath xp(templateSelector);

		std::vector<std::unique_ptr<mxml::node>> result;

		for (auto n : xp.evaluate<mxml::node>(doc, ctx))
		{
			auto e = dynamic_cast<mxml::element *>(n);
			if (e == nullptr)
				continue;

			mxml::document dest;

			auto &attr = e->attributes();

			if (spec["selector"]["by-id"])
				attr.erase("id");

			attr.erase(e->prefix_tag("ref", ns));
			attr.erase(e->prefix_tag("fragment", ns));

			auto parent = e->parent();
			dest.push_back(std::move(*e));

			if (parent->type() == mxml::node_type::element)
				mxml::fix_namespaces(dest.front(), static_cast<const mxml::element &>(*parent), dest.front());

			std::swap(doc, dest);
			break;
		}
	}
}

void basic_template_processor::create_reply_from_template(const std::string &file, const scope &scope, reply &reply)
{
	mxml::document doc;
	doc.set_preserve_cdata(true);

	load_template(file, doc);

	process_tags(doc.child(), scope);

	doc.set_write_html(true);

	reply.set_content(doc);
}

void basic_template_processor::init_scope(request &req, scope & /*scope*/)
{
}

void basic_template_processor::process_tags(mxml::node *node, const scope &scope)
{
	// only process elements
	if (dynamic_cast<mxml::element *>(node) == nullptr)
		return;

	std::set<std::string> registeredNamespaces;
	for (auto &tpc : m_tag_processor_creators)
		registeredNamespaces.insert(tpc.first);

	if (not registeredNamespaces.empty())
		process_tags(static_cast<mxml::element *>(node), scope, registeredNamespaces);

	// decorate all forms with a hidden input with name _csrf

	auto csrf = scope.get_csrf_token();
	if (not csrf.empty())
	{
		auto forms = mxml::xpath(R"(//form[not(input[@name='_csrf'])])");
		mxml::context ctx;
		for (auto &form : forms.evaluate<mxml::element>(*node, ctx))
			form->emplace_back(mxml::element("input", { { "name", "_csrf" },
														  { "value", csrf },
														  { "type", "hidden" } }));
	}
}

void basic_template_processor::process_tags(mxml::element *node, const scope &scope, std::set<std::string> registeredNamespaces)
{
	std::set<std::string> nss;

	for (auto &ns : node->attributes())
	{
		if (not ns.is_namespace())
			continue;

		if (registeredNamespaces.count(ns.value()))
			nss.insert(ns.value());
	}

	for (auto &ns : nss)
	{
		std::unique_ptr<tag_processor_base> processor(create_tag_processor(ns));
		processor->process_xml(node, scope, "", *this);

		registeredNamespaces.erase(ns);
	}

	if (not registeredNamespaces.empty())
	{
		for (auto &e : *node)
			process_tags(&e, scope, registeredNamespaces);
	}
}

} // namespace zeep::http
