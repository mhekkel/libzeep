// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2022-2025
// SPDX-License-Identifier: BSL-1.0

//[ most_simple_http_server_start
#include <zeep/http/server.hpp>

#include <exception>
#include <iostream>

int main()
{
	try
	{
		zeep::http::server srv;
		srv.bind("::", 8080);
		srv.run(2);
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << "\n";
	}

	return 0;
}
//]