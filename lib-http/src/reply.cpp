// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/date_time/local_time/local_time.hpp>

#include <numeric>

#include <zeep/http/reply.hpp>
#include <zeep/xml/document.hpp>

namespace zeep::http
{

namespace detail
{

struct status_string
{
	status_type	code;
	const char*	text;
} kStatusStrings[] = {
	{ cont,					"Continue" },
	{ ok,					"OK" },
	{ created,				"Created" },
	{ accepted,				"Accepted" },
	{ no_content,			"No Content" },
	{ multiple_choices,		"Multiple Choices" },
	{ moved_permanently,	"Moved Permanently" },
	{ moved_temporarily,	"Found" },
	{ not_modified,			"Not Modified" },
	{ bad_request,			"Bad Request" },
	{ unauthorized,			"Unauthorized" },
	{ proxy_authentication_required,	"Proxy Authentication Required" },
	{ forbidden,			"Forbidden" },
	{ not_found,			"Not Found" },
	{ method_not_allowed,	"Method not allowed" },
	{ internal_server_error,"Internal Server Error" },
	{ not_implemented,		"Not Implemented" },
	{ bad_gateway,			"Bad Gateway" },
	{ service_unavailable,	"Service Unavailable" }
}, kStatusDescriptions[] = {
	{ moved_permanently,	"The document requested was moved permanently to a new location" },
	{ moved_temporarily,	"The document requested was moved temporarily to a new location" },
	{ not_modified,			"The requested document was not modified" },
	{ bad_request,			"There was an error in the request, e.g. an incorrect method or a malformed URI" },
	{ unauthorized,			"You are not authorized to access this location" },
	{ proxy_authentication_required,	"You are not authorized to use this proxy" },
	{ forbidden,			"Access to this location is forbidden" },
	{ not_found,			"The requested web page was not found on this server." },
	{ internal_server_error,"An internal error prevented the server from processing your request" },
	{ not_implemented,		"Your request could not be handled since the required code is not implemented" },
	{ bad_gateway,			"The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request. " },
	{ service_unavailable,	"The service is unavailable at this moment, try again later" }
};

const int
	kStatusStringCount = sizeof(kStatusStrings) / sizeof(status_string);

const int
	kStatusDescriptionCount = sizeof(kStatusDescriptions) / sizeof(status_string);
	
}

std::string get_status_text(status_type status)
{
	std::string result = "Internal Service Error";
	
	for (int i = 0; i < detail::kStatusStringCount; ++i)
	{
		if (detail::kStatusStrings[i].code == status)
		{
			result = detail::kStatusStrings[i].text;
			break;
		}
	}
	
	return result;
}
	
std::string get_status_description(status_type status)
{
	std::string result = "An internal error prevented the server from processing your request";
	
	for (int i = 0; i < detail::kStatusDescriptionCount; ++i)
	{
		if (detail::kStatusDescriptions[i].code == status)
		{
			result = detail::kStatusDescriptions[i].text;
			break;
		}
	}
	
	return result;
}
	
// ----------------------------------------------------------------------------

namespace
{
const std::string
		kNameValueSeparator{ ':', ' ' },
		kCRLF{ '\r', '\n' },
		kZERO{ '0' };
}

reply::reply(int version_major, int version_minor)
	: m_status(internal_server_error)
	, m_version_major(version_major)
	, m_version_minor(version_minor)
	, m_data(nullptr)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;

	local_date_time t(local_sec_clock::local_time(time_zone_ptr()));
	local_time_facet* lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));
	
	std::stringstream s;
	s.imbue(std::locale(std::locale(), lf));

	s << t;

	set_header("Date", s.str());
	set_header("Server", "libzeep");
}

reply::reply(status_type status, std::tuple<int,int> version,
		std::vector<header>&& headers, std::string&& payload)
	: m_status(status)
	, m_version_major(std::get<0>(version))
	, m_version_minor(std::get<1>(version))
	, m_headers(std::move(headers))
	, m_content(std::move(payload))
	, m_data(nullptr)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;

	local_date_time t(local_sec_clock::local_time(time_zone_ptr()));
	local_time_facet* lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));
	
	std::stringstream s;
	s.imbue(std::locale(std::locale(), lf));

	s << t;

	set_header("Date", s.str());
	set_header("Server", "libzeep");
}

reply::reply(const reply& rhs)
	: m_status(rhs.m_status)
	, m_version_major(rhs.m_version_major)
	, m_version_minor(rhs.m_version_minor)
	, m_headers(rhs.m_headers)
	, m_content(rhs.m_content)
	, m_data(nullptr)
{
}

reply::~reply()
{
	delete m_data;
}

void reply::clear()
{
	delete m_data;
	m_data = nullptr;
	
	m_status = ok;
	m_headers.clear();
	m_buffer.clear();
	m_content.clear();
}

