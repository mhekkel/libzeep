//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_REPLY_HPP
#define SOAP_HTTP_REPLY_HPP

#include <vector>
#include <zeep/http/header.hpp>
#include <boost/asio/buffer.hpp>
#include <zeep/xml/document.hpp>

namespace zeep { namespace http {

enum status_type
{
	cont =					100,
    ok =					200,
    created =				201,
    accepted =				202,
    no_content =			204,
    multiple_choices =		300,
    moved_permanently =		301,
    moved_temporarily =		302,
    not_modified =			304,
    bad_request =			400,
    unauthorized =			401,
    forbidden =				403,
    not_found =				404,
    internal_server_error =	500,
    not_implemented =		501,
    bad_gateway =			502,
    service_unavailable =	503
};

class reply
{
  public:
						reply(int version_major = 1, int version_minor = 0);
						reply(const reply&);
	reply&				operator=(const reply&);

	void				set_version(int version_major, int version_minor);

	void				set_header(const std::string& name,
							const std::string& value);

	std::string			get_content_type() const;
	void				set_content_type(
							const std::string& type);
	
	void				set_content(xml::document& doc);
	void				set_content(xml::element* data);
	void				set_content(const std::string& data,
							const std::string& contentType);
	// to send a stream of data, with unknown size (using chunked transfer):
	void				set_content(std::istream* data,
							const std::string& contentType);

	void				to_buffers(std::vector<boost::asio::const_buffer>& buffers);
	
	// for istream data, continue to until data_to_buffers returns false
	bool				data_to_buffers(std::vector<boost::asio::const_buffer>& buffers);

	std::string			get_as_text();
	std::size_t			get_size() const;
	
	static reply		stock_reply(status_type inStatus);
	static reply		redirect(const std::string& location);
	
	status_type			get_status() const						{ return m_status; }

  private:
	int					m_version_major, m_version_minor;
	status_type			m_status;
	std::string			m_status_line;
	std::vector<header>	m_headers;
	std::string			m_content;
	std::unique_ptr<std::istream>
						m_data;
	std::vector<char>	m_buffer;
};

}
}

#endif
