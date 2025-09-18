// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2025
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "revision.hpp"

#include "zeep/http/reply.hpp"
#include "zeep/http/uri.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

namespace zeep::http
{

namespace detail
{

	struct status_string
	{
		status_type code;
		std::string_view text;
	} kStatusStrings[] = {
		{ cont, "Continue" },
		{ ok, "OK" },
		{ created, "Created" },
		{ accepted, "Accepted" },
		{ no_content, "No Content" },
		{ multiple_choices, "Multiple Choices" },
		{ moved_permanently, "Moved Permanently" },
		{ moved_temporarily, "Found" },
		{ see_other, "See Other" },
		{ not_modified, "Not Modified" },
		{ bad_request, "Bad Request" },
		{ unauthorized, "Unauthorized" },
		{ proxy_authentication_required, "Proxy Authentication Required" },
		{ forbidden, "Forbidden" },
		{ not_found, "Not Found" },
		{ method_not_allowed, "Method not allowed" },
		{ unprocessable_entity, "Unprocessable Entity" },
		{ internal_server_error, "Internal Server Error" },
		{ not_implemented, "Not Implemented" },
		{ bad_gateway, "Bad Gateway" },
		{ service_unavailable, "Service Unavailable" }
	},
	  kStatusDescriptions[] = { { moved_permanently, "The document requested was moved permanently to a new location" }, { moved_temporarily, "The document requested was moved temporarily to a new location" }, { see_other, "The document can be found at another location" }, { not_modified, "The requested document was not modified" }, { bad_request, "There was an error in the request, e.g. an incorrect method or a malformed URI" }, { unauthorized, "You are not authorized to access this location" }, { proxy_authentication_required, "You are not authorized to use this proxy" }, { forbidden, "Access to this location is forbidden" }, { not_found, "The requested web page was not found on this server." }, { unprocessable_entity, "Your request could not be handled since a parameter contained an invalid value" }, { internal_server_error, "An internal error prevented the server from processing your request" }, { not_implemented, "Your request could not be handled since the required code is not implemented" }, { bad_gateway, "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request. " }, { service_unavailable, "The service is unavailable at this moment, try again later" } };

	const int
		kStatusStringCount = sizeof(kStatusStrings) / sizeof(status_string);

	const int
		kStatusDescriptionCount = sizeof(kStatusDescriptions) / sizeof(status_string);

} // namespace detail

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

reply::reply(status_type status, std::tuple<int, int> version)
	: m_status(status)
	, m_version_major(std::get<0>(version))
	, m_version_minor(std::get<1>(version))
{
	using namespace std::literals;

	auto now = std::chrono::ceil<std::chrono::seconds>(std::chrono::system_clock::now());

	set_header("Date", 
		std::format("{0:%a}, {0:%d} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} GMT", now
	));
	set_header("Server", "libzeep/"s + klibzeepVersionNumber);
	set_header("Content-Length", "0");
}

reply::reply(status_type status, std::tuple<int, int> version,
	std::vector<header> &&headers, std::string &&payload)
	: reply(status, version)
{
	m_headers = std::move(headers);
	m_content = std::move(payload);
}

reply::reply(const reply &rhs)
	: m_status(rhs.m_status)
	, m_version_major(rhs.m_version_major)
	, m_version_minor(rhs.m_version_minor)
	, m_headers(rhs.m_headers)
	, m_data(rhs.m_data)
	, m_content(rhs.m_content)
{
}

reply::~reply()
{
}

void reply::set_version(int version_major, int version_minor)
{
	const std::streambuf::pos_type kNoPos = -1;

	m_version_major = version_major;
	m_version_minor = version_minor;

	// for HTTP/1.0 replies we need to calculate the data length
	if (m_version_major == 1 and m_version_minor == 0 and m_data)
	{
		m_chunked = false;

		std::streamsize length = 0;
		std::streamsize pos = m_data->rdbuf()->pubseekoff(0, std::ios_base::cur);

		if (pos == kNoPos)
		{
			// no other option than copying over the data to our buffer

			char buffer[10240];
			for (;;)
			{
				std::streamsize n = m_data->rdbuf()->sgetn(buffer, sizeof(buffer));
				if (n == 0)
					break;

				length += n;
				m_content.insert(m_content.end(), buffer, buffer + n);
			}

			m_data.reset();
		}
		else
		{
			length = m_data->rdbuf()->pubseekoff(0, std::ios_base::end);
			length -= pos;
			m_data->rdbuf()->pubseekoff(pos, std::ios_base::beg);
		}

		set_header("Content-Length", std::to_string(length));
		remove_header("Transfer-Encoding");
	}
}

void reply::set_header(std::string name, std::string value)
{
	bool updated = false;
	for (header &h : m_headers)
	{
		if (iequals(h.name, name))
		{
			h.value = value;
			updated = true;
			break;
		}
	}

	if (not updated)
	{
		header nh = { std::move(name), std::move(value) };
		m_headers.push_back(nh);
	}
}

std::string reply::get_header(std::string_view name) const
{
	std::string result;

	for (const header &h : m_headers)
	{
		if (iequals(h.name, name))
		{
			result = h.value;
			break;
		}
	}

	return result;
}

void reply::remove_header(std::string_view name)
{
	m_headers.erase(
		std::remove_if(m_headers.begin(), m_headers.end(), [name](header &h)
			{ return iequals(h.name, name); }),
		m_headers.end());
}

void reply::set_cookie(std::string_view name, std::string value, std::initializer_list<cookie_directive> directives)
{
	std::ostringstream vs;
	vs << name << '=' << value;
	for (auto &directive : directives)
		vs << "; " << directive.name << (directive.value.empty() ? "" : "=" + directive.value);

	m_headers.push_back({ "Set-Cookie", vs.str() });
}

void reply::set_delete_cookie(std::string_view name)
{
	using namespace std::literals;

	std::stringstream s;
	const std::time_t now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - 24h);
	s << std::put_time(std::localtime(&now_t), "%a, %d %b %Y %H:%M:%S GMT");

