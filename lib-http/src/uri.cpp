//            Copyright Maarten L. Hekkelman, 2021
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <iostream>
#include <regex>
#include <sstream>
#include <utility>

#include <zeep/http/uri.hpp>
#include <zeep/unicode-support.hpp>

namespace zeep::http
{

namespace
{
const char kHex[] = "0123456789ABCDEF";
}

// ah, the beauty of regular expressions!
// ....
// Unfortunately, the implementation of many regular expression
// libraries is sub-optimal. And thus we don't use this magic
// anymore, apart from matching the IP_LITERAL part for a host.

#define GEN_DELIMS		R"([][]:/?#@])"
#define SUB_DELIMS		R"([!$&'()*+,;=])"
#define RESERVED		GEN_DELIMS | SUB_DELIMS
#define UNRESERVED		R"([-._~A-Za-z0-9])"
#define SCHEME			R"([a-zA-Z][-+.a-zA-Z0-9]*)"
#define PCT_ENCODED		"%[[:xdigit:]]{2}"
#define USERINFO		"(?:" UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS "|" ":" ")*"
#define REG_NAME		"(?:" UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS ")*"
#define PORT			"[[:digit:]]*"

#define DEC_OCTET		"(?:[0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])"
#define IPv4_ADDRESS	DEC_OCTET R"(\.)" DEC_OCTET R"(\.)" DEC_OCTET R"(\.)" DEC_OCTET
#define h16				"[[:xdigit:]]{1,4}"
#define ls32			"(?:" h16 ":" h16 ")|" IPv4_ADDRESS
#define IPv6_ADDRESS	"(?:"	\
																		"(?:" h16 ":){6}"	ls32	"|" \
																"::"	"(?:" h16 ":){5}"	ls32	"|"	\
							"(?:"					h16 ")?"	"::"	"(?:" h16 ":){4}"	ls32	"|" \
							"(?:(?:" h16 ":){1}"	h16 ")?"	"::"	"(?:" h16 ":){3}"	ls32	"|" \
							"(?:(?:" h16 ":){2}"	h16 ")?"	"::"	"(?:" h16 ":){2}"	ls32	"|" \
							"(?:(?:" h16 ":){3}"	h16 ")?"	"::"	"(?:" h16 ":){1}"	ls32	"|" \
							"(?:(?:" h16 ":){4}"	h16 ")?"	"::"						ls32	"|" \
							"(?:(?:" h16 ":){5}"	h16 ")?"	"::"						h16		"|" \
							"(?:(?:" h16 ":){6}"	h16 ")?"	"::"								"|" \
						")"
#define IPvFUTURE		R"(v[[:xdigit:]]\.(?:)" UNRESERVED "|" SUB_DELIMS "|" ":" ")+"
#define IP_LITERAL		R"(\[(?:)" IPv6_ADDRESS "|" IPvFUTURE R"()\])"
#define HOST			IP_LITERAL "|" IPv4_ADDRESS "|" REG_NAME

#define AUTHORITY		"(" USERINFO "\\@" ")?(" HOST ")(:" PORT ")?"

#define PCHAR			UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS "|" ":" "|" "@"
#define	SEGMENT			"(?:" PCHAR ")*"
#define SEGMENT_NZ		"(?:" PCHAR "){1,}"
#define SEGMENT_NZ_NC	"(?:" UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS "){1,}"

#define PATH_ABEMPTY	"(?:" "/" "(" SEGMENT "(?:/" SEGMENT ")*" "))?"
#define PATH_ABSOLUTE	"/" "(?:" SEGMENT_NZ "(?:" "/" SEGMENT ")*" ")?"
#define PATH_ROOTLESS	SEGMENT_NZ "(?:" "/" SEGMENT ")*"
#define PATH_EMPTY		""

#define HIER_PART		"//" AUTHORITY PATH_ABEMPTY "|" \
						"(" PATH_ABSOLUTE ")|" \
						"(" PATH_ROOTLESS ")|" \
						PATH_EMPTY

#define QUERY			"(?:\\?|/|" PCHAR ")*"
#define FRAGMENT		"(?:\\?|/|" PCHAR ")*"

#define URI				"^(?:(" SCHEME "):)?(?:" HIER_PART ")(?:\\?(" QUERY "))?(?:#(" FRAGMENT "))?$"

// --------------------------------------------------------------------

// const std::regex kURIRx(URI);

// --------------------------------------------------------------------

uri::uri(const std::string &url)
{
	parse(url.c_str());
	remove_dot_segments();
}

uri::uri(const char *url)
{
	parse(url);
	remove_dot_segments();
}

uri::uri(const std::string &url, const uri &base)
{
	parse(url.c_str());
	transform(base);
	remove_dot_segments();
}

void uri::swap(uri &u)
{
	std::swap(m_scheme, u.m_scheme);
	std::swap(m_userinfo, u.m_userinfo);
	std::swap(m_host, u.m_host);
	std::swap(m_port, u.m_port);
	std::swap(m_path, u.m_path);
	std::swap(m_query, u.m_query);
	std::swap(m_fragment, u.m_fragment);
	std::swap(m_absolutePath, u.m_absolutePath);
}

std::string uri::string() const
{
	std::ostringstream os;
	write(os, true);
	return os.str();
}

std::string uri::unencoded_string() const
{
	std::ostringstream os;
	write(os, false);
	return os.str();
}

uri uri::get_path() const
{
	uri result;
	result.m_absolutePath = m_absolutePath;
	result.m_path = m_path;
	return result;
}

void uri::set_path(const std::string &path)
{
	m_path.clear();
	m_absolutePath = false;

	auto cp = path.c_str();

	if (*cp == '/')
	{
		m_absolutePath = true;
		++cp;
	}

	cp = parse_segment(cp);

	while (*cp == '/')
	{
		++cp;
		cp = parse_segment(cp);
	}

	remove_dot_segments();
}

void uri::set_query(const std::string &query, bool encode)
{
	if (encode)
	{
		std::ostringstream os;

		for (auto c : query)
		{
			if (is_unreserved(c) or is_sub_delim(c) or c == ':' or c == '@' or c == '/' or c == '?')
				os << c;
			else
				os << '%' << kHex[c >> 4] << kHex[c & 15];
		}

		m_query = os.str();
	}
	else
		m_query = query;
}

void uri::set_fragment(const std::string &fragment, bool encode)
{
	if (encode)
	{
		std::ostringstream os;

		for (auto c : fragment)
		{
			if (is_unreserved(c) or is_sub_delim(c) or c == ':' or c == '@' or c == '/' or c == '?')
				os << c;
			else
				os << '%' << kHex[c >> 4] << kHex[c & 15];
		}

		m_fragment = os.str();
	}
	else
		m_fragment = fragment;
}

uri &uri::operator/=(const uri &rhs)
{
	if (m_path.empty())
	{
		m_absolutePath = rhs.m_absolutePath;
		m_path = rhs.m_path;
	}
	else
	{
		if (m_path.back().empty())
			m_path.pop_back();
		
		m_path.insert(m_path.end(), rhs.m_path.begin(), rhs.m_path.end());
	}

	remove_dot_segments();

	return *this;
}

// --------------------------------------------------------------------

const char *uri::parse_scheme(const char *cp)
{
	auto b = cp;

	if (is_scheme_start(*cp))
	{
		do
			++cp;
		while (is_scheme(*cp));

		if (*cp == ':')
		{
			m_scheme.assign(b, cp);
			to_lower(m_scheme);
			++cp;
		}
		else
			cp = b;
	}

	return cp;
}

const char *uri::parse_hierpart(const char *cp)
{
	if (*cp == '/')
	{
		++cp;
		if (*cp == '/')
		{
			++cp;

			cp = parse_authority(cp);

			if (*cp == '/' and cp[1] == '/')
				m_absolutePath = true;

			while (*cp == '/')
			{
				++cp;
				cp = parse_segment(cp);
			}
		}
		else
		{
			m_absolutePath = true;

			cp = parse_segment(cp);

			while (*cp == '/')
			{
				++cp;
				cp = parse_segment(cp);
			}
		}
	}
	else if (*cp != '?' and *cp != '#' and *cp != 0)
	{
		cp = parse_segment_nz(cp);
		while (*cp == '/')
		{
			++cp;
			cp = parse_segment(cp);
		}
	}

	return cp;
}

const char *uri::parse_authority(const char *cp)
{
	auto b = cp;

	while (is_userinfo(cp))
		++cp;

	if (*cp == '@')
	{
		m_userinfo.assign(b, cp);

		++cp;
		b = cp;

		cp = parse_host(cp);
	}
	else
		cp = parse_host(b);

	if (*cp == ':')
	{
		++cp;
		while (*cp >= '0' and *cp <= '9')
			m_port = 10 * m_port + *cp++ - '0';
	}

	return cp;
}

const char *uri::parse_host(const char *cp)
{
	auto b = cp;

	if (*cp == '[')
	{
		++cp;
		do
			++cp;
		while (*cp != 0 and *cp != ']');

		if (*cp != ']')
			throw uri_parse_error();

		++cp;

		static std::regex rx(IP_LITERAL);
		if (not std::regex_match(b, cp, rx))
			throw uri_parse_error();
	}
	else
	{
		while (is_reg_name(cp))
			++cp;
	}

	m_host.assign(b, cp);
	to_lower(m_host);

	if (m_host.empty())
		throw uri_parse_error();

	return cp;
}

const char *uri::parse_segment(const char *cp)
{
	auto b = cp;

	while (is_pchar(cp))
		++cp;

	m_path.emplace_back(decode_url({b, static_cast<std::string::size_type>(cp - b)}));

	return cp;
}

const char *uri::parse_segment_nz(const char *cp)
{
	cp = parse_segment(cp);

	if (m_path.back().empty())
		throw uri_parse_error();

	return cp;
}

const char *uri::parse_segment_nz_nc(const char *cp)
{
	auto b = cp;

	if (not (is_unreserved(*cp) or is_pct_encoded(cp) or is_sub_delim(*cp)))
		throw uri_parse_error();

	while (is_unreserved(*cp) or is_pct_encoded(cp) or is_sub_delim(*cp))
		++cp;

	m_path.emplace_back(decode_url({b, static_cast<std::string::size_type>(cp - b)}));

	return cp;
}

void uri::parse(const char *s)
{
	m_scheme.clear();
	m_userinfo.clear();
	m_host.clear();
	m_port = 0;
	m_path.clear();
	m_query.clear();
	m_fragment.clear();
	m_absolutePath = false;

	auto b = s;

	auto cp = parse_scheme(b);
	cp = parse_hierpart(cp);
	
	if (*cp == '?')
	{
		++cp;
		auto b = cp;

		while (*cp == '?' or *cp == '/' or is_pchar(cp))
			++cp;
		
		m_query.assign(b, cp);
	}

	if (*cp == '#')
	{
		++cp;
		auto b = cp;

		while (*cp == '?' or *cp == '/' or is_pchar(cp))
			++cp;
		
		m_fragment.assign(b, cp);
	}

	// if (*cp != 0 or static_cast<std::string::size_type>(cp - b) != s.length())
	if (*cp != 0)
		throw uri_parse_error();
}

void uri::remove_dot_segments()
{
	std::vector<std::string> out;

	auto in = m_path.begin();

	while (in != m_path.end())
	{
		if (*in == ".")
		{
			++in;
			
			if (in == m_path.end())
			{
				out.push_back({});
				break;
			}

			continue;
		}

		if (*in == "..")
		{
			if (not out.empty())
				out.pop_back();

			++in;

			if (in == m_path.end())
			{
				out.push_back({});
				break;
			}

			continue;
		}

		out.push_back(*in);
		++in;
		continue;
	}

	std::swap(m_path, out);
}

void uri::transform(const uri &base)
{
	if (m_scheme.empty())
	{
		m_scheme = base.m_scheme;
		
		if (not has_authority())
		{
			if (m_path.empty())
			{
				m_absolutePath = base.m_absolutePath;
				m_path = base.m_path;
				if (m_query.empty())
					m_query = base.m_query;
			}
			else if (not m_absolutePath)
			{
				if (base.m_path.size() > 1)
					m_path.insert(m_path.begin(), base.m_path.begin(), base.m_path.end() - 1);
				remove_dot_segments();
			}
			m_userinfo = base.m_userinfo;
			m_host = base.m_host;
			m_port = base.m_port;
		}
	}
}

// --------------------------------------------------------------------

void uri::write(std::ostream &os, bool encoded) const
{
	// --------------------------------------------------------------------

	if (not m_scheme.empty())
		os << m_scheme << ':';

	bool write_slash = m_absolutePath;

	if (has_authority())
	{
		os << "//";

		if (not m_userinfo.empty())
			os << m_userinfo << '@';
		
		os << m_host;
		if (m_port != 0)
			os << ':' << m_port;
		
		write_slash = true;
	}
	
	for (auto segment : m_path)
	{
		if (write_slash)
			os << '/';
		write_slash = true;

		for (auto c : segment)
		{
			if (not encoded or is_unreserved(c) or is_sub_delim(c) or c == ':' or c == '@')
				os << c;
			else
				os << '%' << kHex[c >> 4] << kHex[c & 15];
		}
	}

	if (not m_query.empty())
		os << '?' << m_query;
	
	if (not m_fragment.empty())
		os << '#' << m_fragment;
}

// --------------------------------------------------------------------
// decode_url function

std::string decode_url(std::string_view s)
{
	std::string result;
	
	for (auto c = s.begin(); c != s.end(); ++c)
	{
		if (*c == '%')
		{
			if (s.end() - c >= 3)
			{
				int value;
				std::string s2(c + 1, c + 3);
				std::istringstream is(s2);
				if (is >> std::hex >> value)
				{
					result += static_cast<char>(value);
					c += 2;
				}
			}
		}
		else if (*c == '+')
			result += ' ';
		else
			result += *c;
	}
	return result;
}

// --------------------------------------------------------------------
// encode_url function

std::string encode_url(std::string_view s)
{
	std::string result;
	
	for (auto c = s.begin(); c != s.end(); ++c)
	{
		unsigned char a = (unsigned char)*c;
		if (not uri::is_unreserved(a))
		{
			result += '%';
			result += kHex[a >> 4];
			result += kHex[a & 15];
		}
		else
			result += *c;
	}

	return result;
}

// --------------------------------------------------------------------

bool is_valid_uri(const std::string& s)
{
	bool result = true;
	try
	{
		uri u(s);
	}
	catch (...)
	{
		result = false;
	}
	return result;
}

bool is_fully_qualified_uri(const std::string& s)
{
	bool result = true;
	try
	{
		uri u(s);
		result = not (u.get_scheme().empty() or u.get_path().empty());
	}
	catch (...)
	{
		result = false;
	}
	return result;
}

} // namespace zeep::http
