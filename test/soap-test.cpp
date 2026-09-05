// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#if ZEEP_CXX_MODULE
import zeem;
import zeep;
#else
#include <zeep/http/status.hpp>
#include <zeep/exception.hpp>
#include <zeep/http/soap-controller.hpp>
#endif

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <iomanip>

struct TestStruct
{
	constexpr static std::string type_name() { return "TestStruct"; }

	int a;
	std::string s;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		ar &zeem::make_element_nvp("a", a) & zeem::make_element_nvp("s", s);
	}
};

// static_assert(zeep::has_serialize_v<TestStruct, zeem::serializer>, "oops");

struct my_test_controller : public zeep::http::soap_controller
{
	my_test_controller()
		: zeep::http::soap_controller("ws", "test", "http://www.hekkelman.com/libzeep/soap")
	{
		set_service("testService");

		map_action("Test", &my_test_controller::test_method_1, "x");
		map_action("Test2", &my_test_controller::test_method_2, "s");
		map_action("Test3", &my_test_controller::test_method_3, "t");
	}

	int test_method_1(int x)
	{
		CHECK(x == 42);
		return x;
	}

	void test_method_2(const std::string &s)
	{
		CHECK(s == "42");
	}

	TestStruct test_method_3(const TestStruct &t)
	{
		return { t.a + 1, t.s + std::to_string(t.a) };
	}
};

TEST_CASE("soap_1")
{
	using namespace zeem::literals;

	my_test_controller srv;

	auto payload_test_1 = R"(<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test xmlns:ns="http://www.hekkelman.com/libzeep/soap">
   <ns:x>42</ns:x>
  </ns:Test>
 </soap:Body>
</soap:Envelope>)";

	zeep::http::request req("POST", "/ws", { 1, 0 }, {}, payload_test_1);

	zeep::http::reply rep;
	srv.handle_request(req, rep);

	REQUIRE(rep.get_status() == zeep::http::status_type::ok);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zeem::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <m:TestResponse xmlns:m="http://www.hekkelman.com/libzeep/soap">42</m:TestResponse>
 </soap:Body>
</soap:Envelope>)"_xml;

	CHECK(repDoc == test);
}

TEST_CASE("soap_2")
{
	using namespace zeem::literals;

	my_test_controller srv;

	auto payload_test = R"(<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test2 xmlns:ns="http://www.hekkelman.com/libzeep/soap">
   <ns:s>42</ns:s>
  </ns:Test2>
 </soap:Body>
</soap:Envelope>)";

	zeep::http::request req("POST", "/ws", { 1, 0 }, {}, payload_test);

	zeep::http::reply rep;
	srv.handle_request(req, rep);

	REQUIRE(rep.get_status() == zeep::http::status_type::ok);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zeem::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test2Response xmlns:ns="http://www.hekkelman.com/libzeep/soap" />
 </soap:Body>
</soap:Envelope>)"_xml;

	CHECK(repDoc == test);
}

TEST_CASE("soap_3")
{
	using namespace zeem::literals;

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

	zeep::http::request req("POST", "/ws", { 1, 0 }, {}, payload_test);

	zeep::http::reply rep;
	srv.handle_request(req, rep);

	REQUIRE(rep.get_status() == zeep::http::status_type::ok);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zeem::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <ns:Test3Response xmlns:ns="http://www.hekkelman.com/libzeep/soap"><ns:a>43</ns:a><ns:s>4242</ns:s></ns:Test3Response>
 </soap:Body>
</soap:Envelope>)"_xml;

	CHECK(repDoc == test);
}

TEST_CASE("soap_3f")
{
	using namespace zeem::literals;

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

	zeep::http::request req("POST", "/ws", { 1, 0 }, {}, payload_test);

	zeep::http::reply rep;
	srv.handle_request(req, rep);

	CHECK(rep.get_status() == zeep::http::status_type::internal_server_error);

	std::stringstream srep;
	srep << rep;

	std::string line;
	while (getline(srep, line))
	{
		if (line.empty() or line == "\r")
			break;
	}

	zeem::document repDoc(srep);

	auto test = R"(
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://www.w3.org/2003/05/soap-encoding">
 <soap:Body>
  <soap:Fault>
   <faultcode>soap:Server</faultcode>
   <faultstring>An internal error prevented the server from processing your request</faultstring>
  </soap:Fault>
 </soap:Body>
</soap:Envelope>)"_xml;

	CHECK(repDoc == test);
}

