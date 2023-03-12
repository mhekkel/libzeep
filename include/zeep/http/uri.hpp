//          Copyright Maarten L. Hekkelman, 2021-2023
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// A simple uri class.

#include <zeep/config.hpp>
#include <zeep/exception.hpp>
#include <zeep/unicode-support.hpp>

namespace zeep::http
{

// --------------------------------------------------------------------

/// \brief the exception thrown by libzeep when an invalid uri is passed to
///        the uri constructor.
class uri_parse_error : public zeep::exception
{
  public:
	uri_parse_error()
		: exception("invalid uri"){};
	uri_parse_error(const std::string &u)
		: exception("invalid uri: " + u){};
};

// --------------------------------------------------------------------

/// \brief Simple class, should be extended to have setters, one day
class uri
{
  public:
	/// \brief constructor for an empty uri
	uri() = default;

	/// \brief constructor that parses the URI in \a s, throws exception if not valid
	uri(const std::string &s);

	/// \brief constructor that parses the URI in \a s, throws exception if not valid
	uri(const char *s);

	/// \brief constructor that parses the URI in \a s relative to the baseuri in \a base, throws exception if not valid
	uri(const std::string &s, const uri &base);

	/// \brief constructor taking two iterators into path segments, for a relative path
	template<typename InputIterator, std::enable_if_t<std::is_constructible_v<std::string, typename InputIterator::value_type>, int> = 0>
	uri(InputIterator b, InputIterator e)
		: uri()
	{
		for (auto i = b; i != e; ++i)
			m_path.emplace_back(*i);
	}

	~uri() = default;

	uri(const uri &u) = default;
	uri(uri &&u) = default;
	uri &operator=(const uri &u) = default;
	uri &operator=(uri &&u) = default;

	void swap(uri &u);

	// --------------------------------------------------------------------
	
	bool has_scheme() const
	{
		return not m_scheme.empty();
	}

	bool has_authority() const
	{
		return not (m_userinfo.empty() and m_host.empty() and m_port == 0);
	}

	bool has_path() const
	{
		return not m_path.empty();
	}

	bool has_query() const
	{
		return not m_query.empty();
	}

	bool has_fragment() const
	{
		return not m_fragment.empty();
	}

	/// \brief Return true if url is empty (not useful I guess, you cannot construct an empty url yet)
	bool empty() const
	{
		return not (
			has_scheme() or has_authority() or has_path() or has_query() or has_fragment()
		);
	}

	/// \brief Return true if the path is absolute
	bool is_absolute() const
	{
		return m_absolutePath;
	}

	/// \brief Return the scheme
	std::string get_scheme() const
	{
		return m_scheme;
	}

	/// \brief Set the scheme to \a scheme
	void set_scheme(const std::string &scheme);

	/// \brief Return the user info
	std::string get_userinfo() const
	{
		return m_userinfo;
	}

	/// \brief Set the userinfo to \a userinfo
	void set_userinfo(const std::string &userinfo)
	{
		m_userinfo = userinfo;
	}

	/// \brief Return the host
	std::string get_host() const
	{
		return m_host;
	}

	/// \brief Set the host to \a host
	void set_host(const std::string &host)
	{
		m_host = host;
		zeep::to_lower(m_host);
	}

	/// \brief Return a uri containing only the path
	uri get_path() const;

	/// \brief Get the individual segments of the path
	const std::vector<std::string> &get_segments() const
	{
		return m_path;
	}

	/// \brief Set the path to \a path
	void set_path(const std::string &path);

	/// \brief Return the query
	std::string get_query() const
	{
		return m_query;
	}

	/// \brief Set the query to \a query
	void set_query(const std::string &query)
	{
		m_query = query;
	}

	/// \brief Return the fragment
	std::string get_fragment() const
	{
		return m_fragment;
	}

	/// \brief Set the fragment to \a fragment
	void set_fragment(const std::string &fragment)
	{
		m_fragment = fragment;
	}

	/// \brief Return the uri as a string
	std::string string() const;

	/// \brief Return the uri as a string, without encoded characters
	std::string unencoded_string() const;

	/// \brief Write the uri in \a u to the stream \a os
	friend std::ostream &operator<<(std::ostream &os, const uri &u)
	{
		u.write(os, true);
		return os;
	}

