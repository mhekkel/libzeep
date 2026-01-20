// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2026
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::reply class encapsulating a valid HTTP reply

#include "zeep/el/object.hpp"
#include "zeep/http/asio.hpp"
#include "zeep/http/header.hpp"
#include "zeep/http/status.hpp"
#include "zeep/uri.hpp"

#include <zeem.hpp>

namespace zeep::http
{

/// the class containing everything you need to generate a HTTP reply
///
/// Create a HTTP reply, should be either HTTP 1.0 or 1.1

class reply
{
  public:
	using cookie_directive = header;

	/// Create a reply, default is HTTP 1.0. Use 1.1 if you want to use keep alive e.g.
	reply(status_type status = status_type::ok, std::tuple<int, int> version = { 1, 0 });

	/// Create a reply with \a status, \a version, \a headers and a \a payload
	reply(status_type status, std::tuple<int, int> version,
		std::vector<header> &&headers, std::string &&payload);

	reply(const reply &rhs);

	reply(reply &&rhs) noexcept
	{
		swap(*this, rhs);
	}

	~reply() = default;

	reply &operator=(reply rhs)
	{
		swap(*this, rhs);
		return *this;
	}

	/// Swap two replies
	friend void swap(reply &a, reply &b) noexcept
	{
		std::swap(a.m_status, b.m_status);
		std::swap(a.m_version_minor, b.m_version_minor);
		std::swap(a.m_headers, b.m_headers);
		std::swap(a.m_data, b.m_data);
		std::swap(a.m_buffer, b.m_buffer);
		std::swap(a.m_content, b.m_content);
		std::swap(a.m_chunked, b.m_chunked);
	}

	/// Simple way to check if a reply is valid
	explicit operator bool() const { return m_status == status_type::ok; }

	/// Set the version to \a version_major . \a version_minor
	void set_version(int version_major, int version_minor);

	/// Set version to \a version
	void set_version(std::tuple<int, int> version)
	{
		set_version(std::get<0>(version), std::get<1>(version));
	}

	/// Add a header with name \a name and value \a value
	void set_header(std::string name, std::string value);

	/// Return the value of the header with name \a name
	[[nodiscard]] std::string get_header(std::string_view name) const;

	/// Remove the header with name \a name from the list of headers
	void remove_header(std::string_view name);

	/// Set a cookie
	void set_cookie(std::string_view name, const std::string &value, std::initializer_list<cookie_directive> directives = {});

	/// Set a header to delete the \a name cookie
	void set_delete_cookie(std::string_view name);

	/// Get a cookie
	[[nodiscard]] std::string get_cookie(std::string_view name) const;

	/// Return the value of the header named content-type
	[[nodiscard]] std::string get_content_type() const
	{
		return get_header("Content-Type");
	}

	/// Set the Content-Type header to \a type
	void set_content_type(std::string type)
	{
		set_header("Content-Type", std::move(type));
	}

	/// Set the content and the content-type header depending on the content of doc (might be xhtml)
	void set_content(zeem::document &doc);

	/// Set the content and the content-type header to text/xml
	void set_content(const zeem::element &data);

	/// Set the content and the content-type header based on JSON data
	void set_content(const el::object &data);

	/// Set the content and the content-type header
	void set_content(std::string data, std::string contentType);

	/// Set the content by copying \a data and the content-type header
	void set_content(const char *data, size_t size, std::string contentType);

	/// To send a stream of data, with unknown size (using chunked transfer).
	/// reply takes ownership of \a data and deletes it when done.
	void set_content(std::istream *data, std::string contentType);

	/// return the content, only useful if the content was set with
	/// some constant string data.
	[[nodiscard]] const std::string &get_content() const
	{
		return m_content;
	}

	/// return the content of the reply as an array of std::string_view objects
	[[nodiscard]] std::vector<std::string_view> to_buffers() const;

	/// for istream data, if the returned buffer array is empty, the data is done
	[[nodiscard]] std::vector<std::string_view> data_to_buffers();

	/// Create a standard reply based on a HTTP status code
	static reply stock_reply(status_type inStatus);
	static reply stock_reply(status_type inStatus, const std::string &info);

	/// Create a standard redirect reply with the specified \a location
	static reply redirect(const uri &location);
	static reply redirect(const uri &location, status_type status);

	void set_status(status_type status) { m_status = status; }
	[[nodiscard]] status_type get_status() const { return m_status; }

	/// return the size of the reply, only correct if the reply is fully memory based (no streams)
	[[nodiscard]] size_t size() const;

	/// Return true if the content will be sent chunked encoded
	[[nodiscard]] bool get_chunked() const { return m_chunked; }

	/// for debugging
	friend std::ostream &operator<<(std::ostream &os, const reply &rep);

  private:
	friend class reply_parser;

	status_type m_status{ status_type::bad_request };
	int m_version_major = 0, m_version_minor = 0;
	std::vector<header> m_headers;
	std::shared_ptr<std::istream> m_data;
	std::vector<char> m_buffer;
	std::string m_content;
	bool m_chunked = false;
};

} // namespace zeep::http
