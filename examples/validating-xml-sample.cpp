// compile: clang++ -o validating-xml-sample validating-xml-sample.cpp -I ../../include/  ../../lib/libzeep-xml.a ../../lib/libzeep-generic.a -std=c++17 -lstdc++fs -I ~/projects/boost_1_73_0/ && ./validating-xml-sample

//[ xml_validation_sample

#include <filesystem>
#include <fstream>
#include <iostream>

#include <zeep/xml/document.hpp>

namespace fs = std::filesystem;

int main()
{
    /*<< Define an entity loader function >>*/
    auto loader = []
        (const std::string& base, const std::string& pubid, const std::string& sysid) -> std::istream*
    {
        if (base == "." and pubid.empty() and fs::exists(sysid))
            return new std::ifstream(sysid);
        
        throw std::invalid_argument("Invalid arguments passed in loader");
    };

    /*<< Create document and set the entity loader >>*/
    zeep::xml::document doc;
    doc.set_entity_loader(loader);

    /*<< Read a file >>*/
    std::ifstream is("sample.xml");
    is >> doc;

    using namespace zeep::xml::literals;

    /*<< Compare the doc with an in-memory constructed document, note that spaces are ignored >>*/
    if (doc == R"(<foo><bar>Hello, world!</bar></foo>)"_xml)
        std::cout << "ok" << std::endl;

    return 0;
}

//]