	/// \brief Extend path
	uri &operator/=(const uri &rhs);

	/// \brief Extend path
	friend uri operator/(uri lhs, const uri &rhs)
	{
		return lhs /= rhs;
	}

	/// \brief Comparison
	bool operator==(const uri &rhs) const
	{
		return 
			m_scheme == rhs.m_scheme and
			m_userinfo == rhs.m_userinfo and
			m_host == rhs.m_host and
			m_port == rhs.m_port and
			m_path == rhs.m_path and
			m_query == rhs.m_query and
			m_fragment == rhs.m_fragment and
			m_absolutePath == rhs.m_absolutePath;
	}

  private:
	
	enum class char_class : uint8_t
	{
		gen_delim		= 1 << 0,
		sub_delim		= 1 << 1,
		reserved		= gen_delim | sub_delim,
		unreserved		= 1 << 2,
		scheme			= 1 << 3,
	};

	static constexpr uint8_t kCharClassTable[] =
	{
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
		 0,  2,  0,  1,  2,  0,  2,  2,  2,  2,  2, 10,  2, 12, 12,  1, 
		12, 12, 12, 12, 12, 12, 12, 12, 12, 12,  1,  2,  0,  2,  0,  1, 
		 1, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
		12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,  1,  0,  1,  0,  4, 
		 0, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
		12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,  0,  0,  0,  4,  0		
	};

	inline friend constexpr bool is_char_class(char ch, char_class mask)
	{
		return ch > 0 and (kCharClassTable[static_cast<uint8_t>(ch)] bitand static_cast<char>(mask)) != 0;
	}

	static inline constexpr bool is_gen_delim(char ch)
	{
		return is_char_class(ch, char_class::gen_delim);
	}

	static inline constexpr bool is_sub_delim(char ch)
	{
		return is_char_class(ch, char_class::sub_delim);
	}

	static inline constexpr bool is_reserved(char ch)
	{
		return is_char_class(ch, char_class::reserved);
	}

	static inline constexpr bool is_unreserved(char ch)
	{
		return is_char_class(ch, char_class::unreserved);
	}

	static inline constexpr bool is_scheme_start(char ch)
	{
		return std::isalpha(ch);
	}

	static inline constexpr bool is_scheme(char ch)
	{
		return is_char_class(ch, char_class::scheme);
	}

	static inline constexpr bool is_xdigit(char ch)
	{
		return std::isxdigit(ch);
	}

	friend std::string encode_url(std::string_view s);

	// --------------------------------------------------------------------
	
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

	void parse(const char *s);
	void transform(const uri &base);
	void remove_dot_segments();

	const char *parse_scheme(const char *ch);
	const char *parse_hierpart(const char *ch);
	const char *parse_authority(const char *ch);
	const char *parse_segment(const char *ch);
	const char *parse_segment_nz(const char *ch);
	const char *parse_segment_nz_nc(const char *ch);

	const char *parse_userinfo(const char *ch);
	const char *parse_host(const char *ch);
	const char *parse_port(const char *ch);

	void write(std::ostream &os, bool encoded) const;

	// --------------------------------------------------------------------

	std::string m_scheme;
	std::string m_userinfo;
	std::string m_host;
	uint16_t m_port = 0;
	std::vector<std::string> m_path;
	std::string m_query;
	std::string m_fragment;
	bool m_absolutePath = false;
};

// --------------------------------------------------------------------

/// \brief Simply check the URI in \a uri, returns true if the uri is valid
/// \param uri		The URI to check
bool is_valid_uri(const std::string &uri);

/// \brief Check the URI in \a uri, returns true if the uri is fully qualified (has a scheme and path)
/// \param uri		The URI to check
bool is_fully_qualified_uri(const std::string &uri);

// --------------------------------------------------------------------

/// \brief Decode a URL using the RFC rules
/// \param s  The URL that needs to be decoded
/// \return	  The decoded URL
std::string decode_url(std::string_view s);

/// \brief Encode a URL using the RFC rules
/// \param s  The URL that needs to be encoded
/// \return	  The encoded URL
std::string encode_url(std::string_view s);

} // namespace zeep::http
