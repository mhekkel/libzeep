//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <regex>
#include <codecvt>
#include <cmath>
#include <deque>
#include <iostream>

#include <zeep/unicode-support.hpp>

#include "format.hpp"

namespace
{

std::string decimal_point(std::locale loc)
{
	std::string result;
	
	if (std::has_facet<std::numpunct<wchar_t>>(loc))
	{
		std::wstring s{ std::use_facet<std::numpunct<wchar_t>>(loc).decimal_point() };
		
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		result = conv.to_bytes(s);
	}

	return result;
}

std::string thousands_sep(std::locale loc)
{
	std::string result;
	
	if (std::has_facet<std::numpunct<wchar_t>>(loc))
	{
		std::wstring s{ std::use_facet<std::numpunct<wchar_t>>(loc).thousands_sep() };
		
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		result = conv.to_bytes(s);
	}

	return result;
}

std::string grouping(std::locale loc)
{
	std::string result;
	
	if (std::has_facet<std::numpunct<wchar_t>>(loc))
		result = std::use_facet<std::numpunct<wchar_t>>(loc).grouping();

	return result;
}

struct thousand_grouping
{
  public:
	thousand_grouping(std::locale loc)
		: m_sep(thousands_sep(loc)), m_grouping(grouping(loc))
	{
	}
	
	bool operator()(int exp10) const
	{
		std::deque<int> gs(m_grouping.begin(), m_grouping.end());
		
		bool result = false;

		if (not gs.empty())
		{
			int g = gs.front();
			
			for (;;)
			{
				if (exp10 < g)
					break;

				if (exp10 == g)
				{
					result = true;
					break;
				}
				
				if (gs.size() > 1)
					gs.pop_front();

				g += gs.front();
			}
		}

		return result;		
	}
	
	std::string separator() const
	{
		return m_sep;
	}

  private:
	std::string	m_sep, m_grouping;
};

template<typename T>
class Decimal
{
  public:
	Decimal(T x);

	std::string formatFixed(int minIntDigits, int decimals, std::locale loc);
	
	friend std::ostream& operator<<(std::ostream& os, Decimal d)
	{
		os << "crunched: " << d.m_crunched << " exp10: " << d.m_exp10 << " dec: " << d.m_dec;
		return os;
	}

  private:

	std::tuple<std::string,int> roundDecimal(int n);

	T			m_crunched;
	int			m_exp10;
	std::string	m_dec;
};

template<typename T>
Decimal<T>::Decimal(T x)
{
	// CrunchDouble

	int exp = (x == 0) ? 0 : static_cast<int>(1 + std::logb(x));
	int n = m_exp10 = (exp * 301029) / 1000000;

	auto p = 10.0;

	if (n < 0)
	{
		for (n = -n; n > 0; n >>= 1)
		{
			if (n & 1)
				x *= p;
			p *= p;
		}
	}
	else
	{
		T f = 1.0;
		for (; n > 0; n >>= 1)
		{
			if (n & 1)
				f *= p;
			p *= p;
		}
		x /= f;
	}
	
	if (x != 0)
	{
		while (std::fabs(x) >= 1.0)
		{
			x *= 0.1;
			++m_exp10;
		}
		
		while (std::fabs(x) < 0.1)
		{
			x *= 10.0;
			--m_exp10;
		}
	}
	
	m_crunched = x;

	// Num2Dec
	
	int digits = std::numeric_limits<T>::digits10 + 1;
	int ix = 0;
	if (x < 0)
		x = -x;

	const double kDigitValues[8] =
	{
		1.0E+01,
		1.0E+02,
		1.0E+03,
		1.0E+04,
		1.0E+05,
		1.0E+06,
		1.0E+07,
		1.0E+08
	};
	
	while (digits > 0)
	{
		n = digits;
		if (n > 8)
			n = 8;
		
		digits -= n;
		m_dec.insert(m_dec.end(), n, ' ');
		
		x *= kDigitValues[n - 1];
		
		auto lx = lrint(trunc(x));
		x -= lx;
		
		for (int i = n - 1; i >= 0; --i)
		{
			m_dec[ix + i] = lx % 10 + '0';
			lx /= 10;
		}
		ix += 8;
	}
}

template<typename T>
std::string Decimal<T>::formatFixed(int intDigits, int decimals, std::locale loc)
{
	int digits = decimals + intDigits;
	if (m_exp10 > intDigits)
		digits += m_exp10 - intDigits;

	int exp10;
	std::string dec;
	
	std::tie(dec, exp10) = roundDecimal(decimals + m_exp10);
	
	std::string s;
	thousand_grouping tg(loc);
		
	auto dp = dec.begin(), de = dec.end();
	int exp = intDigits;
	if (exp < exp10)
		exp = exp10;
	
	for (int i = 0; i < digits; ++i)
	{
		if (i > 0)
		{
			if (tg(exp))
				s += tg.separator();
			else if (exp == 0)
				s += decimal_point(loc);
		}
		
		if (exp <= exp10 and dp != de)
			s += *dp++;
		else
			s += '0';
		
		--exp;
	}
	
	return s;
}

template<typename T>
std::tuple<std::string,int> Decimal<T>::roundDecimal(int newLength)
{
	std::string dec = m_dec;
	int exp10 = m_exp10;
	
	int l = static_cast<int>(dec.length());
	
	if (newLength < 0)
		dec = "0";
	else if (newLength < l)
	{
		l = newLength + 1;
		
		int carry = dec[l - 1] >= '5';
		dec.resize(l - 1);
		
		while (newLength > 0)
		{
			--l;
			int c = dec[l - 1] -'0' + carry;
			carry = c > 9;
			if (carry == 1)
			{
				--newLength;
				dec.resize(newLength);
			}
			else
			{
				dec[l - 1] = static_cast<char>(c + '0');
				dec.resize(l);
				break;
			}
		}
		
		if (carry == 1)
		{
			++exp10;
			dec += '1';
		}
		else if (newLength == 0)
			dec = "0";
	}
	
	return std::make_tuple(dec, exp10);
}

}

namespace zeep::http
{

std::string FormatDecimal(double d, int integerDigits, int decimalDigits, std::locale loc)
{
	Decimal<double> dec(d);
	
	return dec.formatFixed(integerDigits, decimalDigits, loc);
}

}

