#include <zeep/exception.hpp>
#include <zeep/http/soap-controller.hpp>

#define BOOST_TEST_MODULE SOAP_Test
#include <boost/test/included/unit_test.hpp>

using namespace std;
namespace z = zeep;
namespace zx = zeep::xml;
namespace zh = zeep::http;

struct TestStruct
{
	int a;
	string s;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		ar & zx::make_element_nvp("a", a)
		   & zx::make_element_nvp("s", s);
	}
};

static_assert(z::has_serialize_v<TestStruct, zx::serializer>, "oops");

struct my_test_controller : public zh::soap_controller
{
	my_test_controller()
		: zh::soap_controller("ws", "test", "http://www.hekkelman.com/libzeep/soap")
	{
		set_service("testService");

		map_action("Test", &my_test_controller::test_method_1, "x");
		map_action("Test2", &my_test_controller::test_method_2, "s");
		map_action("Test3", &my_test_controller::test_method_3, "t");
	}

	int test_method_1(int x)
	{
		BOOST_TEST(x == 42);
		return x;
	}

	void test_method_2(const std::string &s)
	{
		BOOST_TEST(s == "42");
	}

	TestStruct test_method_3(const TestStruct &t)
	{
		return { t.a + 1, t.s + to_string(t.a) };
	}
};

BOOST_AUTO_TEST_CASE(soap_1)
{
	using namespace zx::literals;

	my_test_controller srv;

	auto payload_test_1 = R"(<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test xmlns:ns="http://www.hekkelman.com/libzeep/soap">
   <ns:x>42</ns:x>
  </ns:Test>
 </soap:Body>
</soap:Envelope>)";

	zh::request req("POST", "/ws", { 1, 0 }, {}, payload_test_1);

	zh::reply rep;
	srv.handle_request(req, rep);

	BOOST_REQUIRE(rep.get_status() == 200);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zx::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <m:TestResponse xmlns:m="http://www.hekkelman.com/libzeep/soap">42</m:TestResponse>
 </soap:Body>
</soap:Envelope>)"_xml;

	BOOST_TEST(repDoc == test);
}

BOOST_AUTO_TEST_CASE(soap_2)
{
	using namespace zx::literals;

	my_test_controller srv;

	auto payload_test = R"(<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test2 xmlns:ns="http://www.hekkelman.com/libzeep/soap">
   <ns:s>42</ns:s>
  </ns:Test2>
 </soap:Body>
</soap:Envelope>)";

	zh::request req("POST", "/ws", { 1, 0 }, {}, payload_test);

	zh::reply rep;
	srv.handle_request(req, rep);

	BOOST_REQUIRE(rep.get_status() == 200);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zx::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test2Response xmlns:ns="http://www.hekkelman.com/libzeep/soap" />
 </soap:Body>
</soap:Envelope>)"_xml;

	BOOST_TEST(repDoc == test);
}

BOOST_AUTO_TEST_CASE(soap_3)
{
	using namespace zx::literals;

	my_test_controller srv;

	auto payload_test = R"(<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test3 xmlns:ns="http://www.hekkelman.com/libzeep/soap">
   <ns:t>
	<ns:a>42</ns:a>
	<ns:s>42</ns:s>
   </ns:t>
  </ns:Test3>
 </soap:Body>
</soap:Envelope>)";

	zh::request req("POST", "/ws", { 1, 0 }, {}, payload_test);

	zh::reply rep;
	srv.handle_request(req, rep);

	BOOST_REQUIRE(rep.get_status() == 200);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zx::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test3Response xmlns:ns="http://www.hekkelman.com/libzeep/soap"><ns:a>43</ns:a><ns:s>4242</ns:s></ns:Test3Response>
 </soap:Body>
</soap:Envelope>)"_xml;

	BOOST_TEST(repDoc == test);
}

BOOST_AUTO_TEST_CASE(soap_3f)
{
	using namespace zx::literals;

	my_test_controller srv;

	auto payload_test = R"(<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test3 xmlns:ns="http://www.hekkelman.com/libzeep/soap-dit-is-fout">
   <ns:t>
	<ns:a>42</ns:a>
	<ns:s>42</ns:s>
   </ns:t>
  </ns:Test3>
 </soap:Body>
</soap:Envelope>)";

	zh::request req("POST", "/ws", { 1, 0 }, {}, payload_test);

	zh::reply rep;
	srv.handle_request(req, rep);

	BOOST_TEST(rep.get_status() == 500);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zx::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <soap:Fault>
   <faultcode>soap:Server</faultcode>
   <faultstring>Invalid namespace for request</faultstring>
  </soap:Fault>
 </soap:Body>
</soap:Envelope>)"_xml;

	BOOST_TEST(repDoc == test);
}

BOOST_AUTO_TEST_CASE(soap_w1)
{
	using namespace zx::literals;

	my_test_controller srv;

	zeep::xml::document doc;
	doc.emplace_back(srv.make_wsdl());

	cerr << setw(2) << doc << endl;
}