// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2025
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::request class encapsulating a valid HTTP request

#include <zeep/config.hpp>

#include <zeep/el/object.hpp>
#include <zeep/http/asio.hpp>
#include <zeep/http/header.hpp>
#include <zeep/http/uri.hpp>

#include <mxml/serialize.hpp>

#include <chrono>
#include <cstdint>
#include <istream>

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
	const char *data;
	size_t length;

	explicit operator bool() const
	{
		return data != nullptr;
	}
};

// Some type traits to detect arrays of file_params.
// Should eventually be made more generic for all request parameters

template <typename T, typename = void>
struct is_file_param_array_type : std::false_type
{
};

template <typename T>
struct is_file_param_array_type<T,
	std::enable_if_t<
		mxml::detail::is_detected_v<mxml::value_type_t, T> and
		mxml::detail::is_detected_v<mxml::iterator_t, T> and
		not mxml::detail::is_detected_v<mxml::std_string_npos_t, T>>>
{
	static constexpr bool value = std::is_same_v<typename T::value_type, file_param>;
};

template <typename T>
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

	using param = header; // alias name
	using cookie_directive = header;

	request(std::string method, uri uri_, std::tuple<int, int> version = { 1, 0 },
		std::vector<header> &&headers = {}, std::string &&payload = {});

	request(const request &req);

	request(request &&rhs) noexcept
	{
		swap(rhs);
	}

	request &operator=(request rhs) noexcept
	{
		swap(rhs);
		return *this;
	}

	void swap(request &rhs) noexcept;

	/// \brief Fetch the local address from the connected socket
	void set_local_endpoint(asio_ns::ip::tcp::socket &socket);
	std::tuple<std::string, uint16_t> get_local_endpoint() const { return { m_local_address, m_local_port }; }

	/// \brief Get the HTTP version requested
	std::tuple<int, int> get_version() const { return { m_version[0] - '0', m_version[2] - '0' }; }

	/// \brief Set the METHOD type (POST, GET, etc)
	void set_method(std::string method) { m_method = std::move(method); }

	/// \brief Return the METHOD type (POST, GET, etc)
	const std::string &get_method() const { return m_method; }

	/// \brief Return the original URI as requested
	const uri &get_uri() const { return m_uri; }

	/// \brief Set the URI
	void set_uri(const uri &uri_) { m_uri = uri_; }

	/// \brief Get the address of the connecting remote
	const std::string &get_remote_address() const { return m_remote_address; }

	/// \brief Get the entire request line (convenience method)
	std::string get_request_line() const
	{
		return m_method + ' ' + m_uri.string() + " HTTP/" + std::string(m_version.data(), m_version.data() + m_version.size());
	}

	/// \brief Return the payload
	const std::string &get_payload() const { return m_payload; }

	/// \brief Set the payload
	void set_payload(std::string payload) { m_payload = std::move(payload); }

	/// \brief Return the time at which this request was received
	std::chrono::system_clock::time_point get_timestamp() const { return m_timestamp; }

	/// \brief Return the value in the Accept header for type
	float get_accept(std::string_view type) const;

	/// \brief Check for Connection: keep-alive header
	bool keep_alive() const;

	/// \brief Set or replace a named header
	void set_header(std::string name, std::string value);

	/// \brief Return the list of headers
	auto get_headers() const { return m_headers; }

	/// \brief Return the named header
	std::string get_header(std::string_view name) const;

	/// \brief Remove this header from the list of headers
	void remove_header(std::string_view name);

	/// \brief Get the credentials. This is filled in if the request was validated
	el::object get_credentials() const { return m_credentials; }

	/// \brief Set the credentials for the request
	void set_credentials(el::object &&credentials) { m_credentials = std::move(credentials); }

	/// \brief Return the named parameter
	///
	/// Fetch parameters from a request, either from the URL or from the payload in case
	/// the request contains a url-encoded or multi-part content-type header
	std::optional<std::string> get_parameter(std::string_view name) const;

	/// \brief Return the value of the parameter named \a name or the \a defaultValue if this parameter was not found
	std::string get_parameter(std::string_view name, std::string defaultValue) const
	{
		return get_parameter(name).value_or(defaultValue);
	}

	/// \brief Return a std::multimap of name/value pairs for all parameters
	std::multimap<std::string, std::string> get_parameters() const;

	/// \brief Return the info for a file parameter with name \a name
	///
	file_param get_file_parameter(std::string name) const;

	/// \brief Return the info for all file parameters with name \a name
	///
	std::vector<file_param> get_file_parameters(std::string name) const;

	/// \brief Return the value of HTTP Cookie with name \a name
	std::string get_cookie(std::string_view name) const;

	/// \brief Set the value of HTTP Cookie with name \a name to \a value
	void set_cookie(std::string name, std::string value);

	/// \brief Return the content of this request in a sequence of const_buffers
	///
	/// Can be used in code that sends HTTP requests
	std::vector<asio_ns::const_buffer> to_buffers() const;

	/// \brief Return the Accept-Language header value in the request as a std::locale object
	std::locale get_locale() const;

	/// \brief For debugging purposes
	friend std::ostream &operator<<(std::ostream &io, const request &req);

	/// \brief suppose we want to construct requests...
	void set_content(std::string text, std::string contentType)
	{
		set_header("content-type", std::move(contentType));
		set_header("content-length", std::to_string(text.length()));
		m_payload = std::move(text);
	}

  private:
	void set_remote_address(std::string address)
	{
		m_remote_address = std::move(address);
	}

	std::string m_local_address; ///< Local endpoint address
	uint16_t m_local_port = 80;  ///< Local endpoint port

	std::string m_method = "UNDEFINED"; ///< POST, GET, etc.
	uri m_uri;                          ///< The uri as requested
	std::array<char, 3> m_version;      ///< The version string
	std::vector<header> m_headers;      ///< A list with zeep::http::header values
	std::string m_payload;              ///< For POST requests
	bool m_close = false;               ///< Whether 'Connection: close' was specified

	std::chrono::system_clock::time_point m_timestamp = std::chrono::system_clock::now();
	el::object m_credentials; ///< The credentials as found in the validated access-token

	std::string m_remote_address; ///< Address of connecting client
};

} // namespace zeep::http
