// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/status.hpp"
#include <zeep/exception.hpp>
#include <zeep/http/soap-controller.hpp>

#include <catch2/catch_test_macros.hpp>

#include <iostream>

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
   <faultstring>Invalid namespace for request</faultstring>
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

	std::cerr << std::setw(2) << doc << '\n';
}