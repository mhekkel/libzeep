//            Copyright Maarten L. Hekkelman, 2021
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// A simple uri class. For now this class wraps around a liburiparser UriUriStruct.

#include <zeep/config.hpp>
#include <zeep/exception.hpp>

namespace zeep::http
{

// --------------------------------------------------------------------

class uri_parse_error : public zeep::exception
{
  public:
	uri_parse_error()
		: exception("invalid uri"){};
	uri_parse_error(const std::string &u)
		: exception("invalid uri: " + u){};
};

// --------------------------------------------------------------------

class uri
{
  public:
	/// \brief constructor that parses the URL in \a s, throws exception if not valid
	uri(const std::string &s);
	~uri();

	uri(const uri &u);
	uri(uri &&u);
	uri &operator=(const uri &u);
	uri &operator=(uri &&u);

	bool empty() const;

	bool is_absolute() const;

	std::string get_scheme() const;
	std::string get_host() const;
	std::filesystem::path get_path() const;
	std::string get_query() const;

	std::string string() const;

	void swap(uri &u) noexcept;

	friend std::ostream &operator<<(std::ostream &os, const uri &u);

  private:
	struct uri_impl *m_impl;
};

// --------------------------------------------------------------------

bool is_valid_uri(const std::string& uri);

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
