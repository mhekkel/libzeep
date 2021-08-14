//          Copyright Maarten L. Hekkelman, 2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// A simple std::streambuf implementation that wraps around const char* data.

#include <zeep/config.hpp>

#include <cassert>
#include <cstring>

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
	char_streambuf(const char* buffer, size_t length)
		: m_begin(buffer), m_end(buffer + length), m_current(buffer)
	{
		assert(std::less_equal<const char*>()(m_begin, m_end));
	}

	/// \brief constructor taking a \a buffer using the standard strlen to determine the length
	char_streambuf(const char* buffer)
		: m_begin(buffer), m_end(buffer + strlen(buffer)), m_current(buffer)
	{
	}

	char_streambuf(const char_streambuf&) = delete;
	char_streambuf &operator=(const char_streambuf&) = delete;

  private:

	int_type underflow()
	{
		if (m_current == m_end)
			return traits_type::eof();

		return traits_type::to_int_type(*m_current);
	}

	int_type uflow()
	{
		if (m_current == m_end)
			return traits_type::eof();

		return traits_type::to_int_type(*m_current++);
	}

	int_type pbackfail(int_type ch)
	{
		if (m_current == m_begin or (ch != traits_type::eof() and ch != m_current[-1]))
			return traits_type::eof();

		return traits_type::to_int_type(*--m_current);
	}

	std::streamsize showmanyc()
	{
		assert(std::less_equal<const char*>()(m_current, m_end));
		return m_end - m_current;
	}

	pos_type seekoff(std::streambuf::off_type off, std::ios_base::seekdir dir, std::ios_base::openmode /*which*/)
	{
		switch (dir)
		{
			case std::ios_base::beg:
				m_current = m_begin + off;
				break;

			case std::ios_base::end:
				m_current = m_end + off;
				break;

			case std::ios_base::cur:
				m_current += off;
				break;
			
			default:
				break;
		}

		if (m_current < m_begin)
			m_current = m_begin;
		
		if (m_current > m_end)
			m_current = m_end;

		return m_current - m_begin;
	}

	pos_type seekpos(std::streambuf::pos_type pos, std::ios_base::openmode /*which*/)
	{
		m_current = m_begin + pos;

		if (m_current < m_begin)
			m_current = m_begin;
		
		if (m_current > m_end)
			m_current = m_end;

		return m_current - m_begin;
	}

  private:
	const char* const m_begin;
	const char* const m_end;
	const char* m_current;
};
	
} // namespace zeep
