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

// ah, the beauty of regular expressions!

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

// Let's try a parser, since regular expressions can crash

constexpr bool is_gen_delim(char ch)
{
	return ch == '[' or ch == ']' or ch == ':' or ch == '/' or ch == '?' or ch == '#' or ch == '@';
}

constexpr bool is_sub_delim(char ch)
{
	return ch == '!' or ch == '$' or ch == '&' or ch == '\'' or ch == '(' or ch == ')' or ch == '*' or ch == '+' or ch == ',' or ch == ';' or ch == '=';		
}

constexpr bool is_reserved(char ch)
{
	return is_gen_delim(ch) or is_sub_delim(ch);
}

constexpr bool is_unreserved(char ch)
{
	return ch == '-' or ch == '.' or ch == '_' or ch == '~' or
		(ch >= 'A' and ch <= 'Z') or (ch >= 'a' and ch <= 'z') or (ch >= '0' and ch <= '9');
}

constexpr bool is_scheme_start(char ch)
{
	return (ch >= 'A' and ch <= 'Z') or (ch >= 'a' and ch <= 'z');
}

constexpr bool is_scheme(char ch)
{
	return is_scheme_start(ch) or ch == '-' or ch == '+' or ch == '.' or (ch >= 0 and ch <= '9');
}

constexpr bool is_xdigit(char ch)
{
	return (ch >= '0' and ch <= '9') or
		(ch >= 'a' and ch <= 'f') or
		(ch >= 'A' and ch <= 'F');
}

struct uri_impl
{
	uri_impl() = default;
	uri_impl(const uri_impl &) = default;

	uri_impl(const std::string& s)
	{
		parse(s);
	}

	void transform(const uri_impl &base);

	bool is_pct_encoded(const char *&cp)
	{
		bool result = false;
		if (*cp == '%' and is_xdigit(cp[1]) and is_xdigit(cp[2]))
		{
			result = true;
			cp += 2;
		}
		return result;
	}

	bool is_userinfo(const char *&cp)
	{
		return is_unreserved(*cp) or is_sub_delim(*cp) or *cp == ':' or is_pct_encoded(cp);
	}

	bool is_reg_name(const char *&cp)
	{
		return is_unreserved(*cp) or is_sub_delim(*cp) or is_pct_encoded(cp);
	}

	bool is_pchar(const char *&cp)
	{
		return is_unreserved(*cp) or is_sub_delim(*cp) or *cp == ':' or *cp == '@' or is_pct_encoded(cp);
	}

	void parse(const std::string &s);

	const char *parse_scheme(const char *ch);
	const char *parse_hierpart(const char *ch);
	const char *parse_authority(const char *ch);
	const char *parse_segment(const char *ch);
	const char *parse_segment_nz(const char *ch);
	const char *parse_segment_nz_nc(const char *ch);

	const char *parse_userinfo(const char *ch);
	const char *parse_host(const char *ch);
	const char *parse_port(const char *ch);

	bool empty() const
	{
		return m_scheme.empty() and m_userinfo.empty() and m_host.empty() and m_path.empty() and m_query.empty() and m_fragment.empty();
	}

	void remove_dot_segments();

	void write(std::ostream &os)
	{
		if (not m_scheme.empty())
			os << m_scheme << ':';

		bool write_slash = m_absolutePath;

		if (not (m_userinfo.empty() and m_host.empty()))
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
			os << encode_url(segment);
		}

		if (not m_query.empty())
			os << '?' << encode_url(m_query);
		
		if (not m_fragment.empty())
			os << '#' << encode_url(m_fragment);
	}

	std::string m_scheme;
	std::string m_userinfo;
	std::string m_host;
	uint16_t m_port;
	std::vector<std::string> m_path;
	std::string m_query;
	std::string m_fragment;
	bool m_absolutePath = false;
};

const char *uri_impl::parse_scheme(const char *cp)
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

const char *uri_impl::parse_hierpart(const char *cp)
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

			cp = parse_segment_nz(cp);

			while (*cp == '/')
			{
				++cp;
				cp = parse_segment(cp);
			}
		}
	}
	else if (*cp != '?' and *cp != '#')
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

const char *uri_impl::parse_authority(const char *cp)
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

const char *uri_impl::parse_host(const char *cp)
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

