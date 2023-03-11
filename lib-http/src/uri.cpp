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
		: m_s(uri)
	{
		parse();
	}

	// Let's try a parser, since regular expressions can crash

	constexpr bool is_gen_delim(char ch) const
	{
		return ch == '[' or ch == ']' or ch == ':' or ch == '/' or ch == '?' or ch == '#' or ch == '@';
	}

	constexpr bool is_sub_delim(char ch) const
	{
		return ch == '!' or ch == '$' or ch == '&' or ch == '\'' or ch == '(' or ch == ')' or ch == '*' or ch == '+' or ch == ',' or ch == ';' or ch == '=';		
	}

	constexpr bool is_reserved(char ch) const
	{
		return is_gen_delim(ch) or is_sub_delim(ch);
	}

	constexpr bool is_unreserved(char ch) const
	{
		return ch == '-' or ch == '.' or ch == '_' or ch == '~' or
			(ch >= 'A' and ch <= 'Z') or (ch >= 'a' and ch <= 'z') or (ch >= '0' and ch <= '9');
	}

	constexpr bool is_scheme_start(char ch) const
	{
		return (ch >= 'A' and ch <= 'Z') or (ch >= 'a' and ch <= 'z');
	}

	constexpr bool is_scheme(char ch) const
	{
		return is_scheme_start(ch) or ch == '-' or ch == '+' or ch == '.' or (ch >= 0 and ch <= '9');
	}

	constexpr bool is_xdigit(char ch) const
	{
		return (ch >= '0' and ch <= '9') or
			(ch >= 'a' and ch <= 'f') or
			(ch >= 'A' and ch <= 'F');
	}

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

	void parse();

	const char *parse_scheme(const char *ch);
	const char *parse_hierpart(const char *ch);
	const char *parse_authority(const char *ch);
	const char *parse_segment(const char *ch);
	const char *parse_segment_nz(const char *ch);
	const char *parse_segment_nz_nc(const char *ch);

	const char *parse_userinfo(const char *ch);
	const char *parse_host(const char *ch);
	const char *parse_port(const char *ch);

	std::string m_s;

	std::string m_scheme;
	std::string m_userinfo;
	std::string m_host;
	uint16_t m_port;
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
			++cp;
		}
		else
			cp = b;
	}

	return cp;
}

const char *uri_impl::parse_hierpart(const char *cp)
{
	auto b = cp;

	if (*cp == '/')
	{
		++cp;
		if (*cp == '/')
		{
			++cp;

			cp = parse_authority(cp);

			b = cp;

			if (*cp == '/')
			{
				do
				{
					++cp;
					cp = parse_segment(cp);

				} while (*cp == '/');
			}
		}
		else
		{
			cp - parse_segment_nz(cp);
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
			throw zeep::exception("invalid uri");

		++cp;

		static std::regex rx(IP_LITERAL);
		if (not std::regex_match(b, cp, rx))
			throw zeep::exception("invalid uri");
	}
	else
	{
		while (is_reg_name(cp))
			++cp;
	}

	m_host.assign(b, cp);

	if (m_host.empty())
		throw zeep::exception("invalid uri");

	return cp;
}

const char *uri_impl::parse_segment(const char *cp)
{
	while (is_pchar(cp))
		++cp;
	return cp;
}

const char *uri_impl::parse_segment_nz(const char *cp)
{
	if (not is_pchar(cp))
		throw zeep::exception("invalid uri");
	while (is_pchar(cp))
		++cp;
	return cp;
}

const char *uri_impl::parse_segment_nz_nc(const char *cp)
{
	if (not (is_unreserved(*cp) or is_pct_encoded(cp) or is_sub_delim(*cp)))
		throw zeep::exception("invalid uri");
	while (is_unreserved(*cp) or is_pct_encoded(cp) or is_sub_delim(*cp))
		++cp;
	return cp;
}

void uri_impl::parse()
{
	m_scheme.clear();
	m_userinfo.clear();
	m_host.clear();
	m_port = 0;
	m_path.clear();
	m_query.clear();
	m_fragment.clear();
	m_absolutePath = false;

	auto cp = parse_scheme(m_s.c_str());
	cp = parse_hierpart(cp);
	
	if (*cp == '?')
	{
		++cp;
		while (*cp == '?' or *cp == '/' or is_pchar(cp))
			++cp;
	}

	if (*cp == '#')
	{
		++cp;
		while (*cp == '?' or *cp == '/' or is_pchar(cp))
			++cp;
	}

	if (*cp != 0 or (cp - m_s.c_str()) != m_s.length())
		throw zeep::exception("Invalid URI");
}

// --------------------------------------------------------------------

uri::uri(const std::string &url)
	: m_impl(new uri_impl{url})
{
	// const std::regex rx(URI);

	// std::smatch m;
	// if (std::regex_match(url, m, rx))
	// {
	// 	m_impl->m_scheme = m[1];
	// 	m_impl->m_host = m[3];

	// 	if (not m_impl->m_host.empty() and m_impl->m_host.front() == '[' and m_impl->m_host.back() == ']')
	// 		m_impl->m_host = m_impl->m_host.substr(1, m_impl->m_host.length() - 2);

	// 	if (m[5].matched)
	// 		m_impl->m_path = m[5].str();
	// 	else if (m[6].matched)
	// 		m_impl->m_path = m[6].str();
	// 	else if (m[7].matched)
	// 		m_impl->m_path = m[7].str();
		
	// 	m_impl->m_query = m[8];
	// 	m_impl->m_fragment = m[9];

	// 	m_impl->m_absolutePath = m_impl->m_host.empty() and m[6].matched;
	// }
	// else
	// 	throw uri_parse_error(url);
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