reply& reply::operator=(const reply& rhs)
{
	if (this != &rhs)
	{
		m_version_major = rhs.m_version_major;
		m_version_minor = rhs.m_version_minor;
		m_status = rhs.m_status;
		m_headers = rhs.m_headers;
		m_content = rhs.m_content;
	}
	
	return *this;
}

void reply::set_version(int version_major, int version_minor)
{
	m_version_major = version_major;
	m_version_minor = version_minor;
}

void reply::set_header(const std::string& name, const std::string& value)
{
	bool updated = false;
	for (header& h: m_headers)
	{
		if (h.name == name)
		{
			h.value = value;
			updated = true;
			break;
		}
	}
	
	if (not updated)
	{
		header nh = { name, value };
		m_headers.push_back(nh);
	}
}

std::string reply::get_header(const std::string& name)
{
	std::string result;

	for (header& h: m_headers)
	{
		if (h.name == name)
		{
			result = h.value;
			break;
		}
	}

	return result;
}

void reply::set_cookie(const char* name, const std::string& value, std::initializer_list<cookie_directive> directives)
{
	std::ostringstream vs;
	vs << name << '=' << value;
	for (auto& directive: directives)
		vs << "; " << directive.name << (directive.value.empty() ? "" : "=" + directive.value);
	
	m_headers.push_back({ "Set-Cookie", vs.str() });
}

std::string reply::get_cookie(const char* name) const
{
	std::string result;

	for (const header& h: m_headers)
	{
		if (iequals(h.name, "Set-Cookie"))
		{
			result = h.value;

			auto ns = result.find('=');
			if (ns == std::string::npos)
				continue;
			
			if (result.compare(0, ns, name) != 0)
				continue;
			
			auto ds = result.find(';', ns + 1);

			result = result.substr(ns + 1, ds - ns - 1);
			break;
		}
	}

	return result;
}

bool reply::keep_alive() const
{
	bool result = false;
	
	for (const header& h: m_headers)
	{
		if (iequals(h.name, "Connection") and iequals(h.value, "keep-alive"))
		{
			result = true;
			break;
		}
	}
	
	return result;
}

void reply::set_content(const json::element& json)
{
	std::ostringstream s;
	s << json;
	set_content(s.str(), "application/json");
}

void reply::set_content(const xml::element& data)
{
	std::stringstream s;
	s << data;
	set_content(s.str(), "text/xml; charset=utf-8");
}

void reply::set_content(xml::document& doc)
{
	std::stringstream s;

	if (doc.front().name() != "html")
		doc.set_write_doctype(false);

	if (doc.is_html5())
	{
		doc.set_write_doctype(true);
		doc.set_escape_double_quote(false);
	}
	else if (doc.child()->get_ns() == "http://www.w3.org/1999/xhtml")
		doc.set_escape_double_quote(false);

	doc.set_collapse_empty_tags(false);

	s << doc;
	
	std::string contentType;
	
	if (doc.is_html5())
		contentType = "text/html; charset=utf-8";
	else if (doc.child()->get_ns() == "http://www.w3.org/1999/xhtml")
		contentType = "application/xhtml+xml; charset=utf-8";
	else
		contentType = "text/xml; charset=utf-8";

	set_content(s.str(), contentType);
}

void reply::set_content(const std::string& data, const std::string& contentType)
{
	m_content = data;
	m_status = ok;

	delete m_data;
	m_data = nullptr;

	set_header("Content-Length", std::to_string(m_content.length()));
	set_header("Content-Type", contentType);
}

void reply::set_content(const char* data, size_t size, const std::string& contentType)
{
	m_content = std::string(data, size);
	m_status = ok;

	delete m_data;
	m_data = nullptr;

	set_header("Content-Length", std::to_string(m_content.length()));
	set_header("Content-Type", contentType);
}

void reply::set_content(std::istream* idata, const std::string& contentType)
{
	delete m_data;
	m_data = idata;
	m_content.clear();

	m_status = ok;

	set_header("Content-Type", contentType);
	
	// for HTTP/1.0 replies we need to calculate the data length
	if (m_version_major == 1 and m_version_minor == 0 and m_data != nullptr)
	{
		std::streamsize pos = m_data->rdbuf()->pubseekoff(0, std::ios_base::cur);
		std::streamsize length = m_data->rdbuf()->pubseekoff(0, std::ios_base::end);
		length -= pos;
		m_data->rdbuf()->pubseekoff(pos, std::ios_base::beg);
		
		set_header("Content-Length", boost::lexical_cast<std::string>(length));
	}
	else
		set_header("Transfer-Encoding", "chunked");
}

std::string reply::get_content_type() const
{
	std::string result;
	
	for (const header& h: m_headers)
	{
		if (iequals(h.name, "Content-Type"))
		{
			result = h.value;
			break;
		}
	}
	
	return result;
}

void reply::set_content_type(
	const std::string& type)
{
	for (header& h: m_headers)
	{
		if (iequals(h.name, "Content-Type"))
		{
			h.value = type;
			break;
		}
	}
}

