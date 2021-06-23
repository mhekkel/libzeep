// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::request class encapsulating a valid HTTP request

#include <zeep/config.hpp>

#include <istream>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <zeep/http/header.hpp>
#include <zeep/json/element.hpp>

namespace zeep::http
{

// --------------------------------------------------------------------
// TODO: one day this should be able to work with temporary files

/// \brief container for file parameter information
///
/// Files submitted using multipart/form-data contain a filename and
/// mimetype that might be interesting to the client.
struct file_param
{
	std::string filename;
	std::string mimetype;
	const char* data;
	size_t length;

	explicit operator bool() const
	{
		return data != nullptr;
	}
};

// Some type traits to detect arrays of file_params.
// Should eventually be made more generic for all request parameters

template<typename T, typename = void>
struct is_file_param_array_type : std::false_type {};

template<typename T>
struct is_file_param_array_type<T,
	std::enable_if_t<
		std::experimental::is_detected_v<value_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		not std::experimental::is_detected_v<std_string_npos_t, T>>>
{
	static constexpr bool value = std::is_same_v<typename T::value_type,file_param>;
};

template<typename T>
inline constexpr bool is_file_param_array_type_v = is_file_param_array_type<T>::value;

// --------------------------------------------------------------------

/// request contains the parsed original HTTP request as received
/// by the server.

class request
{
  public:

	friend class message_parser;
	friend class request_parser;
	friend class basic_server;

	using param = header;	// alias name
	using cookie_directive = header;	

	request(const std::string& method, const std::string& uri, std::tuple<int,int> version = { 1, 0 },
		std::vector<header>&& headers = {}, std::string&& payload = {});

	request(const request& req);
	
	request& operator=(const request& rhs);
	
	/// \brief Fetch the local address from the connected socket
	void set_local_endpoint(boost::asio::ip::tcp::socket& socket);
	std::tuple<std::string,uint16_t> get_local_endpoint() const				{ return { m_local_address, m_local_port }; }

	/// \brief Get the HTTP version requested
	std::tuple<int,int> get_version() const									{ return { m_version[0] - '0', m_version[2] - '0' }; }

	/// \brief Set the METHOD type (POST, GET, etc)
	void set_method(const std::string& method)								{ m_method = method; }

	/// \brief Return the METHOD type (POST, GET, etc)
	const std::string& get_method() const									{ return m_method; }

	/// \brief Return the original URI as requested
	std::string get_uri() const												{ return m_uri; }

	/// \brief Set the URI
	void set_uri(const std::string& uri)									{ m_uri = uri; }

	/// \brief Get the address of the connecting remote
	std::string get_remote_address() const									{ return m_remote_address; }

	/// \brief Get the entire request line (convenience method)
	std::string get_request_line() const
	{
		return get_method() + ' ' + get_uri() + " HTTP/" + std::string(m_version, m_version + 3);
	}

	/// \brief Return the payload
	const std::string& get_payload() const									{ return m_payload; }

	/// \brief Set the payload
	void set_payload(const std::string& payload)							{ m_payload = payload; }

	/// \brief Return the time at which this request was received
	boost::posix_time::ptime get_timestamp() const { return m_timestamp; }

	/// \brief Return the value in the Accept header for type
	float get_accept(const char* type) const;

	/// \brief Check for Connection: keep-alive header
	bool keep_alive() const;

	/// \brief Set or replace a named header
	void set_header(const char* name, const std::string& value);

	/// \brief Return the list of headers
	auto get_headers() const												{ return m_headers; }

	/// \brief Return the named header
	std::string get_header(const char* name) const;

	/// \brief Remove this header from the list of headers
	void remove_header(const char* name);

	/// \brief Get the credentials. This is filled in if the request was validated
	json::element get_credentials() const				{ return m_credentials; }

	/// \brief Set the credentials for the request
	void set_credentials(json::element&& credentials)	{ m_credentials = std::move(credentials); }

	/// \brief Return the named parameter
	///
	/// Fetch parameters from a request, either from the URL or from the payload in case
	/// the request contains a url-encoded or multi-part content-type header
	std::string get_parameter(const char* name) const
	{
		std::string result;
		std::tie(result, std::ignore) = get_parameter_ex(name);
		return result;
	}

