// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

#include <cstdlib>

#include <map>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/random/random_device.hpp>

#include <boost/format.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/el/process.hpp>
#include <zeep/http/md5.hpp>
#include <zeep/http/tag-processor.hpp>

namespace ba = boost::algorithm;
namespace io = boost::iostreams;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace zeep
{
namespace http
{

// --------------------------------------------------------------------
//

// // add a name/value pair as a std::string formatted as 'name=value'
// void parameter_map::add(const std::string& param)
// {
// 	std::string name, value;

// 	std::string::size_type d = param.find('=');
// 	if (d != std::string::npos)
// 	{
// 		name = param.substr(0, d);
// 		value = param.substr(d + 1);
// 	}

// 	add(name, value);
// }

// void parameter_map::add(std::string name, std::string value)
// {
// 	name = decode_url(name);
// 	if (not value.empty())
// 		value = decode_url(value);

// 	insert(make_pair(name, parameter_value(value, false)));
// }

// void parameter_map::replace(std::string name, std::string value)
// {
// 	if (count(name))
// 		erase(lower_bound(name), upper_bound(name));
// 	add(name, value);
// }

// --------------------------------------------------------------------
//

struct auth_info
{
	auth_info(const std::string& realm);

	bool validate(method_type method, const std::string& uri, const std::string& ha1, std::map<std::string, std::string>& info);

	std::string get_challenge() const;
	bool stale() const;

	std::string m_nonce, m_realm;
	std::set<uint32_t> m_replay_check;
	pt::ptime m_created;
};

auth_info::auth_info(const std::string& realm)
	: m_realm(realm)
{
	using namespace boost::gregorian;

	boost::random::random_device rng;
	uint32_t data[4] = {rng(), rng(), rng(), rng()};

	m_nonce = md5(data, sizeof(data)).finalise();
	m_created = pt::second_clock::local_time();
}

std::string auth_info::get_challenge() const
{
	std::string challenge = "Digest ";
	challenge += "realm=\"" + m_realm + "\", qop=\"auth\", nonce=\"" + m_nonce + '"';
	return challenge;
}

bool auth_info::stale() const
{
	pt::time_duration age = pt::second_clock::local_time() - m_created;
	return age.total_seconds() > 1800;
}

bool auth_info::validate(method_type method, const std::string& uri, const std::string& ha1, std::map<std::string, std::string>& info)
{
	bool valid = false;

	uint32_t nc = strtol(info["nc"].c_str(), nullptr, 16);
	if (not m_replay_check.count(nc))
	{
		std::string ha2 = md5(to_string(method) + std::string{':'} + info["uri"]).finalise();

		std::string response = md5(
								   ha1 + ':' +
								   info["nonce"] + ':' +
								   info["nc"] + ':' +
								   info["cnonce"] + ':' +
								   info["qop"] + ':' +
								   ha2)
								   .finalise();

		valid = info["response"] == response;

		// keep a list of seen nc-values in m_replay_check
		m_replay_check.insert(nc);
	}
	return valid;
}

// --------------------------------------------------------------------
//

basic_webapp::basic_webapp(const fs::path& docroot)
	: m_docroot(docroot), m_tag_processor(nullptr)
{
}

basic_webapp::~basic_webapp()
{
}

void basic_webapp::set_tag_processor(tag_processor* p)
{
	if (m_tag_processor != nullptr)
		delete m_tag_processor;
	m_tag_processor = p;
}

void basic_webapp::handle_request(const request& req, reply& rep)
{
	std::string uri = req.uri;

	// shortcut, check for supported method
	if (req.method != method_type::GET and req.method != method_type::POST and req.method != method_type::PUT and
		req.method != method_type::OPTIONS and req.method != method_type::HEAD and req.method != method_type::DELETE)
	{
		create_error_reply(req, bad_request, rep);
		return;
	}

	try
	{
		// start by sanitizing the request's URI, first parse the parameters
		std::string ps = req.payload;
		if (req.method != method_type::POST)
		{
			std::string::size_type d = uri.find('?');
			if (d != std::string::npos)
			{
				ps = uri.substr(d + 1);
				uri.erase(d, std::string::npos);
			}
		}

		// strip off the http part including hostname and such
		if (ba::starts_with(uri, "http://"))
		{
			std::string::size_type s = uri.find_first_of('/', 7);
			if (s != std::string::npos)
				uri.erase(0, s);
		}

		// now make the path relative to the root
		while (uri.length() > 0 and uri[0] == '/')
			uri.erase(uri.begin());

		// decode the path elements
		std::string action = uri;
		std::string::size_type s = action.find('/');
		if (s != std::string::npos)
			action.erase(s, std::string::npos);

		// set up the scope by putting some globals in it
		el::scope scope(req);

		scope.put("action", el::object(action));
		scope.put("uri", el::object(uri));
		s = uri.find('?');
		if (s != std::string::npos)
			uri.erase(s, std::string::npos);
		scope.put("baseuri", uri);
		scope.put("mobile", req.is_mobile());

		auto handler = find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
							   [uri](const mount_point& m) -> bool { return m.path == uri; });

		if (handler == m_dispatch_table.end())
		{
			handler = find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
							  [action](const mount_point& m) -> bool { return m.path == action; });
		}

