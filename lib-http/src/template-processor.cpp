//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/chrono.hpp>

#include <zeep/xml/xpath.hpp>
#include <zeep/http/template-processor.hpp>

namespace io = boost::iostreams;
namespace fs = std::filesystem;

namespace zeep::http
{

// --------------------------------------------------------------------
//

file_loader::file_loader(const std::filesystem::path& docroot)
	: resource_loader(), m_docroot(docroot)
{
	if (not docroot.empty() and not std::filesystem::exists(m_docroot))
		throw std::runtime_error("Docroot '" + m_docroot.string() + "' does not seem to exist");
}

/// return last_write_time of \a file
std::filesystem::file_time_type file_loader::file_time(const std::string& file, std::error_code& ec) noexcept
{
	return fs::last_write_time(m_docroot / file, ec);
}

/// return last_write_time of \a file
std::istream* file_loader::load_file(const std::string& file, std::error_code& ec) noexcept
{
	std::ifstream* result = new std::ifstream(m_docroot / file, std::ios::binary);
	if (not result->is_open())
	{
		delete result;
		result = nullptr;
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
	}
	
	return result;	
}

// --------------------------------------------------------------------
//

void basic_template_processor::handle_file(const http::request& request, const scope& scope, http::reply& reply)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;

	std::error_code ec;
	auto ft = file_time(scope["baseuri"].as<std::string>(), ec);

	if (ec)
	{
		reply = http::reply::stock_reply(http::not_found);
		return;
	}

	auto lastWriteTime = std::chrono::duration_cast<std::chrono::seconds>(ft - decltype(ft)::clock::time_point{}).count();

	// // std::chrono::time_point<std::filesystem::__file_clock> tp(ft);
	// ptime()

	// std::chrono::system_clock::from_time_t()

	// // auto lastWriteTime = decltype(ft)::clock::to_time_t(ft);

	// boost::chrono::duration_cast

	// std::chrono::file_clock;
	// ptime fpt(ft);
	// auto lastWriteTime = to_time_t()

	std::string ifModifiedSince;
	for (const http::header& h : request.headers)
	{
		if (iequals(h.name, "If-Modified-Since"))
		{
			local_date_time modifiedSince(local_sec_clock::local_time(time_zone_ptr()));

			local_time_input_facet *lif1(new local_time_input_facet("%a, %d %b %Y %H:%M:%S GMT"));

			std::stringstream ss;
			ss.imbue(std::locale(std::locale::classic(), lif1));
			ss.str(h.value);
			ss >> modifiedSince;

			local_date_time fileDate(from_time_t(lastWriteTime), time_zone_ptr());

			if (fileDate <= modifiedSince)
			{
				reply = http::reply::stock_reply(http::not_modified);
				return;
			}

			break;
		}
	}

	fs::path file = scope["baseuri"].as<std::string>();

	std::unique_ptr<std::istream> in(load_file(file, ec));
	if (ec)
	{
		reply = http::reply::stock_reply(http::not_found);
		return;
	}

	std::stringstream out;

	io::copy(*in, out);

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

	reply.set_content(out.str(), mimetype);

	local_date_time t(local_sec_clock::local_time(time_zone_ptr()));
	local_time_facet *lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));

	std::stringstream s;
	s.imbue(std::locale(std::cout.getloc(), lf));

	ptime pt = from_time_t(lastWriteTime);
	local_date_time t2(pt, time_zone_ptr());
	s << t2;

	reply.set_header("Last-Modified", s.str());
}

void basic_template_processor::set_docroot(const fs::path& path)
{
	m_docroot = path;
}

std::tuple<bool, std::filesystem::path> basic_template_processor::is_template_file(const std::string& file)
{
	bool found = false;
	fs::path template_file;

	for (const char* ext: { "", ".xhtml", ".html", ".xml" })
	{
		std::error_code ec;

		template_file = file + ext;

		(void)file_time(template_file, ec);

		if (ec)
			continue;

		found = true;

		break;
	}
	
	return { found, template_file };
}

