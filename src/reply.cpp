// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#include "revision.hpp"

#include "zeep/http/reply.hpp"
#include "zeep/http/status.hpp"
#include "zeep/uri.hpp"

#include <chrono>
#include <format>
#include <iostream>

namespace zeep::http
{

std::string get_status_description(status_type status)
{
	switch (status)
	{
		case status_type::moved_permanently: return "The document requested was moved permanently to a new location";
		case status_type::moved_temporarily: return "The document requested was moved temporarily to a new location";
		case status_type::see_other: return "The document can be found at another location";
		case status_type::not_modified: return "The requested document was not modified";
		case status_type::bad_request: return "There was an error in the request, e.g. an incorrect method or a malformed URI";
		case status_type::unauthorized: return "You are not authorized to access this location";
		case status_type::proxy_authentication_required: return "You are not authorized to use this proxy";
		case status_type::forbidden: return "Access to this location is forbidden";
		case status_type::not_found: return "The requested web page was not found on this server.";
		case status_type::unprocessable_entity: return "Your request could not be handled since a parameter contained an invalid value";
		case status_type::internal_server_error: return "An internal error prevented the server from processing your request";
		case status_type::not_implemented: return "Your request could not be handled since the required code is not implemented";
		case status_type::bad_gateway: return "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request. ";
		case status_type::service_unavailable: return "The service is unavailable at this moment, try again later";
		default: return "An internal error prevented the server from processing your request";
	}
}

// ----------------------------------------------------------------------------

std::string reply::get_libzeep_version()
{
	return std::format("{}/{}", klibzeepProjectName, klibzeepVersionNumber);
}

// --------------------------------------------------------------------


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
		std::format("{0:%a}, {0:%d} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} GMT", now));
	set_header("Server", get_libzeep_version());
	set_header("Content-Length", "0");
}

reply::reply(status_type status, std::tuple<int, int> version,
	std::vector<header> &&headers, std::string &&payload)
	: reply(status, version)
{
	m_headers = std::move(headers);
	m_content = std::move(payload);
}

reply::reply(const reply &rhs) = default;

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
	std::erase_if(m_headers, [name](header &h)
		{ return iequals(h.name, name); });
}

void reply::set_cookie(std::string_view name, const std::string &value, std::initializer_list<cookie_directive> directives)
{
	std::ostringstream vs;
	vs << name << '=' << value;
	for (auto &directive : directives)
		vs << "; " << directive.name << (directive.value.empty() ? "" : "=" + directive.value);

	m_headers.emplace_back("Set-Cookie", vs.str());
}

void reply::set_delete_cookie(std::string_view name)
{
	using namespace std::literals;

	auto when = std::chrono::system_clock::now() - 24h;
	set_cookie(name, "", { { "Expires", std::format(R"({0:%a}, {0:%d} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} GMT)", when) } });
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

void reply::set_content(const zeem::element &data)
{
	std::stringstream s;
	s << data;
	set_content(s.str(), "text/xml; charset=utf-8");
}

void reply::set_content(zeem::document &doc)
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
	m_data.reset();
	m_content = std::move(data);
	m_chunked = false;

	set_header("Content-Length", std::to_string(m_content.length()));
	remove_header("Transfer-Encoding");
	set_header("Content-Type", std::move(contentType));
}

void reply::set_content(const char *data, size_t size, std::string contentType)
{
	m_data.reset();
	m_content = std::string(data, size);
	m_chunked = false;

	set_header("Content-Length", std::to_string(m_content.length()));
	remove_header("Transfer-Encoding");
	set_header("Content-Type", std::move(contentType));
}

void reply::set_content(std::unique_ptr<std::istream> idata, std::string contentType)
{
	m_data = std::move(idata);
	m_content.clear();
	m_chunked = true;

	set_header("Content-Type", std::move(contentType));
	set_header("Transfer-Encoding", "chunked");
	remove_header("Content-Length");
}

std::vector<std::string_view> reply::to_buffers() const
{
	std::vector<std::string_view> result;

	m_formatted_line =
		std::format("HTTP/{}.{} {} {}\r\n",
			m_version_major, m_version_minor, static_cast<int>(m_status),
			make_error_code(m_status).message());

	result.emplace_back(m_formatted_line);

	for (const header &h : m_headers)
	{
		result.push_back(h.name);
		result.push_back(kNameValueSeparator);
		result.push_back(h.value);
		result.push_back(kCRLF);
	}

	result.push_back(kCRLF);
	result.push_back(m_content);

	return result;
}

std::vector<std::string_view> reply::data_to_buffers()
{
	std::vector<std::string_view> result;

	if (m_data)
	{
		const unsigned int kMaxChunkSize = 10240;

		m_buffer.resize(kMaxChunkSize);
		std::streamsize n = 0;
		try
		{
			n = m_data->rdbuf()->sgetn(m_buffer.data(), static_cast<std::streamsize>(m_buffer.size()));
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
				result.push_back(kZERO);
				result.push_back(kCRLF);
				result.push_back(kCRLF);
				m_data.reset();
			}
			else
			{
				const char kHex[] = "0123456789abcdef";
				char *e = m_size_buffer.data() + m_size_buffer.size();
				char *p = e;
				auto l = n;

				while (n != 0)
				{
					*--p = kHex[n & 0x0f];
					n >>= 4;
				}

				result.emplace_back(p, e - p);
				result.push_back(kCRLF);
				result.emplace_back(&m_buffer[0], l);
				result.push_back(kCRLF);
			}
		}
		else
		{
			if (n > 0)
				result.emplace_back(&m_buffer[0], n);
			else
				m_data.reset();
		}
	}

	return result;
}

reply reply::stock_reply(status_type status, const std::string &info)
{
	reply result;

	if (status != status_type::not_modified)
	{
		std::stringstream text;

		text << "<html>\n"
			 << "  <body>\n"
			 << "    <h1>" << make_error_code(status).message() << "</h1>\n";

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

	std::string text = make_error_code(status).message();
	result.m_content =
		"<html><head><title>" + text + "</title></head><body><h1>" +
		std::to_string(static_cast<int>(status)) + ' ' + text + "</h1></body></html>";

	result.set_header("Location", location.string());
	result.set_header("Content-Length", std::to_string(result.m_content.length()));
	result.set_header("Content-Type", "text/html; charset=utf-8");

	return result;
}

reply reply::redirect(const uri &location)
{
	return redirect(location, status_type::moved_temporarily);
}

size_t reply::size() const
{
	size_t result = 0;
	for (auto &b : to_buffers())
		result += b.size();
	return result;
}

std::ostream &operator<<(std::ostream &lhs, const reply &rhs)
{
	for (auto &b : rhs.to_buffers())
		lhs << b;

	return lhs;
}

} // namespace zeep::http
