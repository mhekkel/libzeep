//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// internal file, should not be used outside libzeep, I guess

#include <cassert>
#include <memory>
#include <istream>

#include <zeep/xml/character-classification.hpp>
#include <zeep/exception.hpp>

namespace zeep::detail
{

using zeep::unicode;
using zeep::encoding_type;

// --------------------------------------------------------------------

template <typename T>
struct value_saver
{
	T& m_ref;
	T m_value;

	value_saver(T& value, const T& new_value) : m_ref(value), m_value(value) { m_ref = new_value; }
	~value_saver() { m_ref = m_value; }
};

class mini_stack
{
public:
	mini_stack() : m_ix(-1) {}

	unicode top()
	{
		assert(m_ix >= 0 and m_ix < int(sizeof(m_data) / sizeof(unicode)));
		return m_data[m_ix];
	}

	void pop()
	{
		--m_ix;
	}

	void push(unicode uc)
	{
		++m_ix;
		assert(m_ix < int(sizeof(m_data) / sizeof(unicode)));
		m_data[m_ix] = uc;
	}

	bool empty() const
	{
		return m_ix == -1;
	}

private:
	unicode m_data[2];
	int m_ix;
};

bool is_absolute_path(const std::string& s);

}