	set_cookie(name, "", { { "Expires", '"' + s.str() + '"' } });
}

std::string reply::get_cookie(std::string_view name) const
{
	std::string result;

	for (const header &h : m_headers)
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

void reply::set_content(const el::object &json)
{
	std::ostringstream s;
	s << json;
	set_content(s.str(), "application/json");
}

void reply::set_content(const mxml::element &data)
{
	std::stringstream s;
	s << data;
	set_content(s.str(), "text/xml; charset=utf-8");
}

void reply::set_content(mxml::document &doc)
{
	std::stringstream s;

	if (doc.front().name() == "html")
		doc.set_write_html(true);
	else
		doc.set_write_doctype(false);

	if (doc.is_html5())
	{
		doc.set_write_doctype(true);
		doc.set_escape_double_quote(false);
	}
	else if (doc.child()->get_ns() == "http://www.w3.org/1999/xhtml")
	{
		doc.set_escape_double_quote(false);
		doc.set_collapse_empty_tags(true);
	}

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

void reply::set_content(std::string data, std::string contentType)
{
	m_content = std::move(data);
	m_status = ok;

	m_data.reset();
	m_chunked = false;

	set_header("Content-Length", std::to_string(m_content.length()));
	remove_header("Transfer-Encoding");
	set_header("Content-Type", std::move(contentType));
}

void reply::set_content(const char *data, size_t size, std::string contentType)
{
	m_content = std::string(data, size);
	m_status = ok;

	m_data.reset();
	m_chunked = false;

	set_header("Content-Length", std::to_string(m_content.length()));
	remove_header("Transfer-Encoding");
	set_header("Content-Type", std::move(contentType));
}

void reply::set_content(std::istream *idata, std::string contentType)
{
	m_data.reset(idata);
	m_content.clear();

	m_status = ok;
	m_chunked = true;

	set_header("Content-Type", std::move(contentType));
	set_header("Transfer-Encoding", "chunked");
	remove_header("Content-Length");
}

std::vector<asio_ns::const_buffer> reply::to_buffers() const
{
	// A global, thread local storage for the status line text
	thread_local static std::string s_status_line;

	std::vector<asio_ns::const_buffer> result;

	s_status_line =
		"HTTP/" + std::to_string(m_version_major) + '.' + std::to_string(m_version_minor) + ' ' + std::to_string(m_status) + ' ' + get_status_text(m_status) + kCRLF;

	result.push_back(asio_ns::buffer(s_status_line));

	for (const header &h : m_headers)
	{
		result.push_back(asio_ns::buffer(h.name));
		result.push_back(asio_ns::buffer(kNameValueSeparator));
		result.push_back(asio_ns::buffer(h.value));
		result.push_back(asio_ns::buffer(kCRLF));
	}

	result.push_back(asio_ns::buffer(kCRLF));
	result.push_back(asio_ns::buffer(m_content));

	return result;
}

std::vector<asio_ns::const_buffer> reply::data_to_buffers()
{
	std::vector<asio_ns::const_buffer> result;

	if (m_data)
	{
		const unsigned int kMaxChunkSize = 10240;

		m_buffer.resize(kMaxChunkSize);
		std::streamsize n = 0;
		try
		{
			n = m_data->rdbuf()->sgetn(m_buffer.data(), m_buffer.size());
		}
		catch (...)
		{
			std::clog << "Exception in reading from file\n";
		}

		// chunked encoding?
		if (m_chunked)
		{
			if (n == 0)
			{
				result.push_back(asio_ns::buffer(kZERO));
				result.push_back(asio_ns::buffer(kCRLF));
				result.push_back(asio_ns::buffer(kCRLF));
				m_data.reset();
			}
			else
			{
				thread_local static std::array<char, 8> s_size_buffer; ///< to store the string with the size for chunked encoding

				const char kHex[] = "0123456789abcdef";
				char *e = s_size_buffer.data() + s_size_buffer.size();
				char *p = e;
				auto l = n;

				while (n != 0)
				{
					*--p = kHex[n & 0x0f];
					n >>= 4;
				}

				result.push_back(asio_ns::buffer(p, e - p));
				result.push_back(asio_ns::buffer(kCRLF));
				result.push_back(asio_ns::buffer(&m_buffer[0], l));
				result.push_back(asio_ns::buffer(kCRLF));
			}
		}
		else
		{
			if (n > 0)
				result.push_back(asio_ns::buffer(&m_buffer[0], n));
			else
				m_data.reset();
		}
	}

	return result;
}

reply reply::stock_reply(status_type status, std::string info)
{
	reply result;

	if (status != not_modified)
	{
		std::stringstream text;

		text << "<html>\n"
			 << "  <body>\n"
			 << "    <h1>" << get_status_text(status) << "</h1>\n";

		if (not info.empty())
		{
			text << "    <p>";

			for (char c : info)
			{
				switch (c)
				{
					case '&': text << "&amp;"; break;
					case '<': text << "&lt;"; break;
					case '>': text << "&gt;"; break;
					case 0: break; // silently ignore
					default:
						if ((c >= 1 and c <= 8) or (c >= 0x0b and c <= 0x0c) or (c >= 0x0e and c <= 0x1f) or c == 0x7f)
							text << "&#" << std::hex << c << ';';
						else
							text << c;
						break;
				}
			}

			text << "</p>\n";
		}

		text << "  </body>\n"
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

reply reply::redirect(const uri &location, status_type status)
{
	reply result;

	result.m_status = status;

	std::string text = get_status_text(status);
	result.m_content =
		"<html><head><title>" + text + "</title></head><body><h1>" +
		std::to_string(status) + ' ' + text + "</h1></body></html>";

	result.set_header("Location", location.string());
	result.set_header("Content-Length", std::to_string(result.m_content.length()));
	result.set_header("Content-Type", "text/html; charset=utf-8");

	return result;
}

reply reply::redirect(const uri &location)
{
	return redirect(location, moved_temporarily);
}

size_t reply::size() const
{
	auto buffers = to_buffers();
	return std::accumulate(buffers.begin(), buffers.end(), 0LL, [](size_t m, auto &buffer)
		{ return m + asio_ns::buffer_size(buffer); });
}

std::ostream &operator<<(std::ostream &lhs, const reply &rhs)
{
	for (auto &b : rhs.to_buffers())
		lhs.write(static_cast<const char *>(b.data()), b.size());

	return lhs;
}

} // namespace zeep::http
