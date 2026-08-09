// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2021-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the uri class, a URI parser and builder based on RFC 3986

#include "zeep/exception.hpp"
#include "zeep/unicode-support.hpp"

#include <cstdint>

namespace zeep
{

// --------------------------------------------------------------------

/// \brief Simply check the URI in \a uri, returns true if the uri is valid
/// \param uri		The URI to check
bool is_valid_uri(const std::string &uri);

/// \brief Check the URI in \a uri, returns true if the uri is fully qualified (has a scheme and path)
/// \param uri		The URI to check
bool is_fully_qualified_uri(const std::string &uri);

/// \brief Check the parameter in \a host is of the form HOST:PORT as required by CONNECT
/// \param host		The host string to check
bool is_valid_connect_host(std::string_view host) noexcept;

// --------------------------------------------------------------------

/// \brief Decode a URL using the RFC rules
/// \param s  The URL that needs to be decoded
/// \return	  The decoded URL
std::string decode_url(std::string_view s);

/// \brief Encode a URL using the RFC rules
/// \param s  The URL that needs to be encoded
/// \return	  The encoded URL
std::string encode_url(std::string_view s);

// --------------------------------------------------------------------

/// \brief the exception thrown by libzeep when an invalid uri is passed to
///        the uri constructor.
class uri_parse_error : public zeep::exception
{
  public:
	uri_parse_error()
		: exception("invalid uri") {};
	uri_parse_error(const std::string &u)
		: exception("invalid uri: " + u) {};
};

// --------------------------------------------------------------------

/// \brief A class modelling a URI based on RFC 3986 https://www.rfc-editor.org/rfc/rfc3986
///
/// All components are stored separately. Scheme and host are converted to lower case.
/// Path segments are stored decoded whereas query and fragment are stored encoded.
/// This is to avoid double encoding and ease post processing of queries e.g.
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
	template <typename InputIterator>
	uri(InputIterator b, InputIterator e)
		requires(std::is_constructible_v<std::string, typename InputIterator::value_type>)
		: uri()
	{
		for (auto i = b; i != e; ++i)
			m_path.emplace_back(*i);
	}

	~uri() = default;

	uri(const uri &u) = default;

	uri(uri &&u) noexcept
	{
		swap(*this, u);
	}

	uri &operator=(uri u) noexcept
	{
		swap(*this, u);
		return *this;
	}

	friend void swap(uri &lhs, uri &rhs) noexcept;

	// --------------------------------------------------------------------

	[[nodiscard]] bool has_scheme() const noexcept
	{
		return not m_scheme.empty();
	}

	[[nodiscard]] bool has_authority() const noexcept
	{
		return not(m_userinfo.empty() and m_host.empty() and m_port == 0);
	}

	[[nodiscard]] bool has_path() const noexcept
	{
		return not m_path.empty();
	}

	[[nodiscard]] bool has_query() const noexcept
	{
		return not m_query.empty();
	}

	[[nodiscard]] bool has_fragment() const noexcept
	{
		return not m_fragment.empty();
	}

	/// \brief Return true if url is empty
	[[nodiscard]] bool empty() const noexcept
	{
		return not(
			has_scheme() or has_authority() or has_path() or has_query() or has_fragment());
	}

	/// \brief Return true if the path is absolute
	[[nodiscard]] bool is_absolute() const noexcept
	{
		return m_absolutePath;
	}

	/// \brief Return the scheme
	[[nodiscard]] const std::string &get_scheme() const noexcept
	{
		return m_scheme;
	}

	/// \brief Set the scheme to \a scheme
	void set_scheme(std::string scheme)
	{
		m_scheme = std::move(scheme);
		zeep::to_lower(m_scheme);
	}

	/// \brief Return the user info
	[[nodiscard]] const std::string &get_userinfo() const noexcept
	{
		return m_userinfo;
	}

	/// \brief Set the userinfo to \a userinfo
	void set_userinfo(std::string userinfo) noexcept
	{
		m_userinfo = std::move(userinfo);
	}

	/// \brief Return the host
	[[nodiscard]] const std::string &get_host() const noexcept
	{
		return m_host;
	}

	/// \brief Set the host to \a host
	void set_host(std::string host)
	{
		m_host = std::move(host);
		zeep::to_lower(m_host);
	}

	/// \brief Return the port
	[[nodiscard]] uint16_t get_port() const noexcept
	{
		return m_port;
	}

	/// \brief Set the port to \a port
	void set_port(uint16_t port) noexcept
	{
		m_port = port;
	}

	/// \brief Return a uri containing only the path
	[[nodiscard]] uri get_path() const;

	/// \brief Get the individual segments of the path
	[[nodiscard]] const std::vector<std::string> &get_segments() const noexcept
	{
		return m_path;
	}

	/// \brief Set the path to \a path
	void set_path(const std::string &path);

