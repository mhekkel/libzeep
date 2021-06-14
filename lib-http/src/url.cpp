//            Copyright Maarten L. Hekkelman, 2021
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/http/url.hpp>

namespace zeep::http
{

// --------------------------------------------------------------------

enum class url_parser_state
{
	scheme			= 100,
	authority		= 200,
	path			= 300,
	query			= 400,
	fragment		= 500,
	error			= 600
};

class url_parser
{
  public:
	url_parser(const std::string& s)
		: m_s(s), m_i(m_s.begin())
	{
		parse();
	}

  private:

	constexpr bool is_gen_delim(int ch) const
	{
		return ch == ':' or ch == '/' or ch == '?' or ch == '#' or ch == '[' or ch == ']' or ch == '@';
	}

	constexpr bool is_sub_delim(int ch) const
	{
		return ch == '!' or ch == '$' or ch == '&' or ch == '\'' or ch == '(' or ch == ')' or ch == '*' or ch == '+' or ch == ',' or ch == ';' or ch == '=';
	}

	constexpr bool is_reserved(int ch) const
	{
		return is_gen_delim(ch) or is_sub_delim(ch);
	}

	constexpr bool is_unreserved(int ch) const
	{
		return std::isalnum(ch) or ch == '-' or ch == '.' or ch == '_' or ch == '~';
	}

	int get_next_char()
	{
		return m_i != m_s.end() ? static_cast<unsigned char>(*m_i++) : -1;
	}

	void parse();

	void retract()
	{
		--m_i;
	}

	void restart()
	{
		m_i = m_s.begin();
		switch (m_start)
		{
			case url_parser_state::scheme:
				break;

		}
	}

	const std::string& m_s;
	std::string::const_iterator m_i;
	url_parser_state m_state = url_parser_state::scheme;
	url_parser_state m_start = url_parser_state::scheme;
};



// --------------------------------------------------------------------
// Is a valid url?

bool is_valid_url(const std::string& url)
{
	enum {
		scheme1, scheme2,
		authority,
		path,
		query,
		fragment,
		error
	} state = scheme1;

	auto restart = [&state]()
	{
		if (state < authority)
			return authority;
		if (state < path)
			return path;
		if (state < query)
			return query;
		if (state < fragment)
			return fragment;
		return error;
	};

	bool result = true;

	for (auto i = url.begin(); i != url.end(); ++i)
	{
		unsigned char ch = *i;

		switch (state)
		{
			case scheme1:
				if (std::isalpha(ch))
					state = scheme2;
				else
					state = restart(); 
				break;
			
			case scheme2:
				if (ch == ':')
					state = authority;
				else if (not (std::isalnum(ch) or ch == '+' or ch == '-' or ch == '.'))
					state = restart();
		}

	}


	for (unsigned char ch: url)
	{
		if (ch < 32 or ch >= 128 or (kURLAcceptable[ch - 32] & 4) == 0)
		{
			result = false;
			break;
		}
	}

	return result;
}

} // namespace zeep::http