		if (handler != m_dispatch_table.end())
		{
			if (req.method == method_type::OPTIONS)
			{
				rep = reply::stock_reply(ok);
				rep.set_header("Allow", "GET,HEAD,POST,OPTIONS");
				rep.set_content("", "text/plain");
			}
			else
			{
				// Do authentication here, if needed
				if (not handler->realm.empty())
				{
					// #pragma message("This sucks, please fix")
					// this is extremely dirty...
					auto& r = const_cast<request& >(req);
					validate_authentication(r, handler->realm);

					scope.put("username", req.username);
				}

				init_scope(scope);

				handler->handler(req, scope, rep);

				if (req.method == method_type::HEAD)
					rep.set_content("", rep.get_content_type());
			}
		}
		else
			throw not_found;
	}
	catch (unauthorized_exception& e)
	{
		create_unauth_reply(req, e.m_stale, e.m_realm, rep);
	}
	catch (status_type& s)
	{
		create_error_reply(req, s, rep);
	}
	catch (std::exception& e)
	{
		create_error_reply(req, internal_server_error, e.what(), rep);
	}
}

void basic_webapp::create_unauth_reply(const request& req, bool stale, const std::string& realm, const std::string& authenticate, reply& rep)
{
	std::unique_lock<std::mutex> lock(m_auth_mutex);

	create_error_reply(req, unauthorized, get_status_text(unauthorized), rep);

	m_auth_info.push_back(auth_info(realm));

	std::string challenge = m_auth_info.back().get_challenge();
	if (stale)
		challenge += ", stale=\"true\"";

	rep.set_header(authenticate, challenge);
}

void basic_webapp::create_error_reply(const request& req, status_type status, reply& rep)
{
	create_error_reply(req, status, "", rep);
}

void basic_webapp::create_error_reply(const request& req, status_type status, const std::string& message, reply& rep)
{
	el::scope scope(req);

	el::object error;
	error["nr"] = static_cast<int>(status);
	error["head"] = get_status_text(status);
	error["description"] = get_status_description(status);

	if (not message.empty())
		error["message"] = message;

	el::object request;
	request["line"] =
		ba::starts_with(req.uri, "http://") ? (boost::format("%1% %2% HTTP%3%/%4%") % to_string(req.method) % req.uri % req.http_version_major % req.http_version_minor).str() : (boost::format("%1% http://%2%%3% HTTP%4%/%5%") % to_string(req.method) % req.get_header("Host") % req.uri % req.http_version_major % req.http_version_minor).str();
	request["username"] = req.username;
	error["request"] = request;

	scope.put("error", error);

	create_reply_from_template("error.html", scope, rep);
	rep.set_status(status);
}

