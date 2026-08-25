// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/template-processor.hpp"

#include "zeep/el/object.hpp"
#include "zeep/el/processing.hpp"
#include "zeep/exception.hpp"
#include "zeep/http/header.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/scope.hpp"
#include "zeep/http/status.hpp"
#include "zeep/http/tag-processor.hpp"
#include "zeep/unicode-support.hpp"

#include <new>
#include <zeem/zeem.hpp>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace zeep::http
{

// --------------------------------------------------------------------
//

auto sanitizePath(const fs::path &dir, const fs::path &file) -> fs::path
{
	std::error_code ec;

	auto result = fs::weakly_canonical(dir / file, ec);
	auto s = result.generic_string();

	auto dirStr = dir.generic_string();
	if (not dirStr.empty() and dirStr.back() != '/')
		dirStr += '/';
	if (ec or not s.starts_with(dirStr))
		result.clear();

	return result;
}

file_loader::file_loader(std::filesystem::path docroot)
	: resource_loader()
	, m_docroot(std::move(docroot))
{
	if (not m_docroot.empty() and not std::filesystem::exists(m_docroot))
		throw invalid_argument_exception("Docroot '" + m_docroot.string() + "' does not seem to exist");
}

/// return last_write_time of \a file
std::filesystem::file_time_type file_loader::file_time(std::filesystem::path file, std::error_code &ec) noexcept
{
	if (file.has_root_path())
		file = fs::relative(file, file.root_path());

	return fs::last_write_time(m_docroot / file, ec);
}

/// return last_write_time of \a file
std::unique_ptr<std::istream> file_loader::load_file(std::string file, std::error_code &ec) noexcept
{
	fs::path path(file);
	if (path.has_root_path())
		path = fs::relative(path, path.root_path());

	path = sanitizePath(std::filesystem::canonical(m_docroot), path);

	std::unique_ptr<std::ifstream> result;

	if (not fs::is_regular_file(path))
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
	else
	{
		try
		{
			result = std::make_unique<std::ifstream>(path, std::ios::binary);
			if (not result->is_open())
			{
				result.reset();
				ec = std::make_error_code(std::errc::no_such_file_or_directory);
			}
		}
		catch (const std::bad_alloc &)
		{
			ec = std::make_error_code(std::errc::not_enough_memory);
			result.reset();
		}
	}

	return result;
}

// --------------------------------------------------------------------
//

reply basic_template_processor::create_reply_for_get_file(const scope &scope)
{
	std::error_code ec;
	auto ft = file_time(scope["baseuri"].get<std::string>(), ec);

	if (ec)
		return reply::stock_reply(status_type::not_found);

	using namespace std::chrono;
	auto fileDate =
		floor<seconds>(time_point_cast<system_clock::duration>(ft - decltype(ft)::clock::now() + system_clock::now()));

	for (const header &h : scope.get_headers())
	{
		if (iequals(h.name, "If-Modified-Since"))
		{
			std::istringstream ss{ h.value };
			std::chrono::time_point<std::chrono::system_clock> modifiedSince;
			ss >> std::chrono::parse("%a, %d %b %Y %H:%M:%S GMT", modifiedSince);

			if (fileDate <= modifiedSince)
				return reply::stock_reply(status_type::not_modified);

			break;
		}
	}

	fs::path file = scope["baseuri"].get<std::string>();

	std::unique_ptr<std::istream> in(load_file(file.string(), ec));
	if (ec)
		return reply::stock_reply(status_type::not_found);

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

	reply result(status_type::ok);
	result.set_content(std::move(in), mimetype);

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

void basic_template_processor::load_template(const std::string &file, zeem::document &doc)
{
	std::string templateSelector;

	object spec;
	std::unique_ptr<std::istream> data;
	std::error_code ec;

	auto templateFile = get_template_file(file);

	if (templateFile.has_value())
		data = load_file(templateFile->string(), ec);
	else
	{
		auto espec = evaluate_el_link({}, file);

		if (espec.is_object()) // reset the content, saves having to add another method
		{
			templateFile = get_template_file(espec["template"].get<std::string>());

			if (templateFile.has_value())
				data = load_file(templateFile->string(), ec);

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

				strncpy(msg, lpMsgBuf, sizeof(msg) - 1);

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

		zeem::context ctx;

		// this is problematic, take the first processor namespace for now.
		// TODO: maarten - fix this
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
		zeem::xpath xp(templateSelector);

		std::vector<std::unique_ptr<zeem::node>> result;

		for (auto n : xp.evaluate<zeem::node>(doc, ctx))
		{
			auto e = dynamic_cast<zeem::element *>(n);
			if (e == nullptr)
				continue;

			zeem::document dest;

			auto &attr = e->attributes();

			if (spec["selector"]["by-id"])
				attr.erase("id");

			attr.erase(e->prefix_tag("ref", ns));
			attr.erase(e->prefix_tag("fragment", ns));

			auto parent = e->parent();
			dest.push_back(std::move(*e));

			if (parent->type() == zeem::node_type::element)
				zeem::fix_namespaces(dest.front(), static_cast<const zeem::element &>(*parent), dest.front());

			std::swap(doc, dest);
			break;
		}
	}
}

void basic_template_processor::create_reply_from_template(const std::string &file, const scope &scope, reply &reply)
{
	zeem::document doc;
	doc.set_preserve_cdata(true);

	load_template(file, doc);

	process_tags(doc.child(), scope);

	doc.set_write_html(true);

	reply.set_content(doc);
}

void basic_template_processor::init_scope(request & /* req */, scope & /*scope*/)
{
}

void basic_template_processor::process_tags(zeem::node *node, const scope &scope)
{
	// only process elements
	if (dynamic_cast<zeem::element *>(node) == nullptr)
		return;

	std::set<std::string> registeredNamespaces;
	for (auto &tpc : m_tag_processor_creators)
		registeredNamespaces.insert(tpc.first);

	if (not registeredNamespaces.empty())
		process_tags(static_cast<zeem::element *>(node), scope, registeredNamespaces);

	// decorate all forms with a hidden input with name _csrf

	auto csrf = scope.get_csrf_token();
	if (not csrf.empty())
	{
		auto forms = zeem::xpath(R"(//form[not(input[@name='_csrf'])])");
		zeem::context ctx;
		for (auto &form : forms.evaluate<zeem::element>(*node, ctx))
			form->emplace_back(zeem::element("input", { { "name", "_csrf" },
														  { "value", csrf },
														  { "type", "hidden" } }));
	}
}

void basic_template_processor::process_tags(zeem::element *node, const scope &scope, std::set<std::string> registeredNamespaces)
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