void basic_template_processor::load_template(const std::string& file, xml::document& doc)
{
	std::string templateSelector;

	json::element spec;
	std::unique_ptr<std::istream> data;
	std::error_code ec;

	bool regularTemplate;
	fs::path templateFile;

	std::tie(regularTemplate, templateFile) = is_template_file(file);

	if (regularTemplate)
		data.reset(load_file(templateFile.string(), ec));
	else
	{
		auto spec = evaluate_el({}, file);

		if (spec.is_object())	// reset the content, saves having to add another method
		{
			std::tie(regularTemplate, templateFile) = is_template_file(spec["template"].as<std::string>());

			if (regularTemplate)
				data.reset(load_file(templateFile.string(), ec));

			templateSelector = spec["selector"]["xpath"].as<std::string>();
		}
	}

	if (not data)
	{
#if defined(_MSC_VER)
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

		throw exception((boost::format("error opening: %1% (%2%)") % (m_docroot / file) % msg).str());
#else
		throw exception("error opening: " + m_docroot.string() + " (" + strerror(errno) + ")");
#endif
	}

	doc.set_preserve_cdata(true);
	try
	{
		*data >> doc;
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Error parsing template: " << ex.what() << std::endl;
		throw;
	}

	if (not templateSelector.empty())
	{
		// tricky? Find first matching fragment and make it the root node of the document

		xml::context ctx;

		// this is problematic, take the first processor namespace for now.
		// TODO fix this
		std::string ns;

		for (auto& tp: m_tag_processor_creators)
		{
			std::unique_ptr<tag_processor> ptp(tp.second(tp.first));
			if (dynamic_cast<tag_processor_v2*>(ptp.get()) == nullptr)
				continue;

			ns = tp.first;
			ctx.set("ns", ns);
			break;
		}
		xml::xpath xp(templateSelector);

		std::vector<std::unique_ptr<xml::node>> result;

		for (auto n: xp.evaluate<xml::node>(doc, ctx))
		{
			auto e = dynamic_cast<xml::element*>(n);
			if (e == nullptr)
				continue;

			xml::document dest;

			auto& attr = e->attributes();

			if (spec["selector"]["by-id"])
				attr.erase("id");

			attr.erase(e->prefix_tag("ref", ns));
			attr.erase(e->prefix_tag("fragment", ns));

			auto parent = e->parent();
			dest.push_back(std::move(*e));

			xml::fix_namespaces(dest.front(), *parent, dest.front());

			doc.swap(dest);
			break;
		}
	}
}

void basic_template_processor::create_reply_from_template(const std::string& file, const scope& scope, http::reply& reply)
{
	xml::document doc;
	doc.set_preserve_cdata(true);

	load_template(file, doc);

	process_tags(doc.child(), scope);

	reply.set_content(doc);
}

void basic_template_processor::init_scope(scope& scope)
{
}

void basic_template_processor::process_tags(xml::node* node, const scope& scope)
{
	// only process elements
	if (dynamic_cast<xml::element*>(node) == nullptr)
		return;

	std::set<std::string> registeredNamespaces;
	for (auto& tpc: m_tag_processor_creators)
		registeredNamespaces.insert(tpc.first);

	if (not registeredNamespaces.empty())
		process_tags(static_cast<xml::element*>(node), scope, registeredNamespaces);

	// decorate all forms with a hidden input with name _csrf

	auto csrf = scope.get_csrf_token();
	if (not csrf.empty())
	{
		auto forms = xml::xpath(R"(//form[not(input[@name='_csrf'])])");
		xml::context ctx;
		for (auto& form: forms.evaluate<xml::element>(*node, ctx))
			form->emplace_back(xml::element("input", {
				{ "name", "_csrf" },
				{ "value", csrf },
				{ "type", "hidden" }
			}));
	}
}

void basic_template_processor::process_tags(xml::element* node, const scope& scope, std::set<std::string> registeredNamespaces)
{
	std::set<std::string> nss;

	for (auto& ns: node->attributes())
	{
		if (not ns.is_namespace())
			continue;

		if (registeredNamespaces.count(ns.value()))
			nss.insert(ns.value());
	}

	for (auto& ns: nss)
	{
		std::unique_ptr<tag_processor> processor(create_tag_processor(ns));
		processor->process_xml(node, scope, "", *this);

		registeredNamespaces.erase(ns);
	}

	if (not registeredNamespaces.empty())
	{
		for (auto& e: *node)
			process_tags(&e, scope, registeredNamespaces);
	}
}

} // namespace http::zeep
