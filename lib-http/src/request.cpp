// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <filesystem>

#include <boost/algorithm/string.hpp>

#include <zeep/crypto.hpp>
#include <zeep/http/server.hpp>
#include <zeep/json/parser.hpp>
#include <zeep/http/uri.hpp>

namespace ba = boost::algorithm;
namespace fs = std::filesystem;

namespace zeep::http
{

request::request(const std::string& method, const std::string& uri, std::tuple<int,int> version,
		std::vector<header>&& headers, std::string&& payload)
	: m_method(method)
	, m_uri(uri)
	, m_headers(std::move(headers))
	, m_payload(std::move(payload))
{
	m_version[0] = '0' + std::get<0>(version);
	m_version[1] = '.';
	m_version[2] = '0' + std::get<1>(version);
}

request::request(const request& req)
	: m_local_address(req.m_local_address)
	, m_local_port(req.m_local_port)
	, m_method(req.m_method)
	, m_uri(req.m_uri)
	, m_headers(req.m_headers)
	, m_payload(req.m_payload)
	, m_close(req.m_close)
	, m_timestamp(req.m_timestamp)
	, m_credentials(req.m_credentials)
	, m_remote_address(req.m_remote_address)
{
	char* d = &m_version[0];
	for (auto c: req.m_version)
		*d++ = c;
}

request& request::operator=(const request& req)
{
	if (this != &req)
	{
		m_local_address = req.m_local_address;
		m_local_port = req.m_local_port;
		m_method = req.m_method;
		m_uri = req.m_uri;
		m_version[0] = req.m_version[0];
		m_version[1] = req.m_version[1];
		m_version[2] = req.m_version[2];
		m_headers = req.m_headers;
		m_payload = req.m_payload;
		m_close = req.m_close;
		m_timestamp = req.m_timestamp;
		m_credentials = req.m_credentials;
		m_remote_address = req.m_remote_address;
	}

	return *this;
}

void request::set_local_endpoint(boost::asio::ip::tcp::socket& socket)
{
	m_local_address = socket.local_endpoint().address().to_string();
	m_local_port = socket.local_endpoint().port();
}

float request::get_accept(const char* type) const
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
	