const char *uri_impl::parse_segment(const char *cp)
{
	auto b = cp;

	while (is_pchar(cp))
		++cp;

	m_path.emplace_back(decode_url({b, cp - b}));

	return cp;
}

const char *uri_impl::parse_segment_nz(const char *cp)
{
	cp = parse_segment(cp);

	if (m_path.back().empty())
		throw uri_parse_error();

	return cp;
}

const char *uri_impl::parse_segment_nz_nc(const char *cp)
{
	auto b = cp;

	if (not (is_unreserved(*cp) or is_pct_encoded(cp) or is_sub_delim(*cp)))
		throw uri_parse_error();

	while (is_unreserved(*cp) or is_pct_encoded(cp) or is_sub_delim(*cp))
		++cp;

	m_path.emplace_back(decode_url({b, cp - b}));

	return cp;
}

void uri_impl::parse(const std::string &s)
{
	m_scheme.clear();
	m_userinfo.clear();
	m_host.clear();
	m_port = 0;
	m_path.clear();
	m_query.clear();
	m_fragment.clear();
	m_absolutePath = false;

	auto cp = parse_scheme(s.c_str());
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

	remove_dot_segments();

	if (*cp != 0 or (cp - s.c_str()) != s.length())
		throw uri_parse_error();
}

void uri_impl::remove_dot_segments()
{
	std::vector<std::string> out;

	auto in = m_path.begin();
	while (in != m_path.end())
	{
		if (*in == ".")
		{
			++in;
			continue;
		}

		if (*in == "..")
		{
			if (not out.empty())
				out.pop_back();
			++in;
			continue;
		}

		out.push_back(*in);
		++in;
		continue;
	}

	std::swap(m_path, out);
}

void uri_impl::transform(const uri_impl &base)
{
	if (m_scheme.empty())
	{
		m_scheme = base.m_scheme;
		
		if (m_host.empty() and m_userinfo.empty())
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
				m_absolutePath = true;
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

uri::uri()
	: m_impl(new uri_impl())
{
}

uri::uri(const std::string &url)
	: m_impl(new uri_impl{url})
{
}

uri::uri(const std::string &s, const uri &base)
	: m_impl(new uri_impl{s})
{
	m_impl->transform(*base.m_impl);
}

uri::uri(const uri &u)
	: m_impl(new uri_impl(*u.m_impl))
{
}

uri& uri::operator=(const uri &u)
{
	if (&u != this)
	{
		uri tmp(u);
		swap(tmp);
	}

	return *this;
}

uri::uri(uri &&u)
	: m_impl(std::exchange(u.m_impl, nullptr))
{
}

uri& uri::operator=(uri &&u)
{
	if (this != &u)
		m_impl = std::exchange(u.m_impl, nullptr);

	return *this;
}

uri::~uri()
{
	delete m_impl;
}

bool uri::empty() const
{
	return m_impl->empty();
}

bool uri::is_absolute() const
{
	return m_impl->m_absolutePath;
}

void uri::swap(uri &u) noexcept
{
	std::swap(m_impl, u.m_impl);
}

std::string uri::string() const
{
	std::ostringstream os;
	os << *this;
	return os.str();
}

std::string uri::get_scheme() const
{
	return m_impl->m_scheme;
}

std::string uri::get_host() const
{
	return m_impl->m_host;
}

uri uri::get_path() const
{
	uri result;
	result.m_impl->m_absolutePath = m_impl->m_absolutePath;
	result.m_impl->m_path = m_impl->m_path;
	return result;
}

std::string uri::get_query() const
{
	return m_impl->m_query;
}

std::string uri::get_fragment() const
{
	return m_impl->m_fragment;
}

void uri::set_path(const std::filesystem::path &p)
{
	// m_impl->set_path(p);

	m_impl->m_path.clear();
	for (auto pi : p)
		m_impl->m_path.push_back(pi.string());
	
	m_impl->remove_dot_segments();
}

// uri operator/(uri uri, const std::filesystem::path &rhs)
// {
// 	uri.set_path(uri.get_path() / rhs);
// 	return uri;
// }

// --------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const uri &url)
{
	url.m_impl->write(os);

	return os;
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
	const char kHex[] = "0123456789abcdef";

	std::string result;
	
	for (auto c = s.begin(); c != s.end(); ++c)
	{
		unsigned char a = (unsigned char)*c;
		if (not is_unreserved(a))
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
