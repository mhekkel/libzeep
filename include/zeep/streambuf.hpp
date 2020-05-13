//          Copyright Maarten L. Hekkelman, 2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// A simple std::streambuf implementation that wraps around const char* data.

#include <zeep/config.hpp>

#include <streambuf>

namespace zeep
{

// --------------------------------------------------------------------
/// \brief A simple class to use const char buffers as streambuf
///
/// It is very often useful to have a streambuf class that can wrap
/// wrap around a const char* pointer.

class char_streambuf : public std::streambuf
{
  public:

	/// \brief constructor taking a \a buffer and a \a length
	char_streambuf(const char* buffer, size_t length);

	/// \brief constructor taking a \a buffer using the standard strlen to determine the length
	explicit char_streambuf(const char* buffer);

	char_streambuf(const char_streambuf&) = delete;
	char_streambuf &operator=(const char_streambuf&) = delete;

  private:
	int_type underflow();
	int_type uflow();
	int_type pbackfail(int_type ch);
	std::streamsize showmanyc();
	pos_type seekoff(std::streambuf::off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which);
	pos_type seekpos(std::streambuf::pos_type pos, std::ios_base::openmode which);

  private:
	const char* const m_begin;
	const char* const m_end;
	const char* m_current;
};
	
} // namespace zeep
