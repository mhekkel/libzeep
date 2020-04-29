//          Copyright Maarten L. Hekkelman, 2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <zeep/config.hpp>

#include <streambuf>

namespace zeep
{

// --------------------------------------------------------------------
// A simple class to use const char buffers as streambuf

class char_streambuf : public std::streambuf
{
  public:
	char_streambuf(const char* buffer, size_t length);
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
