//           Copyright Maarten L. Hekkelman, 2022-2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

//[ most_simple_http_server_start
#include <zeep/http/server.hpp>

int main()
{
    zeep::http::server srv;
    srv.bind("::", 8080);
    srv.run(2);

    return 0;
}
//]