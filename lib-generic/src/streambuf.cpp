// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <cstring>

#include <zeep/streambuf.hpp>

// --------------------------------------------------------------------

namespace zeep
{

char_streambuf::char_streambuf(const char* buffer, size_t length)
	: m_begin(buffer), m_end(buffer + length), m_current(buffer)
{
	assert(std::less_equal<const char*>()(m_begin, m_end));
}

char_streambuf::char_streambuf(const char* buffer)
	: m_begin(buffer), m_end(buffer + strlen(buffer)), m_current(buffer)
{
}

char_streambuf::int_type char_streambuf::underflow()
{
	if (m_current == m_end)
		return traits_type::eof();

	return traits_type::to_int_type(*m_current);
}

char_streambuf::int_type char_streambuf::uflow()
{
	if (m_current == m_end)
		return traits_type::eof();

	return traits_type::to_int_type(*m_current++);
}

char_streambuf::int_type char_streambuf::pbackfail(int_type ch)
{
	if (m_current == m_begin or (ch != traits_type::eof() and ch != m_current[-1]))
		return traits_type::eof();

	return traits_type::to_int_type(*--m_current);
}

std::streamsize char_streambuf::showmanyc()
{
	assert(std::less_equal<const char*>()(m_current, m_end));
	return m_end - m_current;
}

char_streambuf::pos_type char_streambuf::seekoff(std::streambuf::off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
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

char_streambuf::pos_type char_streambuf::seekpos(std::streambuf::pos_type pos, std::ios_base::openmode which)
{
	m_current = m_begin + pos;

	if (m_current < m_begin)
		m_current = m_begin;
	
	if (m_current > m_end)
		m_current = m_end;

	return m_current - m_begin;
}


}