	/// \brief Return the value of the parameter named \a name or the \a defaultValue if this parameter was not found
	std::string get_parameter(const char* name, const std::string& defaultValue) const
	{
		std::string result = get_parameter(name);
		if (result.empty())
			result = defaultValue;
		return result;
	}

	/// \brief Return the value of the parameter named \a name or the \a defaultValue if this parameter was not found
	template<typename T, typename std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	T get_parameter(const char* name, const T& defaultValue) const
	{
		return static_cast<T>(std::stod(get_parameter(name, std::to_string(defaultValue))));
	}

	/// \brief Return the value of the parameter named \a name or the \a defaultValue if this parameter was not found
	template<typename T, typename std::enable_if_t<std::is_integral_v<T> and not std::is_same_v<T,bool>, int> = 0>
	T get_parameter(const char* name, const T& defaultValue) const
	{
		return static_cast<T>(std::stol(get_parameter(name, std::to_string(defaultValue))));
	}

	/// \brief Return the value of the parameter named \a name or the \a defaultValue if this parameter was not found
	template<typename T, typename std::enable_if_t<std::is_same_v<T,bool>, int> = 0>
	T get_parameter(const char* name, const T& defaultValue) const
	{
		auto v = get_parameter(name, std::to_string(defaultValue));
		return v == "true" or v == "1";
	}

	/// \brief Return a std::multimap of name/value pairs for all parameters
	std::multimap<std::string,std::string> get_parameters() const;

	/// \brief Return the info for a file parameter with name \a name
	///
	file_param get_file_parameter(const char* name) const;

	/// \brief Return the info for all file parameters with name \a name
	///
	std::vector<file_param> get_file_parameters(const char* name) const;

	/// \brief Return whether the named parameter is present in the request
	bool has_parameter(const char* name) const
	{
		bool result;
		tie(std::ignore, result) = get_parameter_ex(name);
		return result;
	}

	/// \brief Return the value of HTTP Cookie with name \a name
	std::string get_cookie(const char* name) const;

	/// \brief Return the value of HTTP Cookie with name \a name
	std::string get_cookie(const std::string& name) const
	{
		return get_cookie(name.c_str());
	}

	/// \brief Set the value of HTTP Cookie with name \a name to \a value
	void set_cookie(const char* name, const std::string& value);

	/// \brief Return the content of this request in a sequence of const_buffers
	///
	/// Can be used in code that sends HTTP requests
	std::vector<boost::asio::const_buffer> to_buffers() const;

	/// \brief Return the Accept-Language header value in the request as a std::locale object
	std::locale& get_locale() const;

	/// \brief For debugging purposes
	friend std::ostream& operator<<(std::ostream& io, const request& req);

	/// \brief suppose we want to construct requests...
	void set_content(const std::string& text, const std::string& contentType)
	{
		set_header("content-type", contentType);
		m_payload = text;
		set_header("content-length", std::to_string(text.length()));
	}

	/// \brief set a header
	void set_header(const std::string& name, const std::string& value);

	/// \brief Return value and flag indicating the existence of a parameter named \a name
	std::tuple<std::string,bool> get_parameter_ex(const char* name) const;

  private:

	void set_remote_address(const std::string& address)
	{
		m_remote_address = address;
	}

	std::string m_local_address;					///< Local endpoint address
	uint16_t m_local_port = 80;						///< Local endpoint port

	std::string m_method = "UNDEFINED";				///< POST, GET, etc.
	std::string m_uri;								///< The uri as requested
	char m_version[3];								///< The version string
	std::vector<header> m_headers;					///< A list with zeep::http::header values
	std::string m_payload;  						///< For POST requests
	bool m_close = false;  							///< Whether 'Connection: close' was specified

	boost::posix_time::ptime m_timestamp = boost::posix_time::second_clock::local_time();
	json::element m_credentials;					///< The credentials as found in the validated access-token

	std::string m_remote_address;					///< Address of connecting client

	mutable std::unique_ptr<std::locale> m_locale;
};

} // namespace zeep::http