TEST_CASE("soap_w1")
{
	using namespace zeem::literals;

	my_test_controller srv;

	zeem::document doc;
	doc.emplace_back(srv.make_wsdl());

	auto test = R"(<wsdl:definitions targetNamespace="http://www.hekkelman.com/libzeep/soap" xmlns:ns="http://www.hekkelman.com/libzeep/soap" xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/" xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/">
  <wsdl:types>
    <xsd:schema targetNamespace="http://www.hekkelman.com/libzeep/soap" elementFormDefault="qualified" attributeFormDefault="unqualified" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
      <xsd:element name="Test2">
        <xsd:complexType>
          <xsd:sequence>
            <xsd:element name="s" type="xsd:string" minOccurs="1" maxOccurs="1"/>
          </xsd:sequence>
        </xsd:complexType>
      </xsd:element>
      <xsd:element name="Test2Response"/>
      <xsd:element name="Test3">
        <xsd:complexType>
          <xsd:sequence>
            <xsd:element name="t" type="ns:TestStruct" minOccurs="1" maxOccurs="1"/>
          </xsd:sequence>
        </xsd:complexType>
      </xsd:element>
      <xsd:element name="Test3Response">
        <xsd:complexType>
          <xsd:sequence>
            <xsd:element name="Response" type="ns:TestStruct" minOccurs="1" maxOccurs="1"/>
          </xsd:sequence>
        </xsd:complexType>
      </xsd:element>
      <xsd:element name="Test">
        <xsd:complexType>
          <xsd:sequence>
            <xsd:element name="x" type="xsd:int" minOccurs="1" maxOccurs="1"/>
          </xsd:sequence>
        </xsd:complexType>
      </xsd:element>
      <xsd:element name="TestResponse">
        <xsd:complexType>
          <xsd:sequence>
            <xsd:element name="Response" type="xsd:int" minOccurs="1" maxOccurs="1"/>
          </xsd:sequence>
        </xsd:complexType>
      </xsd:element>
      <xsd:complexType name="TestStruct">
        <xsd:sequence>
          <xsd:element name="a" type="xsd:int" minOccurs="1" maxOccurs="1"/>
          <xsd:element name="s" type="xsd:string" minOccurs="1" maxOccurs="1"/>
        </xsd:sequence>
      </xsd:complexType>
    </xsd:schema>
  </wsdl:types>
  <wsdl:binding name="testService" type="ns:testServicePortType">
    <soap:binding style="document" transport="http://schemas.xmlsoap.org/soap/http"/>
    <wsdl:operation name="Test">
      <soap:operation soapAction="" style="document"/>
      <wsdl:input>
        <soap:body use="literal"/>
      </wsdl:input>
      <wsdl:output>
        <soap:body use="literal"/>
      </wsdl:output>
    </wsdl:operation>
    <wsdl:operation name="Test2">
      <soap:operation soapAction="" style="document"/>
      <wsdl:input>
        <soap:body use="literal"/>
      </wsdl:input>
      <wsdl:output>
        <soap:body use="literal"/>
      </wsdl:output>
    </wsdl:operation>
    <wsdl:operation name="Test3">
      <soap:operation soapAction="" style="document"/>
      <wsdl:input>
        <soap:body use="literal"/>
      </wsdl:input>
      <wsdl:output>
        <soap:body use="literal"/>
      </wsdl:output>
    </wsdl:operation>
  </wsdl:binding>
  <wsdl:portType name="testServicePortType">
    <wsdl:operation name="Test">
      <wsdl:input message="ns:TestRequestMessage"/>
      <wsdl:output message="ns:TestMessage"/>
    </wsdl:operation>
    <wsdl:operation name="Test2">
      <wsdl:input message="ns:Test2RequestMessage"/>
      <wsdl:output message="ns:Test2Message"/>
    </wsdl:operation>
    <wsdl:operation name="Test3">
      <wsdl:input message="ns:Test3RequestMessage"/>
      <wsdl:output message="ns:Test3Message"/>
    </wsdl:operation>
  </wsdl:portType>
  <wsdl:message name="Test2Message">
    <wsdl:part name="parameters" element="ns:Test2"/>
  </wsdl:message>
  <wsdl:message name="Test2RequestMessage">
    <wsdl:part name="parameters" element="ns:Test2"/>
  </wsdl:message>
  <wsdl:message name="Test3Message">
    <wsdl:part name="parameters" element="ns:Test3"/>
  </wsdl:message>
  <wsdl:message name="Test3RequestMessage">
    <wsdl:part name="parameters" element="ns:Test3"/>
  </wsdl:message>
  <wsdl:message name="TestMessage">
    <wsdl:part name="parameters" element="ns:Test"/>
  </wsdl:message>
  <wsdl:message name="TestRequestMessage">
    <wsdl:part name="parameters" element="ns:Test"/>
  </wsdl:message>
  <wsdl:service name="testService">
    <wsdl:port name="testService" binding="ns:testService">
      <soap:address location="ws"/>
    </wsdl:port>
  </wsdl:service>
</wsdl:definitions>
	)"_xml;
	
	CHECK(doc == test);
	// std::cerr << std::setw(2) << doc << '\n';
}