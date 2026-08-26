// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2022-2026
// SPDX-License-Identifier: BSL-1.0

// This code is originally written for mini-ibs, a content management system

#include "zeep/http/client.hpp"
#include "zeep/exception.hpp"
#include "zeep/http/message-parser.hpp"
#include "zeep/streambuf.hpp"
#include "zeep/unicode-support.hpp"

#include <iostream>

namespace zeep::http
{

using asio_ns::ip::tcp;

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
			throw invalid_argument_exception("Invalid scheme in uri for send_request");
	}

	asio_ns::io_context io_context;
	tcp::resolver resolver(io_context);
	tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

	// prepare a request
	req.set_header("Host", std::format("{}:{}", host, port));

	if (req.get_header("accept").empty())
		req.set_header("Accept", "*/*");

	if (req.get_header("user-agent").empty())
		req.set_header("User-Agent", reply::get_libzeep_version());

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

		ssl_socket sock(io_context, ctx);
		asio_ns::connect(sock.lowest_layer(), endpoints);
		sock.lowest_layer().set_option(tcp::no_delay(true));

		auto sni_host = host;
		if (sni_host.starts_with('[') and sni_host.ends_with(']'))
			sni_host = sni_host.substr(1, sni_host.size() - 2);
		(void)SSL_set_tlsext_host_name(sock.native_handle(), sni_host.c_str());

		// Perform SSL handshake and verify the remote host's certificate.
		sock.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);
		sock.set_verify_callback(ssl::host_name_verification(sni_host));
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
		req.set_content(payload, "text/plain");
	else
		req.set_content(payload, ct);

	return send_request(std::move(req));
}

} // namespace zeep::http