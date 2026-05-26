/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Maarten L. Hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This code is originally written for mini-ibs, a content management system

#include "zeep/http/client.hpp"
#include "zeep/http/message-parser.hpp"
#include "zeep/streambuf.hpp"
#include "zeep/unicode-support.hpp"

#include "revision.hpp"

#include <iostream>

namespace zeep::http
{

using asio_ns::ip::tcp;

template <typename SocketType>
class client_base
{
  public:
	using socket_type = SocketType;

	virtual ~client_base() = default;

	[[nodiscard]] bool done() const { return m_done; }
	reply get_reply() { return m_reply_parser.get_reply(); }

  protected:
	virtual socket_type &get_socket() = 0;

	explicit client_base(const zeep::uri &url)
		: m_req({ "GET", url })
		// , m_verbose(mcfp::config::instance().has("m_verbose"))
	{
	}

	void send_request()
	{
		std::vector<asio_ns::const_buffer> buffers;
		for (auto &buffer : m_req.to_buffers())
			buffers.emplace_back(buffer.data(), buffer.size());

		asio_ns::async_write(get_socket(),
			buffers,
			[this](const asio_system_ns::error_code &error, std::size_t /*length*/)
			{
				if (not error)
					receive_response();
				else if (m_verbose)
					std::clog << "Write failed: " << error.message() << "\n";
			});
	}

	void receive_response()
	{
		asio_ns::async_read(get_socket(),
			asio_ns::buffer(m_buffer),
			[this](const asio_system_ns::error_code &error, std::size_t length)
			{
				if (error and error != asio_ns::error::eof)
				{
					if (m_verbose)
						std::clog << "Read failed: " << error.message() << "\n";
					return;
				}

				zeep::char_streambuf sb(m_buffer.data(), length);

				auto r = m_reply_parser.parse(sb);
				if (r == true or error == asio_ns::error::eof)
					m_done = true;
				else
					receive_response();
			});
	}

	std::array<char, 4096> m_buffer{};
	const request m_req;
	bool m_done = false, m_verbose = false;
	reply_parser m_reply_parser;
};

class client : public client_base<tcp::socket>
{
  public:
	client(asio_ns::io_context &io_context,
		const tcp::resolver::results_type &endpoints,
		const std::string &req)
		: client_base(req)
		, m_socket(io_context)
	{
		connect(endpoints);
	}

  protected:
	socket_type &get_socket() override { return m_socket; }

  private:
	void connect(const tcp::resolver::results_type &endpoints)
	{
		asio_ns::async_connect(m_socket, endpoints,
			[this](const asio_system_ns::error_code &error,
				const tcp::endpoint & /*endpoint*/)
			{
				if (not error)
					send_request();
				else if (m_verbose)
					std::clog << "Connect failed: " << error.message() << "\n";
			});
	}

	tcp::socket m_socket;
};

class ssl_client : public client_base<asio_ns::ssl::stream<tcp::socket>>
{
  public:
	ssl_client(asio_ns::io_context &io_context,
		asio_ns::ssl::context &context,
		const tcp::resolver::results_type &endpoints,
		const std::string &req)
		: client_base(req)
		, m_socket(io_context, context)
	{
		m_socket.set_verify_mode(asio_ns::ssl::verify_peer);
		m_socket.set_verify_callback(
			[this](auto &&preverified, auto &&ctx)
			{ return verify_certificate(
				  std::forward<decltype(preverified)>(preverified),
				  std::forward<decltype(ctx)>(ctx)); });

		connect(endpoints);
	}

  protected:
	socket_type &get_socket() override { return m_socket; }

  private:
	bool verify_certificate(bool preverified,
		asio_ns::ssl::verify_context & /*ctx*/)
	{
		// // The verify callback can be used to check whether the certificate that is
		// // being presented is valid for the peer. For example, RFC 2818 describes
		// // the steps involved in doing this for HTTPS. Consult the OpenSSL
		// // documentation for more details. Note that the callback is called once
		// // for each certificate in the certificate chain, starting from the root
		// // certificate authority.

		// // In this example we will simply print the certificate's subject name.
		// char subject_name[256];
		// X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		// X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		// std::clog << "Verifying " << subject_name << "\n";

		return preverified;
	}