void basic_webapp::handle_file(const zeep::http::request& request,
	const el::scope& scope, zeep::http::reply& reply)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;

	fs::path file = get_docroot() / scope["baseuri"].as<std::string>();
	if (not fs::exists(file))
	{
		reply = zeep::http::reply::stock_reply(not_found);
		return;
	}

	std::string ifModifiedSince;
	for (const zeep::http::header& h : request.headers)
	{
		if (ba::iequals(h.name, "If-Modified-Since"))
		{
			local_date_time modifiedSince(local_sec_clock::local_time(time_zone_ptr()));

			local_time_input_facet *lif1(new local_time_input_facet("%a, %d %b %Y %H:%M:%S GMT"));

			std::stringstream ss;
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

	fs::ifstream in(file, std::ios::binary);
	std::stringstream out;

	io::copy(in, out);

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

	ptime pt = from_time_t(boost::filesystem::last_write_time(file));
	local_date_time t2(pt, time_zone_ptr());
	s << t2;

	reply.set_header("Last-Modified", s.str());
}

void basic_webapp::set_docroot(const fs::path& path)
{
	m_docroot = path;
}

void basic_webapp::load_template(const std::string& file, xml::document& doc)
{
	fs::ifstream data(m_docroot / file, std::ios::binary);
	if (not data.is_open())
	{
		if (not fs::exists(m_docroot))
			throw exception((boost::format("configuration error, docroot not found: '%1%'") % m_docroot).str());
		else
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
			throw exception((boost::format("error opening: %1% (%2%)") % (m_docroot / file) % strerror(errno)).str());
#endif
		}
	}
	doc.read(data);
}

void basic_webapp::create_reply_from_template(const std::string& file, const el::scope& scope, reply& reply)
{
	xml::document doc;
	doc.set_preserve_cdata(true);

	load_template(file, doc);

	if (m_tag_processor != nullptr)
		m_tag_processor->process_xml(doc.child(), scope, "/");

	doc.set_doctype("html", "", "about:legacy-compat");
	
	reply.set_content(doc);

	reply.set_content_type("text/html; charset=utf-8");
}

void basic_webapp::init_scope(el::scope& scope)
{
}

// --------------------------------------------------------------------
//

std::string basic_webapp::validate_authentication(const std::string& authorization,
												  method_type method, const std::string& uri, const std::string& realm)
{
	if (authorization.empty())
		throw unauthorized_exception(false, realm);

	// That was easy, now check the response

	std::map<std::string, std::string> info;

	boost::regex re("(\\w+)=(?|\"([^\"]*)\"|'([^']*)'|(\\w+))(?:,\\s*)?");
	const char *b = authorization.c_str();
	const char *e = b + authorization.length();
	boost::match_results<const char *> m;
	while (b < e and boost::regex_search(b, e, m, re))
	{
		info[std::string(m[1].first, m[1].second)] = std::string(m[2].first, m[2].second);
		b = m[0].second;
	}

	if (info["realm"] != realm)
		throw unauthorized_exception(false, realm);

	std::string ha1 = get_hashed_password(info["username"], realm);

	// lock to avoid accessing m_auth_info from multiple threads at once
	std::unique_lock<std::mutex> lock(m_auth_mutex);
	bool authorized = false, stale = false;

	for (auto auth = m_auth_info.begin(); auth != m_auth_info.end(); ++auth)
	{
		if (auth->m_realm == realm and auth->m_nonce == info["nonce"] and auth->validate(method, uri, ha1, info))
		{
			authorized = true;
			stale = auth->stale();
			if (stale)
				m_auth_info.erase(auth);
			break;
		}
	}

	if (stale or not authorized)
		throw unauthorized_exception(stale, realm);

	return info["username"];
}

std::string basic_webapp::get_hashed_password(const std::string& username, const std::string& realm)
{
	return "";
}

// --------------------------------------------------------------------
//

webapp::webapp(const std::string& ns, const boost::filesystem::path& docroot)
	: basic_webapp(docroot)
{
	set_tag_processor(new tag_processor(*this, ns));
}

webapp::~webapp()
{
}

void webapp::handle_request(const request& req, reply& rep)
{
	server::log() << req.uri;
	basic_webapp::handle_request(req, rep);
}

} // namespace http
} // namespace zeep
