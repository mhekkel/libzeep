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
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <zeep/http/request.hpp>

using namespace std;

namespace zeep { namespace http {
	
void request::clear()
{
	m_request_line.clear();

	method.clear();
	uri.clear();
	http_version_major = 1;
	http_version_minor = 0;
	headers.clear();
	payload.clear();
	close = true;
	local_address.clear();
	local_port = 0;
}

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

const boost::regex
	b("(bb\\d+|meego).+mobile|android|avantgo|bada\\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\\.(browser|link)|vodafone|wap|windows (ce|phone)|xda|xiino",
		boost::regex::icase),
	v("1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\\-(n|u)|c55\\/|capi|ccwa|cdm\\-|cell|chtm|cldc|cmd\\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\\-s|devi|dica|dmob|do(c|p)o|ds(12|\\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\\-|_)|g1 u|g560|gene|gf\\-5|g\\-mo|go(\\.w|od)|gr(ad|un)|haie|hcit|hd\\-(m|p|t)|hei\\-|hi(pt|ta)|hp( i|ip)|hs\\-c|ht(c(\\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\\-(20|go|ma)|i230|iac( |\\-|\\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\\/)|klon|kpt |kwc\\-|kyo(c|k)|le(no|xi)|lg( g|\\/(k|l|u)|50|54|\\-[a-w])|libw|lynx|m1\\-w|m3ga|m50\\/|ma(te|ui|xo)|mc(01|21|ca)|m\\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\\-2|po(ck|rt|se)|prox|psio|pt\\-g|qa\\-a|qc(07|12|21|32|60|\\-[2-7]|i\\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\\-|oo|p\\-)|sdk\\/|se(c(\\-|0|1)|47|mc|nd|ri)|sgh\\-|shar|sie(\\-|m)|sk\\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\\-|v\\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\\-|tdg\\-|tel(i|m)|tim\\-|t\\-mo|to(pl|sh)|ts(70|m\\-|m3|m5)|tx\\-9|up(\\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\\-|your|zeto|zte\\-",
		boost::regex::icase);

bool request::is_mobile() const
{
	// this code is adapted from code generated by http://detectmobilebrowsers.com/
	bool result = false;
	
	foreach (const header& h, headers)
	{
		if (h.name != "User-Agent")
			continue;
		
		result = boost::regex_search(h.value, b) or boost::regex_match(h.value.substr(0, 4), v);
		break;
    }
    
    return result;
}

string request::get_header(const char* name) const
{
	string result;

	foreach (const header& h, headers)
	{
		if (h.name != name)
			continue;
		
		result = h.value;
		break;
    }

	return result;
}

void request::remove_header(const char* name)
{
	headers.erase(
		remove_if(headers.begin(), headers.end(),
			[name](const header& h) -> bool { return h.name == name; }),
		headers.end());
}

string request::get_request_line() const
{
	return (boost::format("%1% %2% HTTP/%3%.%4%")
				% method % uri
				% http_version_major % http_version_minor).str();
}

namespace
{
const char
		kNameValueSeparator[] = { ':', ' ' },
		kCRLF[] = { '\r', '\n' };
}

void request::to_buffers(vector<boost::asio::const_buffer>& buffers)
{
	m_request_line = get_request_line();
	buffers.push_back(boost::asio::buffer(m_request_line));
	buffers.push_back(boost::asio::buffer(kCRLF));
	
	foreach (header& h, headers)
	{
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(kNameValueSeparator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(kCRLF));
	}

	buffers.push_back(boost::asio::buffer(kCRLF));
	buffers.push_back(boost::asio::buffer(payload));
}

iostream& operator<<(iostream& io, request& req)
{
	vector<boost::asio::const_buffer> buffers;

	req.to_buffers(buffers);

	foreach (auto& b, buffers)
		io.write(boost::asio::buffer_cast<const char*>(b), boost::asio::buffer_size(b));

	return io;
}

void request::debug(ostream& os) const
{
	os << get_request_line() << endl;
	foreach (const header& h, headers)
		os << h.name << ": " << h.value << endl;
}

} // http
} // zeep
