//            Copyright Maarten L. Hekkelman, 2021
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// A simple url class.

#include <zeep/config.hpp>

class url
{
  public:
	/// \brief constructor that parses the URL in \a s, throws exception if not valid
	url(const std::string& s);

	url(const url& u) = default;
	url(url&& u) = default;

	url& operator=(const url& u) = default;
	url& operator=(url&& u) = default;

	std::string get_scheme() const		{ return m_scheme; }
	std::string get_authority() const	{ return m_authority; }
	std::string get_path() const		{ return m_path; }
	std::string get_query() const		{ return m_query; }
	std::string get_fragment() const	{ return m_fragment; }

  private:
	std::string m_scheme;
	std::string m_authority;
	std::string m_path;
	std::string m_query;
	std::string m_fragment;
};