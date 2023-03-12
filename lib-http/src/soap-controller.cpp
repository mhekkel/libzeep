// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/exception.hpp>
#include <zeep/http/soap-controller.hpp>
#include <zeep/http/uri.hpp>

namespace zeep::http
{

soap_envelope::soap_envelope()
	: m_request(nullptr)
{
}

// envelope::envelope(xml::document& data)
// 	: m_request(nullptr)
// {
// 	const xml::xpath
// 		sRequestPath("/Envelope[namespace-uri()='http://schemas.xmlsoap.org/soap/envelope/']/Body[position()=1]/*[position()=1]");
	
// 	std::list<xml::element*> l = sRequestPath.evaluate<xml::element>(*data.root());
	
// 	if (l.empty())
// 		throw zeep::exception("Empty or invalid SOAP envelope passed");
	
// 	m_request = l.front();
// }

xml::element make_envelope(xml::element&& data)
{
	xml::element env("soap:Envelope", {
		{ "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/" },
		{ "soap:encodingStyle", "http://www.w3.org/2003/05/soap-encoding" }
	});
	auto& body = env.emplace_back("soap:Body");
	body.emplace_back(std::move(data));
	
	return env;
}

xml::element make_fault(const std::string& what)
{
	xml::element fault("soap:Fault");
	
	auto& faultCode = fault.emplace_back("faultcode");
	faultCode.set_content("soap:Server");
	
	auto& faultString(fault.emplace_back("faultstring"));
	faultString.set_content(what);

	return make_envelope(std::move(fault));
}

xml::element make_fault(const std::exception& ex)
{
	return make_fault(std::string(ex.what()));
}

// --------------------------------------------------------------------

bool soap_controller::handle_request(request& req, reply& reply)
{
	bool result = false;

	auto p = get_prefixless_path(req);
	
	if (req.get_method() == "POST" and p == m_prefix_path)
	{
		result = true;

		try
		{
			xml::document envelope(req.get_payload());

			auto request = envelope.find_first(
				"/Envelope[namespace-uri()='http://schemas.xmlsoap.org/soap/envelope/']/Body[position()=1]/*[position()=1]");
			if (request == envelope.cend())
				throw zeep::exception("Empty or invalid SOAP envelope passed");

			if (request->get_ns() != m_ns)
				throw zeep::exception("Invalid namespace for request");

			std::string action = request->name();
			// log() << action << ' ';

			for (auto& mp: m_mountpoints)
			{
				if (mp->m_action != action)
					continue;
				
				mp->call(*request, reply, m_ns);

				break;
			}
		}
		catch (const std::exception& e)
		{
			reply.set_content(make_fault(e));
			reply.set_status(internal_server_error);
		}
		catch (status_type& s)
		{
			reply.set_content(make_fault(get_status_description(s)));
			reply.set_status(s);
		}
	}
	else if (req.get_method() == "GET" and p == "wsdl")
	{
		reply.set_content(make_wsdl());
		reply.set_status(ok);
	}

	return result;
}

/// \brief Create a WSDL based on the registered actions
xml::element soap_controller::make_wsdl()
{
	// start by making the root node: wsdl:definitions

	xml::element wsdl("wsdl:definitions",
	{
		{ "targetNamespace", m_ns },
		{ "xmlns:ns", m_ns },
		{ "xmlns:wsdl", "http://schemas.xmlsoap.org/wsdl/" },
		{ "xmlns:soap", "http://schemas.xmlsoap.org/wsdl/soap/" }
	});
	
	// add wsdl:types
	auto& types = wsdl.emplace_back("wsdl:types");

	// add xsd:schema
	auto& schema = types.emplace_back("xsd:schema",
	{
		{ "targetNamespace", m_ns },
		{ "elementFormDefault", "qualified" },
		{ "attributeFormDefault", "unqualified" },
		{ "xmlns:xsd", "http://www.w3.org/2001/XMLSchema" }
	});

	using namespace std::literals;

	// add wsdl:binding
	auto& binding = wsdl.emplace_back("wsdl:binding",
	{
		{ "name", m_service },
		{ "type", "ns:" + m_service + "PortType" }
	});
	
	// add soap:binding
	binding.emplace_back("soap:binding",
	{
		{ "style", "document" },
		{ "transport", "http://schemas.xmlsoap.org/soap/http" }
	});
	
	// add wsdl:portType
	auto& portType = wsdl.emplace_back("wsdl:portType",
	{
		{ "name", m_service + "PortType" }
	});
	
	// and the types
	xml::type_map typeMap;
	message_map messageMap;
	
	for (auto& mp: m_mountpoints)
		mp->describe(typeMap, messageMap, portType, binding);
	
	for (auto &m : messageMap)
		wsdl.emplace_back(std::move(m.second));
	
	for (auto &t : typeMap)
		schema.emplace_back(std::move(t.second));
	
	// finish with the wsdl:service
	auto& service = wsdl.emplace_back("wsdl:service",
	{
		{ "name", m_service }
	});
	
	auto& port = service.emplace_back("wsdl:port",
	{
		{ "name", m_service },
		{ "binding", "ns:" + m_service }
	});
	
	std::string location = get_context_name() + "/" + m_location;

	port.emplace_back("soap:address",
	{
		{ "location", location }
	});
	
	return wsdl;
}

}
