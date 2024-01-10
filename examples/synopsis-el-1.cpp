//           Copyright Maarten L. Hekkelman, 2022-2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <cassert>

#include <zeep/json/element.hpp>
#include <zeep/json/parser.hpp>
#include <zeep/http/el-processing.hpp>

int main()
{
    zeep::http::scope scope;

    //[ fill_scope
    /* Fill a scope with an array of objects, each object having one element */
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

    std::cout << s << '\n';

    assert(s == "1: 1, 2: 2");

    return 0;
}