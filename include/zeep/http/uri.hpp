//            Copyright Maarten L. Hekkelman, 2021
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// A simple uri class. Uses a parser based on simple regular expressions

#include <zeep/config.hpp>
#include <zeep/exception.hpp>

#include <filesystem>

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
	uri();

	/// \brief constructor that parses the URI in \a s, throws exception if not valid
	explicit uri(const std::string &s);

	/// \brief constructor that parses the URI in \a s, throws exception if not valid
	explicit uri(const std::string &s, const uri &base);

	~uri();

	uri(const uri &u);
	uri(uri &&u);
	uri &operator=(const uri &u);
	uri &operator=(uri &&u);

	/// \brief Return true if url is empty (not useful I guess, you cannot construct an empty url yet)
	bool empty() const;

	/// \brief Return true if the path is absolute
	bool is_absolute() const;

	/// \brief Return the scheme
	std::string get_scheme() const;

	/// \brief Set the scheme to \a scheme
	void set_scheme(const std::string &scheme);

	/// \brief Return the user info
	std::string get_userinfo() const;

	/// \brief Set the userinfo to \a userinfo
	void set_userinfo(const std::string &userinfo);

	/// \brief Return the host
	std::string get_host() const;

	/// \brief Set the host to \a host
	void set_host(const std::string &host);

	/// \brief Return a uri containing only the path
	uri get_path() const;

	/// \brief Set the path to \a path
	void set_path(const std::filesystem::path &path);

	/// \brief Set the path to \a path
	void set_path(const std::vector<std::string> &path);

	/// \brief Set the path to \a path
	void set_path(const std::string &path);

	/// \brief Return the query
	std::string get_query() const;

	/// \brief Set the query to \a query
	void set_query(const std::string &query);

	/// \brief Return the fragment
	std::string get_fragment() const;

	/// \brief Set the fragment to \a fragment
	void set_fragment(const std::string &fragment);

	/// \brief Return the uri as a string
	std::string string() const;

	void swap(uri &u) noexcept;

	friend std::ostream &operator<<(std::ostream &os, const uri &u);

  private:
	struct uri_impl *m_impl;
};

/// @brief Append \a rhs to the uri in \a uri and return the result
/// @param uri The uri to extend
/// @param rhs The path to add to the uri
/// @return The newly constructed uri
uri operator/(uri uri, const std::filesystem::path &rhs);

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
