#include <zeep/http/tag-processor-v2.hpp>

using namespace std;

zeep::xml::document operator""_xml(const char* text, size_t length)
{
    zeep::xml::document doc;
    doc.read(string(text, length));
    return doc;
}

void test_1()
{
    cout << "test 1" << endl;

    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test m:if="${true}"/>
</data>
    )"_xml;

    // auto xml = doc.child();
    cout << doc << endl;

    class tl : public zeep::http::template_loader
    {
       	void load_template(const std::string& file, zeep::xml::document& doc)
           {}
    } tldr;

    zeep::http::tag_processor_v2 tp(tldr, "http://www.hekkelman.com/libzeep/m2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
    tp.process_xml(doc.child(), scope, "");

    cout << doc << endl;
}

void test_2()
{
    cout << "test 2" << endl;

    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test m:if="${'d' not in b}"/>
</data>
    )"_xml;

    // auto xml = doc.child();
    cout << doc << endl;

    class tl : public zeep::http::template_loader
    {
       	void load_template(const std::string& file, zeep::xml::document& doc)
           {}
    } tldr;

    zeep::http::tag_processor_v2 tp(tldr, "http://www.hekkelman.com/libzeep/m2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
 
    scope.put("b", zeep::el::element{ "a", "b", "c"});
 
    tp.process_xml(doc.child(), scope, "");

    cout << doc << endl;
}

void test_3()
{
    cout << __FUNCTION__ << endl;

    auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
    <test m:text="${x}"/>
</data>
    )"_xml;

    // auto xml = doc.child();
    cout << doc << endl;

    class tl : public zeep::http::template_loader
    {
       	void load_template(const std::string& file, zeep::xml::document& doc)
           {}
    } tldr;

    zeep::http::tag_processor_v2 tp(tldr, "http://www.hekkelman.com/libzeep/m2");

    zeep::el::scope scope(*static_cast<zeep::http::request*>(nullptr));
 
    scope.put("x", "<hallo, wereld!>");
 
    tp.process_xml(doc.child(), scope, "");

    cout << doc << endl;
}
int main()
{
    try
    {
        test_1();
        test_2();
        test_3();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}