	void connect(const tcp::resolver::results_type &endpoints)
	{
		asio_ns::async_connect(m_socket.lowest_layer(), endpoints,
			[this](const asio_system_ns::error_code &error,
				const tcp::endpoint & /*endpoint*/)
			{
				if (not error)
					handshake();
				else if (m_verbose)
					std::clog << "Connect failed: " << error.message() << "\n";
			});
	}

	void handshake()
	{
		m_socket.async_handshake(asio_ns::ssl::stream_base::client,
			[this](const asio_system_ns::error_code &error)
			{
				if (not error)
					send_request();
				else if (m_verbose)
					std::clog << "Handshake failed: " << error.message() << "\n";
			});
	}

	asio_ns::ssl::stream<tcp::socket> m_socket;
};

reply send_request(request req)
{
	namespace ssl = asio_ns::ssl;
	using ssl_socket = ssl::stream<tcp::socket>;

	const zeep::uri &uri = req.get_uri();

	auto host = uri.get_host();
	auto port = uri.get_port();
	if (port == 0)
	{
		if (iequals(uri.get_scheme(), "http"))
			port = 80;
		else if (iequals(uri.get_scheme(), "https"))
			port = 443;
		else
			throw std::invalid_argument("Invalid scheme in uri for send_request");
	}

	asio_ns::io_context io_context;
	tcp::resolver resolver(io_context);
	tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

	// prepare a request
	req.set_header("Host", std::format("{}:{}", host, port));

	if (req.get_header("accept").empty())
		req.set_header("Accept", "*/*");

	if (req.get_header("user-agent").empty())
		req.set_header("User-Agent", std::format("{}/{}", klibzeepProjectName, klibzeepVersionNumber));

	std::vector<asio_ns::const_buffer> req_buffer;
	for (auto &buffer : req.to_buffers())
		req_buffer.emplace_back(buffer.data(), buffer.size());

	auto reader = [&, is_head = zeep::iequals(req.get_method(), "HEAD")](auto &socket)
	{
		reply result;
		reply_parser p;

		for (;;)
		{
			std::array<char, 4096> buf{};
			asio_system_ns::error_code error{};

			size_t len = socket.read_some(asio_ns::buffer(buf), error);

			zeep::char_streambuf sb(buf.data(), len);

			auto r = p.parse(sb);

			if (r == true or error == asio_ns::error::eof or len == 0 or (sb.in_avail() == 0 and is_head))
			{
				result = p.get_reply();
				break;
			}
			else if (error)
			{
				// if (mcfp::config::instance().has("verbose"))
					std::clog << error << '\n';
				break;
			}
		}

		return result;
	};

	if (uri.get_scheme() == "https")
	{
		asio_ns::ssl::context ctx(ssl::context::tls);

		ctx.set_default_verify_paths();
		ctx.set_options(ssl::context::default_workarounds);
		ctx.load_verify_file("/etc/ssl/certs/ca-certificates.crt");

		ssl_socket sock(io_context, ctx);
		asio_ns::connect(sock.lowest_layer(), endpoints);
		sock.lowest_layer().set_option(tcp::no_delay(true));

		(void)SSL_set_tlsext_host_name(sock.native_handle(), host.c_str());

		// Perform SSL handshake and verify the remote host's certificate.
		sock.set_verify_mode(ssl::verify_peer);
#if (BOOST_VERSION / 100 % 1000) >= 73
		sock.set_verify_callback(ssl::host_name_verification(host));
#else
		sock.set_verify_callback(ssl::rfc2818_verification(host));
#endif
		sock.handshake(ssl_socket::client);

		asio_ns::write(sock, req_buffer);

		return reader(sock);
	}
	else
	{
		tcp::socket sock(io_context);
		asio_ns::connect(sock, endpoints);

		asio_ns::write(sock, req_buffer);

		return reader(sock);
	}
}

reply head_request(const zeep::uri &url, std::vector<header> headers)
{
	return send_request({ "HEAD", url, { 1, 0 }, std::move(headers) });
}

reply get_request(const zeep::uri &url, std::vector<header> headers)
{
	return send_request({ "GET", url, { 1, 0 }, std::move(headers) });
}

reply post_request(const zeep::uri &url, std::vector<header> headers, const std::string &payload)
{
	// prepare a request

	request req{ "POST", url, { 1, 0 }, std::move(headers) };

	if (auto ct = req.get_header("Content-Type"); ct.empty())
		req.set_content(payload, "application/plain");
	else
		req.set_content(payload, ct);

	return send_request(std::move(req));
}

} // namespace zeep::http