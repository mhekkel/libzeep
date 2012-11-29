//  Copyright Maarten L. Hekkelman, Radboud University 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <cassert>
#include <sstream>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <zeep/http/request.hpp>

using namespace std;

namespace zeep { namespace http {

float request::accept(const char* type) const
{
	float result = 1.0f;

#define IDENT		"[-+.a-z0-9]+"
#define TYPE		"\\*|" IDENT
#define MEDIARANGE	"\\s*(" TYPE ")/(" TYPE ").*?(?:;\\s*q=(\\d(?:\\.\\d?)?))?"

	static boost::regex rx(MEDIARANGE);

	assert(type);
	if (type == nullptr)
		return 1.0;

	string t1(type), t2;
	string::size_type s = t1.find('/');
	if (s != string::npos)
	{
		t2 = t1.substr(s + 1);
		t1.erase(s, t1.length() - s);
	}
	
	foreach (const header& h, headers)
	{
		if (h.name != "Accept")
			continue;
		
		result = 0;

		string::size_type b = 0, e = h.value.find(',');
		for (;;)
		{
			if (e == string::npos)
				e = h.value.length();
			
			string mediarange = h.value.substr(b, e - b);
			
			boost::smatch m;
			if (boost::regex_search(mediarange, m, rx))
			{
				string type1 = m[1].str();
				string type2 = m[2].str();

				float value = 1.0f;
				if (m[3].matched)
					value = boost::lexical_cast<float>(m[3].str());
				
				if (type1 == t1 and type2 == t2)
				{
					result = value;
					break;
				}
				
				if ((type1 == t1 and type2 == "*") or
					(type1 == "*" and type2 == "*"))
				{
					if (result < value)
						result = value;
				}
			}
			
			if (e == h.value.length())
				break;

			b = e + 1;
			while (b < h.value.length() and isspace(h.value[b]))
				++b;
			e = h.value.find(',', b);
		}

		break;
	}
	
	return result;
}

} // http
} // zeep
