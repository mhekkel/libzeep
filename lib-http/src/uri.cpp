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

// --------------------------------------------------------------------

enum URICharClass
{
	alpha		= 1 << 0,
	digit		= 1 << 1,
	gen_delim	= 1 << 2,
	sub_delim	= 1 << 3,
	reserved	= 1 << 4,
	pchar		= 1 << 5,
	unreserved	= 1 << 6
};

/*

constexpr bool is_alpha(int ch)
{
	return (ch >= 'A' and ch <= 'Z') or (ch >= 'a' and ch <= 'z');
}

constexpr bool is_gen_delim(int ch)
{
	return ch == ':' or ch == '/' or ch == '?' or ch == '#' or ch == '[' or ch == ']' or ch == '@';
}

constexpr bool is_sub_delim(int ch)
{
	return ch == '!' or ch == '$' or ch == '&' or ch == '\'' or ch == '(' or ch == ')' or ch == '*' or ch == '+' or ch == ',' or ch == ';' or ch == '=';
}

constexpr bool is_reserved(int ch)
{
	return is_gen_delim(ch) or is_sub_delim(ch);
}

constexpr bool is_unreserved(int ch)
{
	return std::isalnum(ch) or ch == '-' or ch == '.' or ch == '_' or ch == '~';
}

constexpr bool is_pchar(int ch)
{
	return is_unreserved(ch) or is_sub_delim(ch) or ch == ':' or ch == '@';
}

enum URICharClass
{
	alpha		= 1 << 0,
	digit		= 1 << 1,
	gen_delim	= 1 << 2,
	sub_delim	= 1 << 3,
	reserved	= 1 << 4,
	pchar		= 1 << 5,
	unreserved	= 1 << 6
};

int main()
{
	for (int i = 32; i < 128; ++i)
	{
		int f = 0;
		if (is_alpha(i))				f |= URICharClass::alpha;
		if (i >= '0' and i <= '9')		f |= URICharClass::digit;
		if (is_gen_delim(i))			f |= URICharClass::gen_delim;
		if (is_sub_delim(i))			f |= URICharClass::sub_delim;
		if (is_reserved(i))				f |= URICharClass::reserved;
		if (is_pchar(i))				f |= URICharClass::pchar;
		if (is_unreserved(i))			f |= URICharClass::unreserved;

		std::cout << std::dec << i << '\t'
			<< (std::isprint(i) and not std::isspace(i) ? char(i) : ' ') << '\t'
			<< std::hex << f << std::endl;
	}

	return 0;
}
*/

const uint8_t kURICharClass[] = {
	 0,56, 0,20,56, 0,56,56,56,56,56,56,56,96,96,20,        /*  !"#$%&'()*+,-./*/
	98,98,98,98,98,98,98,98,98,98,52,56, 0,56, 0,20,        /* 0123456789:;<=>?*/
	52,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,        /* @ABCDEFGHIJKLMNO*/
	97,97,97,97,97,97,97,97,97,97,97,20, 0,20, 0,96,        /* PQRSTUVWXYZ[\]^_*/
	 0,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,        /* `abcdefghijklmno*/
	97,97,97,97,97,97,97,97,97,97,97, 0, 0, 0,96, 0,        /* pqrstuvwxyz{|}~ */
};

constexpr bool is_alpha(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & URICharClass::alpha) != 0;
}

constexpr bool is_digit(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & URICharClass::digit) != 0;
}

constexpr bool is_alnum(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & (URICharClass::digit | URICharClass::alpha)) != 0;
}

constexpr bool is_hex(int ch)
{
	return (ch >= '0' and ch <= '9') or (ch >= 'a' and ch <= 'f') or (ch >= 'A' and ch <= 'F');
}

constexpr bool is_scheme(int ch)
{
	return is_alnum(ch) or ch == '+' or ch == '-' or ch == '.';
}

constexpr bool is_gen_delim(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & URICharClass::gen_delim) != 0;
}

constexpr bool is_sub_delim(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & URICharClass::sub_delim) != 0;
}

constexpr bool is_reserved(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & URICharClass::reserved) != 0;
}

constexpr bool is_unreserved(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & URICharClass::unreserved) != 0;
}

