// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2025-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// A simple std::streambuf implementation that wraps around const char* data.

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include <cassert>
# include <cstring>
# include <functional>
# include <streambuf>
#endif

namespace zeep
{

// --------------------------------------------------------------------
/// \brief A simple class to use const char buffers as streambuf
///
/// It is very often useful to have a streambuf class that can wrap
/// wrap around a const char* pointer.

ZEEP_EXPORT class char_streambuf : public std::streambuf
{
  public:
	/// \brief constructor taking a \a buffer and a \a length
	char_streambuf(const char *buffer, size_t length)
		: m_begin(buffer)
		, m_end(buffer + length)
		, m_current(buffer)
	{
		assert(std::less_equal<>()(m_begin, m_end));
	}

	/// \brief constructor taking a \a buffer using the standard strlen to determine the length
	char_streambuf(const char *buffer)
		: m_begin(buffer)
		, m_end(buffer + strlen(buffer))
		, m_current(buffer)
	{
	}

	char_streambuf(const char_streambuf &) = delete;
	char_streambuf &operator=(const char_streambuf &) = delete;

  protected:
	/// \brief Read one character without advancing the get pointer
	int_type underflow() override
	{
		if (m_current == m_end)
			return traits_type::eof();

		return traits_type::to_int_type(*m_current);
	}

	/// \brief Read one character and advance the get pointer
	int_type uflow() override
	{
		if (m_current == m_end)
			return traits_type::eof();

		return traits_type::to_int_type(*m_current++);
	}

	/// \brief Put a character back into the buffer
	int_type pbackfail(int_type ch) override
	{
		if (m_current == m_begin or (ch != traits_type::eof() and ch != m_current[-1]))
			return traits_type::eof();

		return traits_type::to_int_type(*--m_current);
	}

	/// \brief Return the number of characters available
	std::streamsize showmanyc() override
	{
		assert(std::less_equal<>()(m_current, m_end));
		return m_end - m_current;
	}

	/// \brief Seek to a position relative to a base location
	pos_type seekoff(std::streambuf::off_type off, std::ios_base::seekdir dir, std::ios_base::openmode /*which*/) override
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

	/// \brief Seek to an absolute position
	pos_type seekpos(std::streambuf::pos_type pos, std::ios_base::openmode /*which*/) override
	{
		m_current = m_begin + pos;

		if (m_current < m_begin)
			m_current = m_begin;

		if (m_current > m_end)
			m_current = m_end;

		return m_current - m_begin;
	}

  private:
	/// @cond

	const char *const m_begin; ///< Start of the buffer
	const char *const m_end;   ///< End of the buffer
	const char *m_current;     ///< Current read position

	/// @endcond
};

} // namespace zeep
