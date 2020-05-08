#include <zeep/xml/document.hpp>

//[ synopsis_xml_main
int main()
{
    using namespace zeep::xml::literals; 

    /*<< Construct an XML document in memory using a string literal >>*/
    auto doc = 
        R"(<persons>
            <person id="1">
                <firstname>John</firstname>
                <lastname>Doe</lastname>
            </person>
            <person id="2">
                <firstname>Jane</firstname>
                <lastname>Jones</lastname>
            </person>
        </persons>)"_xml;

    /*<< Iterate over an XPath result set >>*/
    for (auto& person: doc.find("//person")) 
    {
        std::string firstname, lastname;

        /*<< Iterate over the __element__ nodes inside the person __element__ >>*/
        for (auto& name: *person)
        {
            if (name.name() == "firstname")	firstname = name.str();
            if (name.name() == "lastname")	lastname = name.str();
        }

        std::cout << person->attr("id") << ": " << lastname << ", " << firstname << std::endl;
    }

    return 0;
}
//]