	/// \brief Return the query
	[[nodiscard]] std::string get_query(bool decoded) const
	{
		return decoded ? decode_url(m_query) : m_query;
	}

	/// \brief Set the query to \a query and optionally encode it based on \a encode
	void set_query(std::string query, bool encode);

	/// \brief Return the fragment
	[[nodiscard]] std::string get_fragment(bool decoded) const
	{
		return decoded ? decode_url(m_fragment) : m_fragment;
	}

	/// \brief Set the fragment to \a fragment and optionally encode it based on \a encode
	void set_fragment(std::string fragment, bool encode);

	/// \brief Return the uri as a string
	[[nodiscard]] std::string string() const;

	/// \brief Return the uri as a string, without encoded characters
	[[nodiscard]] std::string unencoded_string() const;

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
	[[nodiscard]] bool operator==(const uri &rhs) const noexcept
	{
		return m_scheme == rhs.m_scheme and
		       m_userinfo == rhs.m_userinfo and
		       m_host == rhs.m_host and
		       m_port == rhs.m_port and
		       m_path == rhs.m_path and
		       m_query == rhs.m_query and
		       m_fragment == rhs.m_fragment and
		       m_absolutePath == rhs.m_absolutePath;
	}

	/// \brief return the uri relative from \a base.
	///
	/// If the scheme and authority of this and \a base are the same
	/// a relative uri will be returned with the path of base removed from this path.
	[[nodiscard]] uri relative(const uri &base) const;

  private:
	enum class char_class : uint8_t
	{
		gen_delim = 1 << 0,
		sub_delim = 1 << 1,
		reserved = gen_delim | sub_delim,
		unreserved = 1 << 2,
		scheme = 1 << 3,
		hexdigit = 1 << 4,
		alpha = 1 << 5
	};

	static constexpr uint8_t kCharClassTable[] = {
		// clang-format off
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
		 0,  2,  0,  1,  2,  0,  2,  2,  2,  2,  2, 10,  2, 12, 12,  1, 
		28, 28, 28, 28, 28, 28, 28, 28, 28, 28,  1,  2,  0,  2,  0,  1, 
		 1, 60, 60, 60, 60, 60, 60, 44, 44, 44, 44, 44, 44, 44, 44, 44, 
		44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,  1,  0,  1,  0,  4, 
		 0, 60, 60, 60, 60, 60, 60, 44, 44, 44, 44, 44, 44, 44, 44, 44, 
		44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,  0,  0,  0,  4,  0,
		// clang-format on
	};

  public:
	static inline constexpr bool is_char_class(int ch, char_class mask) noexcept
	{
		return ch > 0 and ch < 128 and (kCharClassTable[static_cast<uint8_t>(ch)] bitand static_cast<char>(mask)) != 0;
	}

	static inline constexpr bool is_gen_delim(int ch) noexcept
	{
		return is_char_class(ch, char_class::gen_delim);
	}

	static inline constexpr bool is_sub_delim(int ch) noexcept
	{
		return is_char_class(ch, char_class::sub_delim);
	}

	static inline constexpr bool is_reserved(int ch) noexcept
	{
		return is_char_class(ch, char_class::reserved);
	}

	static inline constexpr bool is_unreserved(int ch) noexcept
	{
		return is_char_class(ch, char_class::unreserved);
	}

	static inline constexpr bool is_scheme_start(int ch) noexcept
	{
		return is_char_class(ch, char_class::alpha);
	}

	static inline constexpr bool is_scheme(int ch) noexcept
	{
		return is_char_class(ch, char_class::scheme);
	}

	static inline constexpr bool is_xdigit(int ch) noexcept
	{
		return is_char_class(ch, char_class::hexdigit);
	}

	friend std::string encode_url(std::string_view s);

  private:
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

	bool is_userinfo(const char *&cp) noexcept
	{
		return is_unreserved(*cp) or is_sub_delim(*cp) or *cp == ':' or is_pct_encoded(cp);
	}

	bool is_reg_name(const char *&cp) noexcept
	{
		return is_unreserved(*cp) or is_sub_delim(*cp) or is_pct_encoded(cp);
	}

	bool is_pchar(const char *&cp) noexcept
	{
		return is_unreserved(*cp) or is_sub_delim(*cp) or *cp == ':' or *cp == '@' or is_pct_encoded(cp);
	}

	void parse(const char *s);
	void transform(const uri &base);
	void remove_dot_segments();

	const char *parse_scheme(const char *ch);
	const char *parse_authority(const char *ch);
	const char *parse_host(const char *ch);
	const char *parse_hierpart(const char *ch);
	const char *parse_segment(const char *ch);
	const char *parse_segment_nz(const char *ch);
	const char *parse_segment_nz_nc(const char *ch);

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

} // namespace zeep
