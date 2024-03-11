//           Copyright Maarten L. Hekkelman, 2022-2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <zeep/xml/document.hpp>
#include <zeep/xml/xpath.hpp>

//[ xpath_sample
int main()
{
    using namespace mxml::literals;

    auto doc = R"(<bar xmlns:z="https://www.hekkelman.com/libzeep">
        <z:foo>foei</z:foo>
    </bar>)"_xml;

    /* Create an xpath context and store our variable */
    mxml::context ctx;
    ctx.set("ns", "https://www.hekkelman.com/libzeep");

    /* Create an xpath object with the specified XPath using the variable `ns` */
    auto xp = mxml::xpath("//*[namespace-uri() = $ns]");

    /* Iterate over the result of the evaluation of this XPath, the result will consist of mxml::element object pointers */
    for (auto n: xp.evaluate<mxml::element>(doc, ctx))
        std::cout << n->str() << '\n';

    return 0;
}
//]