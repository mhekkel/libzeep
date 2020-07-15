// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::reply class encapsulating a valid HTTP reply

#include <zeep/config.hpp>

#include <boost/asio/buffer.hpp>

#include <zeep/http/header.hpp>
#include <zeep/xml/document.hpp>
#include <zeep/json/element.hpp>

namespace zeep::http
{

/// Various predefined HTTP status codes

enum status_type
{
    cont =                  100,
    ok =                    200,
    created =               201,
    accepted =              202,
    no_content =            204,
    multiple_choices =      300,
    moved_permanently =     301,
    moved_temporarily =     302,
    not_modified =          304,
    bad_request =           400,
    unauthorized =          401,
    forbidden =             403,
    not_found =             404,
	method_not_allowed =	405,
	proxy_authentication_required =	407,
    internal_server_error = 500,
    not_implemented =       501,
    bad_gateway =           502,
    service_unavailable =   503
};

/// Return the error string for the status_type
std::string get_status_text(status_type status);

/// Return the string describing the status_type in more detail
std::string get_status_description(status_type status);

/// \brief the class containing all to generate a HTTP reply
///
/// Create a HTTP reply, should be either HTTP 1.0 or 1.1

class reply
{
  public:
	using cookie_directive = header;

	/// Create a reply, default is HTTP 1.0. Use 1.1 if you want to use keep alive e.g.

	reply(int major_version = 1, int minor_version = 0);

	reply(status_type status, std::tuple<int,int> version,
		std::vector<header>&& headers, std::string&& payload);

	reply(const reply& rhs);
	~reply();
	reply& operator=(const reply &);

	void clear();

	void set_version(int version_major, int version_minor);
	void set_version(std::tuple<int,int> version);

	/// Add a header with name \a name and value \a value
	void set_header(const std::string& name,
									const std::string& value);

	std::string get_header(const std::string& name);

	/// Set a cookie
	void set_cookie(const char* name, const std::string& value, std::initializer_list<cookie_directive> directives = {});
	std::string get_cookie(const char* name) const;

	/// If the reply contains Connection: keep-alive
	bool keep_alive() const;

	std::string get_content_type() const;
	void set_content_type(
			const std::string& type); ///< Set the Content-Type header

	/// Set the content and the content-type header
	void set_content(xml::document& doc);

	/// Set the content and the content-type header
	void set_content(const xml::element& data);

	/// Set the content and the content-type header based on JSON data
	void set_content(const json::element& json);

	/// Set the content and the content-type header
	void set_content(const std::string& data,
									 const std::string& contentType);

	/// Set the content by copying \a data and the content-type header
	void set_content(const char* data, size_t size, const std::string& contentType);

	/// To send a stream of data, with unknown size (using chunked transfer).
	/// reply takes ownership of \a data and deletes it when done.
	void set_content(std::istream* data,
									 const std::string& contentType);

	/// return the content, only useful if the content was set with
	/// some constant string data. 
	std::string get_content() const
	{
		return m_content;
	}

	/// return the content of the reply as an array of boost::asio::const_buffer objects
	std::vector<boost::asio::const_buffer> to_buffers() const;

	/// for istream data, if the returned buffer array is empty, the data is done
	std::vector<boost::asio::const_buffer> data_to_buffers();

	/// Create a standard reply based on a HTTP status code
	static reply stock_reply(status_type inStatus);
	static reply stock_reply(status_type inStatus, const std::string& info);

	/// Create a standard redirect reply with the specified \a location
	static reply redirect(const std::string& location);

	void set_status(status_type status) { m_status = status; }
	status_type get_status() const { return m_status; }

	/// return the size of the reply, only correct if the reply is fully memory based (no streams)
	size_t size() const;

	/// for debugging
	friend std::ostream& operator<<(std::ostream& os, const reply& rep);

  private:
	friend class reply_parser;

	status_type m_status;
	int m_version_major, m_version_minor;
	std::vector<header> m_headers;
	std::string m_content;
	std::istream* m_data;
	std::vector<char> m_buffer;

	// this status line is only here to have a sensible location to store it
	mutable std::string m_status_line;
};

}