	for (const header& h: m_headers)
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

// m_request.http_version_minor >= 1 and not m_request.close

bool request::keep_alive() const
{
	return get_version() >= std::make_tuple(1, 1) and
		iequals(get_header("Connection"), "keep-alive");
}

void request::set_header(const char* name, const std::string& value)
{
	bool replaced = false;

	for (header& h: m_headers)
	{
		if (not iequals(h.name, name))
			continue;
		
		h.value = value;
		replaced = true;
		break;
    }

	if (not replaced)
		m_headers.push_back({ name, value });
}

std::string request::get_header(const char* name) const
{
	std::string result;

	for (const header& h: m_headers)
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
	m_headers.erase(
		remove_if(m_headers.begin(), m_headers.end(),
			[name](const header& h) -> bool { return h.name == name; }),
		m_headers.end());
}

std::pair<std::string,bool> get_urlencoded_parameter(const std::string& s, const char* name)
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
		tie(result, found) = get_urlencoded_parameter(m_payload, name);
		if (found)
			return std::make_tuple(result, true);
	}

	uri uri(m_uri);
	auto query = uri.get_query();

	if (not query.empty())
	{
		tie(result, found) = get_urlencoded_parameter(query, name);
		if (found)
			return std::make_tuple(result, true);
	}

	if (ba::starts_with(contentType, "application/json"))
	{
		try
		{
			json::element e;
			json::parse_json(m_payload, e);
			if (e.is_object() and e.contains(name))
			{
				result = e.at(name).as<std::string>();
				found = true;
			}
		}
		catch (const std::exception& ex)
		{
		}
	}
	else if (ba::starts_with(contentType, "multipart/form-data"))
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
			
			for (i = 0; i <= m_payload.length(); ++i)
			{
				if (i < m_payload.length() and m_payload[i] != '\r' and m_payload[i] != '\n')
					continue;

				// we have found a 'line' at [l, i)
				if (m_payload.compare(l, 2, "--") == 0 and
					m_payload.compare(l + 2, boundary.length(), boundary) == 0)
				{
					// if we're in the content state or if this is the last line
					if (state == CONTENT or m_payload.compare(l + 2 + boundary.length(), 2, "--") == 0)
					{
						if (r > 0)
						{
							auto n = l - r;
							if (n >= 1 and m_payload[r + n - 1] == '\n')
								--n;
							if (n >= 1 and m_payload[r + n - 1] == '\r')
								--n;

							result.assign(m_payload, r, n);
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
							if (m_payload[i] == '\r' and m_payload[i + 1] == '\n')
								r = i + 2;
						}
						else
							state = SKIP;
					}
					else if (std::regex_match(m_payload.begin() + l, m_payload.begin() + i, m, rx))
						contentName = m[1].str();
				}
				
				if (m_payload[i] == '\r' and m_payload[i + 1] == '\n')
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
	
	if (m_method == "POST")
	{
		std::string contentType = get_header("Content-Type");
		
		if (ba::starts_with(contentType, "application/x-www-form-urlencoded"))
			ps = m_payload;
	}
	else if (m_method == "GET" or m_method == "PUT")
	{
		uri uri(m_uri);
		ps = uri.get_query();
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
			std::swap(param, ps);
		
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

	// for (auto m : { "POST", "GET" })
	// {
	// 	std::string ps;

	// 	// skip m_payload unless this is a POST

	// 	switch (m)
	// 	{
	// 		case "POST":
	// 			if (m_method == m)
	// 			{
	// 				std::string contentType = get_header("Content-Type");
					
	// 				if (ba::starts_with(contentType, "application/x-www-form-urlencoded"))
	// 					ps = m_payload;
	// 			}
	// 			break;
			
	// 		case "GET":
	// 		{
	// 			std::string::size_type d = m_uri.find('?');
	// 			if (d != std::string::npos)
	// 				ps = m_uri.substr(d + 1);
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

struct file_param_parser
{
	file_param_parser(const request& req, const std::string& payload, const char* name);

	file_param next();

	const request& m_req;
	const std::string m_name;
	const std::string& m_payload;
	std::string m_boundary;
	static const std::regex k_rx_disp, k_rx_cont;
	enum { START, HEADER, CONTENT, SKIP } m_state = SKIP;
	std::string::size_type m_i = 0;
};

const std::regex file_param_parser::k_rx_disp(R"x(content-disposition:\s*form-data(;.+))x", std::regex::icase);
const std::regex file_param_parser::k_rx_cont(R"x(content-type:\s*(\S+/[^;]+)(;.*)?)x", std::regex::icase);

file_param_parser::file_param_parser(const request& req, const std::string& payload, const char* name)
	: m_req(req), m_name(name), m_payload(payload)
{
	std::string contentType = m_req.get_header("Content-Type");

	if (ba::starts_with(contentType, "multipart/form-data"))
	{
		std::string::size_type b = contentType.find("boundary=");
		if (b != std::string::npos)
			m_boundary = contentType.substr(b + strlen("boundary="));
	}
}

file_param file_param_parser::next()
{
	if (m_boundary.empty())
		return {};

	std::string contentName;
	std::smatch m;
	
	std::string::size_type r = 0, l = 0;
	file_param result = {};
	bool found = false;

	for (; m_i <= m_payload.length(); ++m_i)
	{
		if (m_i < m_payload.length() and m_payload[m_i] != '\r' and m_payload[m_i] != '\n')
			continue;

		// we have found a 'line' at [l, i)
		if (m_payload.compare(l, 2, "--") == 0 and
			m_payload.compare(l + 2, m_boundary.length(), m_boundary) == 0)
		{
			// if we're in the content state or if this is the last line
			if (m_state == CONTENT or m_payload.compare(l + 2 + m_boundary.length(), 2, "--") == 0)
			{
				if (r > 0)
				{
					auto n = l - r;
					if (n >= 1 and m_payload[r + n - 1] == '\n')
						--n;
					if (n >= 1 and m_payload[r + n - 1] == '\r')
						--n;

					result.data = m_payload.data() + r;
					result.length = n;
				}
				
				m_state = HEADER;
				break;
			}
			
			// Not the last, so it must be a separator and we're now in the Header part
			m_state = HEADER;
		}
		else if (m_state == HEADER)
		{
			if (l == m_i)	// empty line
			{
				if (contentName == m_name)
				{
					m_state = CONTENT;
					found = true;

					r = m_i + 1;
					if (m_payload[m_i] == '\r' and m_payload[m_i + 1] == '\n')
						r = m_i + 2;
				}
				else
				{
					result = {};
					m_state = SKIP;
				}
			}
			else if (std::regex_match(m_payload.begin() + l, m_payload.begin() + m_i, m, k_rx_disp))
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
			else if (std::regex_match(m_payload.begin() + l, m_payload.begin() + m_i, m, k_rx_cont))
			{
				result.mimetype = m[1].str();
				if (ba::starts_with(result.mimetype, "multipart/"))
					throw std::runtime_error("multipart file uploads are not supported");
			}
		}
		
		if (m_payload[m_i] == '\r' and m_payload[m_i + 1] == '\n')
			++m_i;

		l = m_i + 1;
	}
	
	if (not found)
		result = {};

	return result;	
}

file_param request::get_file_parameter(const char* name) const
{
	file_param_parser fpp(*this, m_payload, name);
	return fpp.next();
}

std::vector<file_param> request::get_file_parameters(const char* name) const
{
	file_param_parser fpp(*this, m_payload, name);

	std::vector<file_param> result;
	for (;;)
	{
		auto fp = fpp.next();
		if (not fp)
			break;
		result.push_back(fp);
	}
	
	return result;
}

std::string request::get_cookie(const char* name) const
{
	for (const header& h : m_headers)
	{
		if (h.name != "Cookie")
			continue;

		std::vector<std::string> rawCookies;
		ba::split(rawCookies, h.value, ba::is_any_of(";"));

		for (std::string& cookie : rawCookies)
		{
			trim(cookie);

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
	for (auto& h: m_headers)
	{
		if (not iequals(h.name, "Cookie"))
			continue;
		
		std::vector<std::string> rawCookies;
		ba::split(rawCookies, h.value, ba::is_any_of(";"));

		for (std::string& cookie : rawCookies)
		{
			trim(cookie);

			auto d = cookie.find('=');
			if (d == std::string::npos)
				continue;
			
			cookies[cookie.substr(0, d)] = cookie.substr(d + 1);
		}
	}

	m_headers.erase(
		std::remove_if(m_headers.begin(), m_headers.end(), [](header& h) { return iequals(h.name, "Cookie"); }),
		m_headers.end());

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

// std::string request::get_request_line() const
// {
// 	return to_string(m_method) + std::string{' '} + m_uri + " HTTP/" + std::to_string(m_http_version_major) + '.' + std::to_string(m_http_version_minor);
// }

void request::set_header(const std::string& name, const std::string& value)
{
	bool set = false;
	for (auto& h: m_headers)
	{
		if (iequals(h.name, name))
		{
			h.value = value;
			set = true;
			break;
		}
	}

	if (not set)
		m_headers.push_back({ name, value });
}

namespace
{
const char
		kNameValueSeparator[] = { ':', ' ' },
		kCRLF[] = { '\r', '\n' },
		kSpace[] = { ' ' },
		kHTTPSlash[] = { ' ', 'H', 'T', 'T', 'P', '/' }
		;
}

std::vector<boost::asio::const_buffer> request::to_buffers() const
{
	std::vector<boost::asio::const_buffer> result;

	// m_request_line = get_request_line();

	// m_request_line = m_method + ' ' + m_uri + " HTTP/" + std::to_string(m_http_version_major) + '.' + std::to_string(m_http_version_minor);

	result.push_back(boost::asio::buffer(m_method));
	result.push_back(boost::asio::buffer(kSpace));
	result.push_back(boost::asio::buffer(m_uri));
	result.push_back(boost::asio::buffer(kHTTPSlash));
	result.push_back(boost::asio::buffer(m_version));
	
	result.push_back(boost::asio::buffer(kCRLF));
	
	for (const header& h: m_headers)
	{
		result.push_back(boost::asio::buffer(h.name));
		result.push_back(boost::asio::buffer(kNameValueSeparator));
		result.push_back(boost::asio::buffer(h.value));
		result.push_back(boost::asio::buffer(kCRLF));
	}

	result.push_back(boost::asio::buffer(kCRLF));
	result.push_back(boost::asio::buffer(m_payload));

	return result;
}

std::ostream& operator<<(std::ostream& io, const request& req)
{
	std::vector<boost::asio::const_buffer> buffers = req.to_buffers();

	for (auto& b: buffers)
		io.write(static_cast<const char*>(b.data()), b.size());

	return io;
}

} // zeep::http
