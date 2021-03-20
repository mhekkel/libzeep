/* compile:
clang++ -o http-server-0 http-server-0.cpp -I ../../include/  ../../lib/libzeep-http.a ../../lib/libzeep-xml.a ../../lib/libzeep-json.a -std=c++17 -lstdc++fs -I ~/projects/boost_1_73_0/ dummy_rsrc.o -pthread
*/

//[ most_simple_http_server
#include <zeep/http/server.hpp>

int main()
{
    zeep::http::server srv;
    srv.bind("::", 8080);
    srv.run(2);

    return 0;
}
//]