//            Copyright Maarten L. Hekkelman, 2021
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <iostream>
#include <regex>
#include <sstream>

#include <zeep/http/uri.hpp>

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

const std::regex kURIRx(URI);

// --------------------------------------------------------------------
/// \brief Is \a url a valid url?

bool is_valid_uri(const std::string& url)
{
	return std::regex_match(url, kURIRx);
}

bool is_fully_qualified_uri(const std::string& uri)
{
	std::smatch m;
	return std::regex_match(uri, m, kURIRx) and m[1].matched and m[3].matched;
}

// --------------------------------------------------------------------

struct uri_impl
{
	uri_impl(const std::string& uri)
		: m_s(uri) {}

	std::string m_s;
	std::string m_scheme;
	std::string m_host;
	std::filesystem::path m_path;
	std::string m_query;
	std::string m_fragment;
	bool m_absolutePath = false;

	void set_path(const std::filesystem::path &p)
	{
		m_path = p;
		m_absolutePath = p.is_absolute();

		std::ostringstream os;
		os << m_scheme << "://" << m_host;
		if (not m_absolutePath)
			os << '/';
		os << m_path.string();
		m_s = os.str();
	}
};

// --------------------------------------------------------------------

uri::uri(const std::string &url)
	: m_impl(new uri_impl{url})
{
	const std::regex rx(URI);

	std::smatch m;
	if (std::regex_match(url, m, rx))
	{
		m_impl->m_scheme = m[1];
		m_impl->m_host = m[3];

		if (not m_impl->m_host.empty() and m_impl->m_host.front() == '[' and m_impl->m_host.back() == ']')
			m_impl->m_host = m_impl->m_host.substr(1, m_impl->m_host.length() - 2);

		if (m[5].matched)
			m_impl->m_path = m[5].str();
		else if (m[6].matched)
			m_impl->m_path = m[6].str();
		else if (m[7].matched)
			m_impl->m_path = m[7].str();
		
		m_impl->m_query = m[8];
		m_impl->m_fragment = m[9];

		m_impl->m_absolutePath = m_impl->m_host.empty() and m[6].matched;
	}
	else
		throw uri_parse_error(url);
}

uri::uri(const uri &u)
	: uri(u.string())
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
	return m_impl->m_s.empty();
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
	return m_impl->m_s;
}

std::string uri::get_scheme() const
{
	return m_impl->m_scheme;
}

std::string uri::get_host() const
{
	return m_impl->m_host;
}

std::filesystem::path uri::get_path() const
{
	std::filesystem::path result;

	for (auto p: m_impl->m_path)
		result /= decode_url(p.string());
	
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
	m_impl->set_path(p);
}

uri operator/(uri uri, const std::filesystem::path &rhs)
{
	uri.set_path(uri.get_path() / rhs);
	return uri;
}

// --------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const uri &url)
{
	os << url.string();
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

const unsigned char kURLAcceptable[96] =
{/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	0,0,0,0,0,0,0,0,0,0,7,6,0,7,7,4,		/* 2x   !"#$%&'()*+,-./	 */
	7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,		/* 3x  0123456789:;<=>?	 */
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 4x  @ABCDEFGHIJKLMNO  */
	7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,		/* 5X  PQRSTUVWXYZ[\]^_	 */
	0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 6x  `abcdefghijklmno	 */
	7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0			/* 7X  pqrstuvwxyz{\}~	DEL */
};

std::string encode_url(std::string_view s)
{
	const char kHex[] = "0123456789abcdef";

	std::string result;
	
	for (auto c = s.begin(); c != s.end(); ++c)
	{
		unsigned char a = (unsigned char)*c;
		if (not (a >= 32 and a < 128 and (kURLAcceptable[a - 32] & 4)))
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

} // namespace zeep::http
