// compile: clang++ -o synopsis-json synopsis-json.cpp -I ../../include/  ../../lib/libzeep-json.a ../../lib/libzeep-generic.a -std=c++17 -lstdc++fs -I ~/projects/boost_1_73_0/

#include <iostream>
#include <cassert>

#include <zeep/json/element.hpp>
#include <zeep/json/parser.hpp>

void test_stl()
{
    using namespace zeep::json::literals; 
    using json = zeep::json::element;

//[ stl_interface
    json j;

    /*<< Make j an array >>*/
    j = zeep::json::element::array({ 1, 2, 3 });
    j.push_back(4);
    j.emplace_back("five");

    assert(j == R"([ 1, 2, 3, 4, "five" ])"_json);

    /*<< Now make j an object, this will erase the data and initialize a new object >>*/
    j = zeep::json::element::object({ { "a", true }, { "b", "2" } });
    j.emplace("c", 3);

    assert(j == R"({ "a": true, "b": "2", "c": 3 })"_json);
//]
}

void construct()
{
//[ synopsis_json_main
    using namespace zeep::json::literals; 
    using json = zeep::json::element;

    json j1;

    /*<< Fill a JSON object with some data, the type is detected automatically >>*/
    j1["b"] = true;
    j1["i"] = 1;
    j1["f"] = 2.7183;
    j1["s"] = "Hello, world!";
    j1["ai"] = { 1, 2, 3 };
    j1["n"] = nullptr;
    j1["o"] = { { "b", false }, { "i", 2 } };
    j1["o"]["s"] = "sub field";

    std::cout << j1 << std::endl;

    /*<< Construct a JSON object by parsing a raw string >>*/

    json j2 = R"(
    {
        "b": true,
        "i": 1,
        "f": 2.7183,
        "s": "Hello, world!",
        "ai": [ 1, 2, 3 ],
        "n": null,
        "o": {
            "b": false,
            "i": 2,
            "s": "sub field"
        }
    }
    )"_json;

    std::cout << j2 << std::endl;

    assert(j1 == j2);
//]
}

int main()
{
    construct();
    test_stl();
    return 0;
}