constexpr bool is_pchar(int ch)
{
	return ch >= 32 and ch < 128 and (kURICharClass[ch - 32] & URICharClass::pchar) != 0;
}

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
							"(?:(?:" h16 ":){4}"	h16 ")?"	"::"					ls32	"|" \
							"(?:(?:" h16 ":){5}"	h16 ")?"	"::"					h16		"|" \
							"(?:(?:" h16 ":){6}"	h16 ")?"	"::"							"|" \
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

#define URI				"^(" SCHEME "):(?:" HIER_PART ")(?:\\?(" QUERY "))?(?:#(" FRAGMENT "))?$"

const std::regex kIPv6Rx(IPv6_ADDRESS);

class url_parser
{
  public:
	url_parser(const std::string& s)
		: m_s(s), m_i(m_s.begin())
	{
		parse();
	}

	const int kEOF = -1;

	int get_next_char()
	{
		int result = m_i != m_s.end() ? static_cast<unsigned char>(*m_i++) : kEOF;

		m_token += char(result);

		bool escaped = result == '%';

		if (result == '%')
		{
			if (not (is_hex(*m_i) and is_hex(*std::next(m_i))))
				throw uri_parse_error("invalid escaped character");

			m_token += *m_i++;
			m_token += *m_i++;
		}

		m_escaped.push_back(escaped);

		return result;
	}

	int decode_next_char()
	{
		assert(*m_i == '%');
		if (*m_i++ != '%')
			throw std::logic_error("url parser internal error");

		int e[2] = {
			get_next_char(), get_next_char()
		};

		int result = 0;

		if (e[0] >= '0' and e[0] <= '9')
			result = (e[0] - '0') << 8;
		else if (e[0] >= 'a' and e[0] <= 'f')
			result = (e[0] - 'a' + 10) << 8;
		else if (e[0] >= 'A' and e[0] <= 'F')
			result = (e[0] - 'A' + 10) << 8;

		if (e[1] >= '0' and e[1] <= '9')
			result |= e[1] - '0';
		else if (e[1] >= 'a' and e[1] <= 'f')
			result |= e[1] - 'a' + 10;
		else if (e[1] >= 'A' and e[1] <= 'F')
			result |= e[1] - 'A' + 10;

		return result;
	}

	void retract()
	{
		if (m_escaped.back())
		{
			m_i -= 3;
			m_token.pop_back();
			m_token.pop_back();
			m_token.pop_back();
		}
		else
		{
			--m_i;
			m_token.pop_back();
		}

		m_escaped.pop_back();
	}

	enum url_parser_state
	{
		scheme			= 100,
		hierpart		= 200,
		authority		= 400,

		userinfo		= 410,
		host			= 420,
		ip_literal		= 430,
		ipv4			= 440,
		ipv6			= 470,
		ipvfuture		= 450,
		reg_name		= 460,

		port			= 600,
		

		path_0			= 700,


		// path			= 300,
		// query			= 400,
		// fragment		= 500,
		// error			= 600
	};

	void clear_token()
	{
		m_token.clear();
		m_escaped.clear();
	}

	void restart()
	{
		m_i -= m_token.length();
		clear_token();

		switch (m_start)
		{
			case scheme:
				m_start = m_state = hierpart;
				m_scheme.clear();
				break;

			case hierpart:
				m_start = m_state = path_0;
				break;			

			case authority:
				m_start = m_state = host;
				break;

			default:
				throw uri_parse_error("Invalid state in restart");
		}
	}

	void parse()
	{
		m_state = m_start = scheme;
		clear_token();
		bool done = false;

		while (not done)
		{
			int ch = get_next_char();

			switch (m_state)
			{
				case scheme:
					if (is_alpha(ch))
					{
						m_scheme = { char(ch) };
						++m_state;
					}
					else
						restart();
					break;
				
				case scheme + 1:
					if (ch == ':')
					{
						m_state = hierpart;
						clear_token();
					}
					else if (not is_scheme(ch))
						restart();
					else
						m_scheme += char(ch);
					break;
				
				case hierpart:
					if (ch == '/')
						++m_state;
					else
						restart();
					break;

				case hierpart + 1:
					if (ch == '/')
					{
						m_state = m_start = authority;
						clear_token();
					}
					else
						restart();
					break;

				case authority:
					// if (ch == '/' or )


				case path_0:
					if (ch == kEOF)
						done = true;
					break;

				default:
					throw uri_parse_error("Invalid state in parse");

			}
		}
	}

	const std::string& m_s;

	std::string::const_iterator m_i;
	int m_state = scheme;
	int m_start = scheme;

