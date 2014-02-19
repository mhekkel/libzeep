//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/format.hpp>

#include <iostream>

#include <zeep/http/reply.hpp>
#include <zeep/xml/document.hpp>
#include <zeep/xml/writer.hpp>

using namespace std;
namespace io = boost::iostreams;

namespace zeep { namespace http {

namespace detail {

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

string get_status_text(status_type status)
{
	string result = "Internal Service Error";
	
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
	
string get_status_description(status_type status)
{
	string result = "An internal error prevented the server from processing your request";
	
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
const char
		kNameValueSeparator[] = { ':', ' ' },
		kCRLF[] = { '\r', '\n' };
}

reply::reply(int version_major, int version_minor)
	: m_version_major(version_major)
	, m_version_minor(version_minor)
	, m_status(internal_server_error)
	, m_data(nullptr)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;

	local_date_time t(local_sec_clock::local_time(time_zone_ptr()));
	local_time_facet* lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));
	
	stringstream s;
	s.imbue(std::locale(std::locale(), lf));

	s << t;

	set_header("Date", s.str());
	set_header("Server", "libzeep");
}

reply::reply(const reply& rhs)
	: m_version_major(rhs.m_version_major)
	, m_version_minor(rhs.m_version_minor)
	, m_status(rhs.m_status)
	, m_status_line(rhs.m_status_line)
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
	m_status_line = "OK";
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
		m_status_line = rhs.m_status_line;
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

void reply::set_header(const string& name, const string& value)
{
	bool updated = false;
	foreach (header& h, m_headers)
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

bool reply::keep_alive() const
{
	bool result = false;
	
	foreach (const header& h, m_headers)
	{
		if (h.name == "Connection" and h.value == "keep-alive")
		{
			result = true;
			break;
		}
	}
	
	return result;
}

void reply::set_content(xml::element* data)
{
	xml::document doc;
	doc.child(data);
	set_content(doc);
}

void reply::set_content(xml::document& doc)
{
	stringstream s;

	xml::writer w(s);
	w.set_wrap(false);
	w.set_indent(0);
//	w.trim(true);
	doc.write(w);
	
	string contentType = "text/xml; charset=utf-8";
	if (doc.child()->ns() == "http://www.w3.org/1999/xhtml")
		contentType = "application/xhtml+xml; charset=utf-8";

	set_content(s.str(), contentType);
}

void reply::set_content(const string& data, const string& contentType)
{
	m_content = data;
	m_status = ok;

	delete m_data;
	m_data = nullptr;

	set_header("Content-Length", boost::lexical_cast<string>(m_content.length()));
	set_header("Content-Type", contentType);
}

void reply::set_content(istream* idata, const string& contentType)
{
	delete m_data;
	m_data = idata;
	m_content.clear();

	m_status = ok;

	set_header("Content-Type", contentType);
	
	// for HTTP/1.0 replies we need to calculate the data length
	if (m_version_major == 1 and m_version_minor == 0 and m_data != nullptr)
	{
		streamsize pos = m_data->rdbuf()->pubseekoff(0, ios_base::cur);
		streamsize length = m_data->rdbuf()->pubseekoff(0, ios_base::end);
		length -= pos;
		m_data->rdbuf()->pubseekoff(pos, ios_base::beg);
		
		set_header("Content-Length", boost::lexical_cast<string>(length));
	}
	else
		set_header("Transfer-Encoding", "chunked");
}

string reply::get_content_type() const
{
	string result;
	
	foreach (const header& h, m_headers)
	{
		if (h.name == "Content-Type")
		{
			result = h.value;
			break;
		}
	}
	
	return result;
}

void reply::set_content_type(
	const string& type)
{
	foreach (header& h, m_headers)
	{
		if (h.name == "Content-Type")
		{
			h.value = type;
			break;
		}
	}
}

void reply::to_buffers(vector<boost::asio::const_buffer>& buffers)
{
	m_status_line = (boost::format("HTTP/%1%.%2% %3% %4%\r\n")
		% m_version_major % m_version_minor % m_status % get_status_text(m_status)).str();
	buffers.push_back(boost::asio::buffer(m_status_line));
	
	foreach (header& h, m_headers)
	{
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(kNameValueSeparator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(kCRLF));
	}

	buffers.push_back(boost::asio::buffer(kCRLF));
	buffers.push_back(boost::asio::buffer(m_content));
}

bool reply::data_to_buffers(vector<boost::asio::const_buffer>& buffers)
{
	bool result = false;
	
	if (m_data != nullptr)
	{
		result = true;
		
		const unsigned int kMaxChunkSize = 10240;
		
		if (m_buffer.size() < kMaxChunkSize)
			m_buffer.insert(m_buffer.begin(), kMaxChunkSize, 0);
		else if (m_buffer.size() > kMaxChunkSize)
			m_buffer.erase(m_buffer.begin() + kMaxChunkSize, m_buffer.end());
		
		streamsize n = m_data->readsome(&m_buffer[0], m_buffer.size());

		// chunked encoding?
		if (m_version_major > 1 or m_version_minor >= 1)
		{
			if (n == 0)
			{
				buffers.push_back(boost::asio::buffer("0"));
				buffers.push_back(boost::asio::buffer(kCRLF));
				buffers.push_back(boost::asio::buffer(kCRLF));
				delete m_data;
				m_data = nullptr;
			}
			else
			{
				io::filtering_ostream out(io::back_inserter(m_buffer));
				out << std::hex << n << '\r' << '\n';
				out.flush();
		
				buffers.push_back(boost::asio::buffer(&m_buffer[kMaxChunkSize], m_buffer.size() - kMaxChunkSize));
				buffers.push_back(boost::asio::buffer(&m_buffer[0], n));
				buffers.push_back(boost::asio::buffer(kCRLF));
			}
		}
		else
		{
			if (n > 0)
				buffers.push_back(boost::asio::buffer(&m_buffer[0], n));
			else
			{
				delete m_data;
				m_data = nullptr;
				result = false;
			}
		}
	}

	return result;
}

string reply::get_as_text()
{
	// for best performance, we pre calculate memory requirements and reserve that first
	m_status_line = (boost::format("HTTP/%1%.%2% %3% %4%\r\n")
		% m_version_major % m_version_minor % m_status % get_status_text(m_status)).str();

	string result;
	result.reserve(get_size());
	
	result = m_status_line;
	foreach (header& h, m_headers)
	{
		result += h.name;
		result += ": ";
		result += h.value;
		result += "\r\n";
	}

	result += "\r\n";
	result += m_content;
	
	return result;
}

size_t reply::get_size() const
{
	size_t size = m_status_line.length();
	foreach (const header& h, m_headers)
		size += h.name.length() + 2 + h.value.length() + 2;
	size += 2 + m_content.length();
	
	return size;
}

reply reply::stock_reply(status_type status)
{
	reply result;

	if (status != not_modified)
	{
		stringstream text;
		text << "<html><body><h1>" << get_status_text(status) << "</h1></body></html>";
		result.set_content(text.str(), "text/html; charset=utf-8");
	}

	result.m_status = status;
	
	return result;
}

reply reply::redirect(const std::string& location)
{
	reply result;

	result.m_status = moved_temporarily;

	string text = get_status_text(moved_temporarily);
	result.m_content =
		string("<html><head><title>") + text + "</title></head><body><h1>" +
 		boost::lexical_cast<string>(moved_temporarily) + ' ' + text + "</h1></body></html>";
	
	result.set_header("Location", location);
	result.set_header("Content-Length", boost::lexical_cast<string>(result.m_content.length()));
	result.set_header("Content-Type", "text/html; charset=utf-8");
	
	return result;
}

void reply::debug(ostream& os) const
{
	os << (boost::format("HTTP/%1%.%2% %3% %4%")
		% m_version_major % m_version_minor % m_status % get_status_text(m_status)) << endl;
	foreach (const header& h, m_headers)
		os << h.name << ": " << h.value << endl;
}

ostream& operator<<(ostream& lhs, reply& rhs)
{
	vector<boost::asio::const_buffer> buffers;

	rhs.to_buffers(buffers);

	foreach(auto& b, buffers)
		lhs.write(boost::asio::buffer_cast<const char*>(b), boost::asio::buffer_size(b));

	return lhs;
}

}
}