std::vector<boost::asio::const_buffer> reply::to_buffers() const
{
	std::vector<boost::asio::const_buffer> result;

	m_status_line =
		"HTTP/" + std::to_string(m_version_major) + '.' + std::to_string(m_version_minor) + ' ' + std::to_string(m_status) + ' ' + get_status_text(m_status) + kCRLF;
	
	result.push_back(boost::asio::buffer(m_status_line));
	
	for (const header& h: m_headers)
	{
		result.push_back(boost::asio::buffer(h.name));
		result.push_back(boost::asio::buffer(kNameValueSeparator));
		result.push_back(boost::asio::buffer(h.value));
		result.push_back(boost::asio::buffer(kCRLF));
	}

	result.push_back(boost::asio::buffer(kCRLF));
	result.push_back(boost::asio::buffer(m_content));

	return result;
}

std::vector<boost::asio::const_buffer> reply::data_to_buffers()
{
	std::vector<boost::asio::const_buffer> result;
	
	if (m_data != nullptr)
	{
		const unsigned int kMaxChunkSize = 10240;
		
		if (m_buffer.size() < kMaxChunkSize)
			m_buffer.insert(m_buffer.begin(), kMaxChunkSize, 0);
		else if (m_buffer.size() > kMaxChunkSize)
			m_buffer.erase(m_buffer.begin() + kMaxChunkSize, m_buffer.end());
		
		std::streamsize n = m_data->rdbuf()->sgetn(&m_buffer[0], m_buffer.size());

		// chunked encoding?
		if (m_version_major > 1 or m_version_minor >= 1)
		{
			if (n == 0)
			{
				result.push_back(boost::asio::buffer(kZERO));
				result.push_back(boost::asio::buffer(kCRLF));
				result.push_back(boost::asio::buffer(kCRLF));
				delete m_data;
				m_data = nullptr;
			}
			else
			{
				std::ostringstream os;
				os << std::hex << n << '\r' << '\n';
				os.flush();
				auto s = os.str();
				m_buffer.insert(m_buffer.end(), s.begin(), s.end());
		
				result.push_back(boost::asio::buffer(&m_buffer[kMaxChunkSize], m_buffer.size() - kMaxChunkSize));
				result.push_back(boost::asio::buffer(&m_buffer[0], n));
				result.push_back(boost::asio::buffer(kCRLF));
			}
		}
		else
		{
			if (n > 0)
				result.push_back(boost::asio::buffer(&m_buffer[0], n));
			else
			{
				delete m_data;
				m_data = nullptr;
			}
		}
	}

	return result;
}

reply reply::stock_reply(status_type status, const std::string& info)
{
	reply result;

	if (status != not_modified)
	{
		std::stringstream text;

		text << "<html>" << std::endl
			 << "  <body>" << std::endl
			 << "    <h1>" << get_status_text(status) << "</h1>" << std::endl;
		
		if (not info.empty())
		{
			text << "    <p>";
		
			for (char c: info)
			{
				switch (c)
				{
					case '&':	text << "&amp;"; break;
					case '<':	text << "&lt;"; break;
					case '>':	text << "&gt;"; break;
					case 0:		break;	// silently ignore
					default:	if ((c >= 1 and c <= 8) or (c >= 0x0b and c <= 0x0c) or (c >= 0x0e and c <= 0x1f) or c == 0x7f)
									text << "&#" << std::hex << c << ';';
								else	
									text << c;
								break;
				}
			}
			
			text << "</p>" << std::endl;
		}
		
		text << "  </body>" << std::endl
			 << "</html>";
		result.set_content(text.str(), "text/html; charset=utf-8");
	}

	result.m_status = status;
	
	return result;
}

reply reply::stock_reply(status_type status)
{
	return stock_reply(status, "");
}

reply reply::redirect(const std::string& location)
{
	reply result;

	result.m_status = moved_temporarily;

	std::string text = get_status_text(moved_temporarily);
	result.m_content =
		"<html><head><title>" + text + "</title></head><body><h1>" +
 		std::to_string(moved_temporarily) + ' ' + text + "</h1></body></html>";
	
	result.set_header("Location", location);
	result.set_header("Content-Length", std::to_string(result.m_content.length()));
	result.set_header("Content-Type", "text/html; charset=utf-8");
	
	return result;
}

size_t reply::size() const
{
	auto buffers = to_buffers();
	return std::accumulate(buffers.begin(), buffers.end(), 0LL, [](size_t m, auto& buffer) { return m + boost::asio::buffer_size(buffer); });
}

std::ostream& operator<<(std::ostream& lhs, const reply& rhs)
{
	for (auto& b: rhs.to_buffers())
		lhs.write(boost::asio::buffer_cast<const char*>(b), boost::asio::buffer_size(b));

	return lhs;
}

}
