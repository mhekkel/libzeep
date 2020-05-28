//[ synopsis_xml_serialize
#include <fstream>
#include <zeep/xml/document.hpp>

struct Person
{
    std::string firstname;
    std::string lastname;

    /*<< A struct we want to serialize needs a `serialize` method >>*/
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & zeep::make_nvp("firstname", firstname)
           & zeep::make_nvp("lastname", lastname);
    }
};

int main()
{
    /*<< Read in a text document containing XML and parse it into a document object >>*/
    std::ifstream file("test.xml");
    zeep::xml::document doc(file);
    
    std::vector<Person> persons;
    /*<< Deserialize all persons into an array >>*/
    doc.deserialize("persons", persons);

    doc.clear();

    /*<< Serialize all persons back into an XML document again >>*/
    doc.serialize("persons", persons);

    return 0;
}
//]