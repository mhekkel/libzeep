//            Copyright Maarten L. Hekkelman, 2021
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <iostream>
#include <regex>

#include <zeep/http/uri.hpp>

extern "C"
{
#include <uriparser/Uri.h>
}

namespace zeep::http
{

// --------------------------------------------------------------------

struct uri_impl
{
	std::string m_s;
	UriUriA m_uri;

	~uri_impl()
	{
		uriFreeUriMembersA(&m_uri);
	}
};

// --------------------------------------------------------------------

uri::uri(const std::string &url)
	: m_impl(new uri_impl{url})
{
	if (uriParseSingleUriA(&m_impl->m_uri, m_impl->m_s.c_str(), nullptr) != URI_SUCCESS)
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

uri::~uri()
{
	delete m_impl;
}

void uri::swap(uri &u) noexcept
{
	std::swap(m_impl, u.m_impl);
}

std::string uri::string() const
{
	int charsRequired;
	if (uriToStringCharsRequiredA(&m_impl->m_uri, &charsRequired) != URI_SUCCESS)
	{
		throw zeep::exception("Failed to copy url to string");
	}
	
	std::string result(charsRequired, 0);
	charsRequired += 1;

	if (uriToStringA(result.data(), &m_impl->m_uri, charsRequired, NULL) != URI_SUCCESS)
	{
		throw zeep::exception("Failed to copy url to string");
	}

	return result;
}

std::string uri::get_scheme() const
{
	return { m_impl->m_uri.scheme.first, m_impl->m_uri.scheme.afterLast };
}

std::string uri::get_host() const
{
	return { m_impl->m_uri.hostText.first, m_impl->m_uri.hostText.afterLast };
}

std::filesystem::path uri::get_path() const
{
	std::filesystem::path result;

	for (auto p = m_impl->m_uri.pathHead; p != nullptr /*m_impl->m_uri.pathTail*/; p = p->next)
		result /= decode_url({ p->text.first, p->text.afterLast - p->text.first });
	
	return result;
}

std::string uri::get_query() const
{
	return { m_impl->m_uri.query.first, m_impl->m_uri.query.afterLast };
}

// --------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const uri &url)
{
	os << url.string();
	return os;
}

// --------------------------------------------------------------------

bool is_valid_uri(const std::string &s)
{
	UriUriA uri;

	bool result = uriParseSingleUriA(&uri, s.c_str(), nullptr) == URI_SUCCESS;

	uriFreeUriMembersA(&uri);

	return result;
}


// #define GEN_DELIMS		R"([][]:/?#@])"
// #define SUB_DELIMS		R"([!$&'()*+,;=])"
// #define RESERVED		GEN_DELIMS | SUB_DELIMS
// #define UNRESERVED		R"([-._~A-Za-z0-9])"
// #define SCHEME			R"([a-zA-Z][-+.a-zA-Z0-9]*)"

// #define PCT_ENCODED		"%[[:xdigit:]]{2}"

// #define USERINFO		"(" UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS "|" ":" ")*"

// #define HOST			IP_LITERAL "|" IPv4_ADDRESS "|" REG_NAME

// #define IP_LITERAL		R"(\[(?:)" IPv6_ADDRESS "|" IPvFUTURE R"()\])"
// #define IPvFUTURE		R"(v[[:xdigit:]]\.(?:)" UNRESERVED "|" SUB_DELIMS "|" ":" ")+"

// #define IPv6_ADDRESS	"(?:"	\
// 																	"(?:" h16 ":){6}"	ls32	"|" \
// 															"::"	"(?:" h16 ":){5}"	ls32	"|"	\
// 							"(?:"					h16 ")?""::"	"(?:" h16 ":){4}"	ls32	"|" \
// 							"(?:(" h16 ":){1}"	h16 ")?"	"::"	"(?:" h16 ":){3}"	ls32	"|" \
// 							"(?:(" h16 ":){2}"	h16 ")?"	"::"	"(?:" h16 ":){2}"	ls32	"|" \
// 							"(?:(" h16 ":){3}"	h16 ")?"	"::"	"(?:" h16 ":){1}"	ls32	"|" \
// 							"(?:(" h16 ":){4}"	h16 ")?"	"::"						ls32	"|" \
// 							"(?:(" h16 ":){5}"	h16 ")?"	"::"						h16		"|" \
// 							"(?:(" h16 ":){6}"	h16 ")?"	"::"								"|" \
// 						")"

// #define ls32			"(?:" h16 ":" h16 ")|" IPv4_ADDRESS

// #define IPv4_ADDRESS	DEC_OCTET R"(\.)" DEC_OCTET R"(\.)" DEC_OCTET R"(\.)" DEC_OCTET

// #define DEC_OCTET		"(?:[0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])"

// #define REG_NAME		"(" UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS ")*"

// #define h16				"[[:xdigit:]]{1,4}"

// #define PORT			"[[:digit:]]*"

// #define AUTHORITY		"(" USERINFO "@" ")?" HOST "(:" PORT ")?"

// #define PATH_ABEMPTY	"(" "/" SEGMENT ")*"
// #define PATH_ABSOLUTE	"/" "(" SEGMENT_NZ "(" "/" SEGMENT ")*" ")?"
// #define PATH_ROOTLESS	SEGMENT_NZ "(" "/" SEGMENT ")*"
// #define PATH_EMPTY		""

// #define	SEGMENT			"(" PCHAR ")*"
// #define SEGMENT_NZ		"(" PCHAR "){1,}"
// #define SEGMENT_NZ_NC	"(" UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS "){1,}"
// #define PCHAR			UNRESERVED "|" PCT_ENCODED "|" SUB_DELIMS "|" ":" "|" "@"

// #define HIER_PART		"//" AUTHORITY PATH_ABEMPTY "|" PATH_ABSOLUTE "|" PATH_ROOTLESS "|" PATH_EMPTY

// #define QUERY			R"((\?|/|PCHAR)*)"
// #define FRAGMENT		R"((\?|/|PCHAR)*)"

// #define URI				"(" SCHEME "):(" HIER_PART ")(\\?(" QUERY "))?(#(" FRAGMENT "))?"
// // #define URI				"(" HIER_PART ")"

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

// // --------------------------------------------------------------------

// const uint8_t kURLAcceptable[96] =
// {/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
// 	0,0,0,0,0,0,0,0,0,0,7,6,0,7,7,4,		/* 2x   !"#$%&'()*+,-./	 */
// 	7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,		/* 3x  0123456789:;<=>?	 */
// 	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 4x  @ABCDEFGHIJKLMNO  */
// 	7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,		/* 5X  PQRSTUVWXYZ[\]^_	 */
// 	0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 6x  `abcdefghijklmno	 */
// 	7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0			/* 7X  pqrstuvwxyz{\}~	DEL */
// };

// enum class url_parser_state
// {
// 	scheme			= 100,
// 	authority		= 200,
// 	path			= 300,
// 	query			= 400,
// 	fragment		= 500,
// 	error			= 600
// };

// class url_parser
// {
//   public:
// 	url_parser(const std::string& s)
// 		: m_s(s), m_i(m_s.begin())
// 	{
// 		parse();
// 	}

//   private:

// 	constexpr bool is_alpha(int ch) const
// 	{
// 		return (ch >= 'A' and ch <= 'Z') or (ch >= 'a' and ch <= 'z');
// 	}

// 	constexpr bool is_gen_delim(int ch) const
// 	{
// 		return ch == ':' or ch == '/' or ch == '?' or ch == '#' or ch == '[' or ch == ']' or ch == '@';
// 	}

// 	constexpr bool is_sub_delim(int ch) const
// 	{
// 		return ch == '!' or ch == '$' or ch == '&' or ch == '\'' or ch == '(' or ch == ')' or ch == '*' or ch == '+' or ch == ',' or ch == ';' or ch == '=';
// 	}

// 	constexpr bool is_reserved(int ch) const
// 	{
// 		return is_gen_delim(ch) or is_sub_delim(ch);
// 	}

// 	constexpr bool is_pchar(int ch) const
// 	{
// 		return is_unreserved(ch) or is_sub_delim(ch) or ch == ':' or ch == '@';
// 	}

// 	constexpr bool is_unreserved(int ch) const
// 	{
// 		return std::isalnum(ch) or ch == '-' or ch == '.' or ch == '_' or ch == '~';
// 	}

// 	int get_next_char()
// 	{
// 		return m_i != m_s.end() ? static_cast<unsigned char>(*m_i++) : -1;
// 	}

// 	int decode_next_char()
// 	{
// 		assert(*m_i == '%');
// 		if (*m_i++ != '%')
// 			throw std::logic_error("url parser internal error");

// 		int e[2] = {
// 			get_next_char(), get_next_char()
// 		};

// 		int result = 0;

// 		if (e[0] >= '0' and e[0] <= '9')
// 			result = (e[0] - '0') << 8;
// 		else if (e[0] >= 'a' and e[0] <= 'f')
// 			result = (e[0] - 'a' + 10) << 8;
// 		else if (e[0] >= 'A' and e[0] <= 'F')
// 			result = (e[0] - 'A' + 10) << 8;

// 		if (e[1] >= '0' and e[1] <= '9')
// 			result |= e[1] - '0';
// 		else if (e[1] >= 'a' and e[1] <= 'f')
// 			result |= e[1] - 'a' + 10;
// 		else if (e[1] >= 'A' and e[1] <= 'F')
// 			result |= e[1] - 'A' + 10;

// 		return result;
// 	}

// 	void parse()
// 	{

// 		parse_scheme();

// 	}

// 	void retract()
// 	{
// 		--m_i;
// 	}

// 	void restart()
// 	{
// 		m_i = m_s.begin();
// 		switch (m_start)
// 		{
// 			case url_parser_state::scheme:
// 				break;

// 		}
// 	}

// 	void parse_scheme()
// 	{
// 		int ch = get_next_char();
// 		if (not is_alpha(ch))
// 			throw uri_parse_error();

// 		m_scheme = { static_cast<char>(ch) };

// 		for (;;)
// 		{
// 			ch = get_next_char();
// 			if (not is_alpha(ch) or (ch >= '0' and ch <= '9') or ch == '+' or ch == '-' or ch == '.')
// 			{
// 				retract();
// 				break;
// 			}

// 			m_scheme += static_cast<char>(ch);
// 		}

// 		if (*m_i++ != ':')
// 			throw uri_parse_error();
// 	}

// 	const std::string& m_s;

// 	std::string::const_iterator m_i;
// 	url_parser_state m_state = url_parser_state::scheme;
// 	url_parser_state m_start = url_parser_state::scheme;

// 	std::string m_scheme;
// 	std::string m_authority;
// 	std::string m_path;
// 	std::string m_query;
// 	std::string m_fragment;

// };

// // --------------------------------------------------------------------
// // Is a valid url?

// bool is_valid_url(const std::string& url)
// {
// 	enum {
// 		scheme1, scheme2,
// 		authority,
// 		path,
// 		query,
// 		fragment,
// 		error
// 	} state = scheme1;

// 	auto restart = [&state]()
// 	{
// 		if (state < authority)
// 			return authority;
// 		if (state < path)
// 			return path;
// 		if (state < query)
// 			return query;
// 		if (state < fragment)
// 			return fragment;
// 		return error;
// 	};

// 	bool result = true;

// 	for (auto i = url.begin(); i != url.end(); ++i)
// 	{
// 		unsigned char ch = *i;

// 		switch (state)
// 		{
// 			case scheme1:
// 				if (std::isalpha(ch))
// 					state = scheme2;
// 				else
// 					state = restart();
// 				break;

// 			case scheme2:
// 				if (ch == ':')
// 					state = authority;
// 				else if (not (std::isalnum(ch) or ch == '+' or ch == '-' or ch == '.'))
// 					state = restart();
// 		}

// 	}

// 	for (unsigned char ch: url)
// 	{
// 		if (ch < 32 or ch >= 128 or (kURLAcceptable[ch - 32] & 4) == 0)
// 		{
// 			result = false;
// 			break;
// 		}
// 	}

// 	return result;
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
