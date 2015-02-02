//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_REPLY_HPP
#define SOAP_HTTP_REPLY_HPP

#include <vector>
#include <memory>

#include <boost/asio/buffer.hpp>

#include <zeep/http/header.hpp>
#include <zeep/xml/document.hpp>

namespace zeep { namespace http {

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

class reply
{
  public:
	/// Create a reply, default is HTTP 1.0. Use 1.1 if you want to use keep alive e.g.

						reply(int version_major = 1, int version_minor = 0);
						reply(const reply& rhs);
						~reply();
	reply&				operator=(const reply&);
	
	void				clear();

	void				set_version(int version_major, int version_minor);

	/// Add a header with name \a name and value \a value
	void				set_header(const std::string& name,
							const std::string& value);

	/// If the reply contains Connection: keep-alive
	bool				keep_alive() const;

	std::string			get_content_type() const;
	void				set_content_type(
							const std::string& type);	///< Set the Content-Type header
	
	/// Set the content and the content-type header
	void				set_content(xml::document& doc);

	/// Set the content and the content-type header
	void				set_content(xml::element* data);

	/// Set the content and the content-type header
	void				set_content(const std::string& data,
							const std::string& contentType);

	/// To send a stream of data, with unknown size (using chunked transfer).
	/// reply takes ownership of \a data and deletes it when done.
	void				set_content(std::istream* data,
							const std::string& contentType);

	void				to_buffers(std::vector<boost::asio::const_buffer>& buffers);
	
	/// for istream data, continues until data_to_buffers returns false
	bool				data_to_buffers(std::vector<boost::asio::const_buffer>& buffers);

	/// For debugging purposes
	std::string			get_as_text();
	std::size_t			get_size() const;

	/// Create a standard reply based on a HTTP status code	
	static reply		stock_reply(status_type inStatus);
	static reply		stock_reply(status_type inStatus, const std::string& info);
	
	/// Create a standard redirect reply with the specified \a location
	static reply		redirect(const std::string& location);
	
	void				set_status(status_type status)			{ m_status = status; }
	status_type			get_status() const						{ return m_status; }

	void				debug(std::ostream& os) const;

	friend std::ostream& operator<<(std::ostream&, reply&);

  private:
	friend class reply_parser;

	int					m_version_major, m_version_minor;
	status_type			m_status;
	std::string			m_status_line;
	std::vector<header>	m_headers;
	std::string			m_content;
	std::istream*		m_data;
	std::vector<char>	m_buffer;
};

std::ostream& operator<<(std::ostream&, reply&);

}
}

#endif