	std::string m_scheme;
	std::string m_host;
	std::filesystem::path m_path;
	std::string m_port;
	std::string m_query;
	std::string m_fragment;

	std::string m_token;
	std::vector<bool> m_escaped;
};

// --------------------------------------------------------------------
// Is a valid url?

bool is_valid_uri(const std::string& url)
{
	bool result = false;

	try
	{
		uri p(url);
		result = true;
	}
	catch (const uri_parse_error& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
	

	return result;

	// enum {
	// 	scheme1, scheme2,
	// 	authority,
	// 	path,
	// 	query,
	// 	fragment,
	// 	error
	// } state = scheme1;

	// auto restart = [&state]()
	// {
	// 	if (state < authority)
	// 		return authority;
	// 	if (state < path)
	// 		return path;
	// 	if (state < query)
	// 		return query;
	// 	if (state < fragment)
	// 		return fragment;
	// 	return error;
	// };

	// bool result = true;

	// for (auto i = url.begin(); i != url.end(); ++i)
	// {
	// 	unsigned char ch = *i;

	// 	switch (state)
	// 	{
	// 		case scheme1:
	// 			if (std::isalpha(ch))
	// 				state = scheme2;
	// 			else
	// 				state = restart();
	// 			break;

	// 		case scheme2:
	// 			if (ch == ':')
	// 				state = authority;
	// 			else if (not (std::isalnum(ch) or ch == '+' or ch == '-' or ch == '.'))
	// 				state = restart();
	// 	}

	// }

	// // for (unsigned char ch: url)
	// // {
	// // 	if (ch < 32 or ch >= 128 or (kURLCharInfoTable[ch - 32] & 4) == 0)
	// // 	{
	// // 		result = false;
	// // 		break;
	// // 	}
	// // }

	// return result;
}






// --------------------------------------------------------------------

struct uri_impl
{
	std::string m_s;
	std::string m_scheme;
	std::string m_host;
	std::filesystem::path m_path;
	std::string m_query;
	std::string m_fragment;
	bool m_absolutePath = false;
};

// --------------------------------------------------------------------

uri::uri(const std::string &url)
	: m_impl(new uri_impl{url})
{
	// url_parser p(url);

	// m_impl->m_scheme = p.m_scheme;
	// m_impl->m_host = p.m_host;
	// m_impl->m_query = p.m_query;
	// m_impl->m_path = p.m_path;

	const std::regex rx(URI);

	std::smatch m;
	if (std::regex_match(url, m, rx))
	{
		m_impl->m_scheme = m[1];
		m_impl->m_host = m[3];
		if (m[5].matched)
			m_impl->m_path = m[5];
		else if (m[6].matched)
			m_impl->m_path = m[6];
		else if (m[7].matched)
			m_impl->m_path = m[7];
		
		m_impl->m_query = m[8];
		m_impl->m_fragment = m[9];
	}
	else
		throw uri_parse_error(url);


	// if (uriParseSingleUriA(&m_impl->m_uri, m_impl->m_s.c_str(), nullptr) != URI_SUCCESS)
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
	// int charsRequired;
	// if (uriToStringCharsRequiredA(&m_impl->m_uri, &charsRequired) != URI_SUCCESS)
	// {
	// 	throw zeep::exception("Failed to copy url to string");
	// }
	
	// std::string result(charsRequired, 0);
	// charsRequired += 1;

	// if (uriToStringA(result.data(), &m_impl->m_uri, charsRequired, NULL) != URI_SUCCESS)
	// {
	// 	throw zeep::exception("Failed to copy url to string");
	// }

	// return result;
	// return {};
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

// --------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const uri &url)
{
	os << url.string();
	return os;
}

// --------------------------------------------------------------------

// bool is_valid_uri(const std::string &s)
// {
// 	UriUriA uri;

// 	bool result = uriParseSingleUriA(&uri, s.c_str(), nullptr) == URI_SUCCESS;

// 	uriFreeUriMembersA(&uri);

// 	return result;
// }




// // --------------------------------------------------------------------

// url::url(const std::string& u)
// {
// 	std::regex rx1(URI);

// 	std::smatch m;

// 	if (not std::regex_match(u, m, rx1))
// 		throw uri_parse_error(u);

// 	std::cout << u << std::endl;

// 	for (auto mi: m)
// 		std::cout << mi << std::endl;

// 	// std::regex rx(URI);

// }

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
