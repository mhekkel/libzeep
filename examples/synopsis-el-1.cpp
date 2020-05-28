// compile: clang++ -o synopsis-json synopsis-json.cpp -I ../../include/  ../../lib/libzeep-json.a ../../lib/libzeep-generic.a -std=c++17 -lstdc++fs -I ~/projects/boost_1_73_0/

#include <iostream>
#include <cassert>

#include <zeep/json/element.hpp>
#include <zeep/json/parser.hpp>
#include <zeep/http/el-processing.hpp>

int main()
{
    zeep::http::scope scope;

    //[ fill_scope
    /*<< Fill a scope with an array of objects, each object having one element >>*/
    zeep::json::element ints{
        {
            { "value", 1 }
        },
        {
            { "value", 2 }
        }
    };
    scope.put("ints", ints);
    //]

    //[ evaluate_el
    auto s = zeep::http::evaluate_el(scope, "|1: ${ints[0].value}, 2: ${ints[1].value}|");
    //]

    std::cout << s << std::endl;

    assert(s == "1: 1, 2: 2");

    return 0;
}