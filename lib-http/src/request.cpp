// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <cassert>
#include <sstream>

#include <zeep/http/request.hpp>
#include <zeep/http/server.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

namespace ba = boost::algorithm;
namespace bl = boost::locale;

namespace zeep::http
{

void request::clear()
{
	m_request_line.clear();
	m_timestamp = boost::posix_time::second_clock::local_time();

	method = method_type::UNDEFINED;
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

	static std::regex rx(MEDIARANGE);

	assert(type);
	if (type == nullptr)
		return 1.0;

	std::string t1(type), t2;
	std::string::size_type s = t1.find('/');
	if (s != std::string::npos)
	{
		t2 = t1.substr(s + 1);
		t1.erase(s, t1.length() - s);
	}
	
	for (const header& h: headers)
	{
		if (h.name != "Accept")
			continue;
		
		result = 0;

		std::string::size_type b = 0, e = h.value.find(',');
		for (;;)
		{
			if (e == std::string::npos)
				e = h.value.length();
			
			std::string mediarange = h.value.substr(b, e - b);
			
			std::smatch m;
			if (std::regex_search(mediarange, m, rx))
			{
				std::string type1 = m[1].str();
				std::string type2 = m[2].str();

				float value = 1.0f;
				if (m[3].matched)
					value = std::stof(m[3].str());
				
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

bool request::keep_alive() const
{
	bool result = false;

	for (const header& h: headers)
	{
		if (h.name != "Connection")
			continue;

		result = iequals(h.value, "keep-alive");
		break;
	}

	return result;
}

void request::set_header(const char* name, const std::string& value)
{
	bool replaced = false;

	for (header& h: headers)
	{
		if (not iequals(h.name, name))
			continue;
		
		h.value = value;
		replaced = true;
		break;
    }

	if (not replaced)
		headers.push_back({ name, value });
}

std::string request::get_header(const char* name) const
{
	std::string result;

	for (const header& h: headers)
	{
		if (not iequals(h.name, name))
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

std::pair<std::string,bool> get_urlencode_parameter(const std::string& s, const char* name)
{
	std::string::size_type b = 0;
	std::string result;
	bool found = false;
	size_t nlen = strlen(name);
	
	while (b != std::string::npos)
	{
		std::string::size_type e = s.find_first_of("&;", b);
		std::string::size_type n = (e == std::string::npos) ? s.length() - b : e - b;
		
		if ((n == nlen or (n > nlen + 1 and s[b + nlen] == '=')) and strncmp(name, s.c_str() + b, nlen) == 0)
		{
			found = true;

			if (n == nlen)
				result = name;	// what else?
			else
			{
				b += nlen + 1;
				result = s.substr(b, e - b);
				result = decode_url(result);
			}
			
			break;
		}
		
		b = e == std::string::npos ? e : e + 1;
	}

	return std::make_pair(result, found);
}

std::tuple<std::string,bool> request::get_parameter_ex(const char* name) const
{
	std::string result, contentType = get_header("Content-Type");
	bool found = false;

	if (ba::starts_with(contentType, "application/x-www-form-urlencoded"))
	{
		tie(result, found) = get_urlencode_parameter(payload, name);
		if (found)
			return std::make_tuple(result, true);
	}

	auto b = uri.find('?');
	if (b != std::string::npos)
	{
		tie(result, found) = get_urlencode_parameter(uri.substr(b + 1), name);
		if (found)
			return std::make_tuple(result, true);
	}

	if (ba::starts_with(contentType, "multipart/form-data"))
	{
		std::string::size_type b = contentType.find("boundary=");
		if (b != std::string::npos)
		{
			std::string boundary = contentType.substr(b + strlen("boundary="));
			
			enum { START, HEADER, CONTENT, SKIP } state = SKIP;
			
			std::string contentName;
			std::regex rx("content-disposition:\\s*form-data;.*?\\bname=\"([^\"]+)\".*", std::regex::icase);
			std::smatch m;
			
			std::string::size_type i = 0, r = 0, l = 0;
			
			for (i = 0; i <= payload.length(); ++i)
			{
				if (i < payload.length() and payload[i] != '\r' and payload[i] != '\n')
					continue;

				// we have found a 'line' at [l, i)
				if (payload.compare(l, 2, "--") == 0 and
					payload.compare(l + 2, boundary.length(), boundary) == 0)
				{
					// if we're in the content state or if this is the last line
					if (state == CONTENT or payload.compare(l + 2 + boundary.length(), 2, "--") == 0)
					{
						if (r > 0)
						{
							auto n = l - r;
							if (n >= 1 and payload[r + n - 1] == '\n')
								--n;
							if (n >= 1 and payload[r + n - 1] == '\r')
								--n;

							result.assign(payload, r, n);
						}
							
						break;
					}
					
					// Not the last, so it must be a separator and we're now in the Header part
					state = HEADER;
				}
				else if (state == HEADER)
				{
					if (l == i)	// empty line
					{
						if (contentName == name)
						{
							found = true;
							state = CONTENT;

							r = i + 1;
							if (payload[i] == '\r' and payload[i + 1] == '\n')
								r = i + 2;
						}
						else
							state = SKIP;
					}
					else if (std::regex_match(payload.begin() + l, payload.begin() + i, m, rx))
						contentName = m[1].str();
				}
				
				if (payload[i] == '\r' and payload[i + 1] == '\n')
					++i;

				l = i + 1;
			}
		}

		ba::replace_all(result, "\r\n", "\n");
	}
	
	return make_tuple(result, found);
}

std::multimap<std::string,std::string> request::get_parameters() const
{
	std::string ps;
	
	if (method == method_type::POST)
	{
		std::string contentType = get_header("Content-Type");
		
		if (ba::starts_with(contentType, "application/x-www-form-urlencoded"))
			ps = payload;
	}
	else if (method == method_type::GET or method == method_type::PUT)
	{
		std::string::size_type d = uri.find('?');
		if (d != std::string::npos)
			ps = uri.substr(d + 1);
	}

	std::multimap<std::string,std::string> parameters;

	while (not ps.empty())
	{
		std::string::size_type e = ps.find_first_of("&;");
		std::string param;
		
		if (e != std::string::npos)
		{
			param = ps.substr(0, e);
			ps.erase(0, e + 1);
		}
		else
			swap(param, ps);
		
		if (not param.empty())
		{
			std::string name, value;
	
			std::string::size_type d = param.find('=');
			if (d != std::string::npos)
			{
				name = param.substr(0, d);
				value = param.substr(d + 1);
			}

			parameters.emplace(decode_url(name), decode_url(value));
		}
	}

	// std::multimap<std::string,std::string> parameters;

	// for (auto m : { method_type::POST, method_type::GET })
	// {
	// 	std::string ps;

	// 	// skip payload unless this is a POST

	// 	switch (m)
	// 	{
	// 		case method_type::POST:
	// 			if (method == m)
	// 			{
	// 				std::string contentType = get_header("Content-Type");
					
	// 				if (ba::starts_with(contentType, "application/x-www-form-urlencoded"))
	// 					ps = payload;
	// 			}
	// 			break;
			
	// 		case method_type::GET:
	// 		{
	// 			std::string::size_type d = uri.find('?');
	// 			if (d != std::string::npos)
	// 				ps = uri.substr(d + 1);
	// 			break;
	// 		}

	// 		default:
	// 			;
	// 	}

	// 	while (not ps.empty())
	// 	{
	// 		std::string::size_type e = ps.find_first_of("&;");
	// 		std::string param;
			
	// 		if (e != std::string::npos)
	// 		{
	// 			param = ps.substr(0, e);
	// 			ps.erase(0, e + 1);
	// 		}
	// 		else
	// 			swap(param, ps);
			
	// 		if (not param.empty())
	// 		{
	// 			std::string name, value;
		
	// 			std::string::size_type d = param.find('=');
	// 			if (d != std::string::npos)
	// 			{
	// 				name = param.substr(0, d);
	// 				value = param.substr(d + 1);
	// 			}

	// 			parameters.emplace(decode_url(name), decode_url(value));
	// 		}
	// 	}
	// }

	return parameters;
}

file_param request::get_file_parameter(const char* name) const
{
	std::string contentType = get_header("Content-Type");
	file_param result;
	bool found = false;

	if (ba::starts_with(contentType, "multipart/form-data"))
	{
		std::string::size_type b = contentType.find("boundary=");
		if (b != std::string::npos)
		{
			std::string boundary = contentType.substr(b + strlen("boundary="));
			
			enum { START, HEADER, CONTENT, SKIP } state = SKIP;
			
			std::string contentName;
			std::regex rx_disp(R"x(content-disposition:\s*form-data(;.+))x", std::regex::icase);
			std::regex rx_cont(R"x(content-type:\s*(\S+/[^;]+)(;.*)?)x", std::regex::icase);
			std::smatch m;
			
			std::string::size_type i = 0, r = 0, l = 0;
			
			for (i = 0; i <= payload.length(); ++i)
			{
				if (i < payload.length() and payload[i] != '\r' and payload[i] != '\n')
					continue;

				// we have found a 'line' at [l, i)
				if (payload.compare(l, 2, "--") == 0 and
					payload.compare(l + 2, boundary.length(), boundary) == 0)
				{
					// if we're in the content state or if this is the last line
					if (state == CONTENT or payload.compare(l + 2 + boundary.length(), 2, "--") == 0)
					{
						if (r > 0)
						{
							auto n = l - r;
							if (n >= 1 and payload[r + n - 1] == '\n')
								--n;
							if (n >= 1 and payload[r + n - 1] == '\r')
								--n;

							result.data = payload.data() + r;
							result.length = n;
						}
							
						break;
					}
					
					// Not the last, so it must be a separator and we're now in the Header part
					state = HEADER;
				}
				else if (state == HEADER)
				{
					if (l == i)	// empty line
					{
						if (contentName == name)
						{
							state = CONTENT;
							found = true;

							r = i + 1;
							if (payload[i] == '\r' and payload[i + 1] == '\n')
								r = i + 2;
						}
						else
							state = SKIP;
					}
					else if (std::regex_match(payload.begin() + l, payload.begin() + i, m, rx_disp))
					{
						auto p = m[1].str();
						std::regex re(R"rx(;\s*(\w+)=("[^"]*"|'[^']*'|\w+))rx");

						auto b = p.begin();
						auto e = p.end();
						std::match_results<std::string::iterator> m;
						while (b < e and std::regex_search(b, e, m, re))
						{
							auto key = m[1].str();
							auto value = m[2].str();
							if (value.length() > 1 and ((value.front() == '"' and value.back() == '"') or (value.front() == '\'' and value.back() == '\'')))
								value = value.substr(1, value.length() - 2);

							if (key == "name")
								contentName = value;
							else if (key == "filename")
								result.filename = value;

							b = m[0].second;
						}
					}
					else if (std::regex_match(payload.begin() + l, payload.begin() + i, m, rx_cont))
					{
						result.mimetype = m[1].str();
						if (ba::starts_with(result.mimetype, "multipart/"))
							throw std::runtime_error("multipart file uploads are not supported");
					}
				}
				
				if (payload[i] == '\r' and payload[i + 1] == '\n')
					++i;

				l = i + 1;
			}
		}
	}
	
	if (not found)
		result = {};

	return result;
}

std::string request::get_cookie(const char* name) const
{
	for (const header& h : headers)
	{
		if (h.name != "Cookie")
			continue;

		std::vector<std::string> rawCookies;
		ba::split(rawCookies, h.value, ba::is_any_of(";"));

		for (std::string& cookie : rawCookies)
		{
			ba::trim(cookie);

			auto d = cookie.find('=');
			if (d == std::string::npos)
				continue;
			
			if (cookie.compare(0, d, name) == 0)
				return cookie.substr(d + 1);
		}
	}

	return "";
}

void request::set_cookie(const char* name, const std::string& value)
{
	std::map<std::string,std::string> cookies;
	for (auto& h: headers)
	{
		if (not iequals(h.name, "Cookie"))
			continue;
		
		std::vector<std::string> rawCookies;
		ba::split(rawCookies, h.value, ba::is_any_of(";"));

		for (std::string& cookie : rawCookies)
		{
			ba::trim(cookie);

			auto d = cookie.find('=');
			if (d == std::string::npos)
				continue;
			
			cookies[cookie.substr(0, d)] = cookie.substr(d + 1);
		}
	}

	headers.erase(
		std::remove_if(headers.begin(), headers.end(), [](header& h) { return iequals(h.name, "Cookie"); }),
		headers.end());

	cookies[name] = value;

	std::ostringstream cs;
	bool first = true;
	for (auto& cookie: cookies)
	{
		if (first)
			first = false;
		else
			cs << "; ";

		cs << cookie.first << '=' << cookie.second;
	}

	set_header("Cookie", cs.str());
}

const std::map<std::string,std::vector<std::string>>
	kLocalesPerLang = {
		{ "ar", { "AE", "BH", "DZ", "EG", "IQ", "JO", "KW", "LB", "LY", "MA", "OM", "QA", "SA", "SD", "SY", "TN", "YE" } },
		{ "be", { "BY" } },
		{ "bg", { "BG" } },
		{ "ca", { "ES" } },
		{ "cs", { "CZ" } },
		{ "da", { "DK" } },
		{ "de", { "AT", "CH", "DE", "LU" } },
		{ "el", { "GR" } },
		{ "en", { "US", "AU", "CA", "GB", "IE", "IN", "NZ", "ZA" } },
		{ "es", { "AR", "BO", "CL", "CO", "CR", "DO", "EC", "ES", "GT", "HN", "MX", "NI", "PA", "PE", "PR", "PY", "SV", "UY", "VE" } },
		{ "et", { "EE" } },
		{ "fi", { "FI" } },
		{ "fr", { "BE", "CA", "CH", "FR", "LU" } },
		{ "hi", { "IN" } },
		{ "hr", { "HR" } },
		{ "hu", { "HU" } },
		{ "is", { "IS" } },
		{ "it", { "CH", "IT" } },
		{ "iw", { "IL" } },
		{ "ja", { "JP" } },
		{ "ko", { "KR" } },
		{ "lt", { "LT" } },
		{ "lv", { "LV" } },
		{ "mk", { "MK" } },
		{ "nl", { "NL", "BE" } },
		{ "no", { "NO", "NO_NY" } },
		{ "pl", { "PL" } },
		{ "pt", { "BR", "PT" } },
		{ "ro", { "RO" } },
		{ "ru", { "RU" } },
		{ "sk", { "SK" } },
		{ "sl", { "SI" } },
		{ "sq", { "AL" } },
		{ "sr", { "BA", "CS" } },
		{ "sv", { "SE" } },
		{ "th", { "TH", "TH_TH" } },
		{ "tr", { "TR" } },
		{ "uk", { "UA" } },
		{ "vi", { "VN" } },
		{ "zh", { "CN", "HK", "TW" } }
	};

std::locale& request::get_locale() const
{
	if (not m_locale)
	{
		auto acceptedLanguage = get_header("Accept-Language");

		std::string preferred;
		std::vector<std::string> accepted;
		ba::split(accepted, acceptedLanguage, ba::is_any_of(","));

		struct lang_score
		{
			std::string lang, region;
			float score;
			std::locale loc;
			bool operator<(const lang_score& rhs) const
			{
				return score > rhs.score;
			}
		};

		std::vector<lang_score> scores;

		std::regex r(R"(([[:alpha:]]{1,8})(?:-([[:alnum:]]{1,8}))?(?:;q=([01](?:\.\d{1,3})))?)");

		auto tryLangRegion = [&scores](const std::string& lang, const std::string& region, float score)
		{
			try
			{
				auto name = lang + '_' + region + ".UTF-8";
				std::locale loc(name);
				if (iequals(loc.name(), name))
					scores.push_back({lang, region, score, loc});
			}
			catch(const std::exception& e)
			{
			}
		};

		for (auto& l: accepted)
		{
			std::smatch m;
			if (std::regex_search(l, m, r))
			{
				float score = 1;
				if (m[3].matched)
					score = std::stof(m.str(3));
				
				auto lang = m.str(1);

				if (m[2].matched)
					tryLangRegion(lang, m[2], score);
				else if (kLocalesPerLang.count(lang))
				{
					for (auto region: kLocalesPerLang.at(lang))
						tryLangRegion(lang, region, score);
				}
			}
		}

		if (scores.empty())
			m_locale.reset(new std::locale("C"));
		else
		{
			std::stable_sort(scores.begin(), scores.end());
			m_locale.reset(new std::locale(scores.front().loc));
		}
	}

	return *m_locale;
}

std::string request::get_request_line() const
{
	return to_string(method) + std::string{' '} + uri + " HTTP/" + std::to_string(http_version_major) + '.' + std::to_string(http_version_minor);
}

namespace
{
const char
		kNameValueSeparator[] = { ':', ' ' },
		kCRLF[] = { '\r', '\n' };
}

void request::to_buffers(std::vector<boost::asio::const_buffer>& buffers)
{
	m_request_line = get_request_line();
	buffers.push_back(boost::asio::buffer(m_request_line));
	buffers.push_back(boost::asio::buffer(kCRLF));
	
	for (header& h: headers)
	{
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(kNameValueSeparator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(kCRLF));
	}

	buffers.push_back(boost::asio::buffer(kCRLF));
	buffers.push_back(boost::asio::buffer(payload));
}

std::iostream& operator<<(std::iostream& io, request& req)
{
	std::vector<boost::asio::const_buffer> buffers;

	req.to_buffers(buffers);

	for (auto& b: buffers)
		io.write(boost::asio::buffer_cast<const char*>(b), boost::asio::buffer_size(b));

	return io;
}

void request::debug(std::ostream& os) const
{
	os << get_request_line() << std::endl;
	for (const header& h: headers)
		os << h.name << ": " << h.value << std::endl;
}

} // zeep::http
