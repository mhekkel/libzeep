// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/soap-controller.hpp"
#include "zeep/exception.hpp"
#include "zeep/uri.hpp"

namespace zeep::http
{

soap_envelope::soap_envelope()
	: m_request(nullptr)
{
}

// envelope::envelope(zeem::document& data)
// 	: m_request(nullptr)
// {
// 	const zeem::xpath
// 		sRequestPath("/Envelope[namespace-uri()='http://schemas.xmlsoap.org/soap/envelope/']/Body[position()=1]/*[position()=1]");

// 	std::list<zeem::element*> l = sRequestPath.evaluate<zeem::element>(*data.root());

// 	if (l.empty())
// 		throw zeep::exception("Empty or invalid SOAP envelope passed");

// 	m_request = l.front();
// }

zeem::element make_envelope(zeem::element &&data)
{
	zeem::element env("soap:Envelope", { { "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/" },
										   { "soap:encodingStyle", "http://www.w3.org/2003/05/soap-encoding" } });
	auto body = env.emplace_back("soap:Body");
	body->emplace_back(std::move(data));

	return env;
}

zeem::element make_fault(std::string what)
{
	zeem::element fault("soap:Fault");

	auto faultCode = fault.emplace_back("faultcode");
	faultCode->set_content("soap:Server");

	auto faultString(fault.emplace_back("faultstring"));
	faultString->set_content(std::move(what));

	return make_envelope(std::move(fault));
}

zeem::element make_fault(const std::exception &ex)
{
	return make_fault(std::string(ex.what()));
}

// --------------------------------------------------------------------

bool soap_controller::handle_request(request &req, reply &reply)
{
	bool result = false;

	auto p = get_prefixless_path(req);

	if (req.get_method() == "POST" and p.empty())
	{
		result = true;

		try
		{
			zeem::document envelope(req.get_payload());

			auto request = envelope.find_first(
				"/*:Envelope[namespace-uri()='http://schemas.xmlsoap.org/soap/envelope/']/*:Body[position()=1]/*[position()=1]");
			if (request == envelope.end())
				throw zeep::exception("Empty or invalid SOAP envelope passed");

			if (request->get_ns() != m_ns)
				throw zeep::exception("Invalid namespace for request");

			std::string action = request->name();
			// log() << action << ' ';

			for (auto &mp : m_mountpoints)
			{
				if (mp->m_action != action)
					continue;

				mp->call(*request, reply, m_ns);

				break;
			}
		}
		catch (const std::exception &e)
		{
			reply.set_content(make_fault(e));
			reply.set_status(status_type::internal_server_error);
		}
		catch (status_type &s)
		{
			reply.set_content(make_fault(get_status_description(s)));
			reply.set_status(s);
		}
	}
	else if (req.get_method() == "GET" and p == "wsdl")
	{
		reply.set_content(make_wsdl());
		reply.set_status(status_type::ok);
		result = true;
	}

	return result;
}

/// \brief Create a WSDL based on the registered actions
zeem::element soap_controller::make_wsdl()
{
	// start by making the root node: wsdl:definitions

	zeem::element wsdl("wsdl:definitions",
		{ { "targetNamespace", m_ns },
			{ "xmlns:ns", m_ns },
			{ "xmlns:wsdl", "http://schemas.xmlsoap.org/wsdl/" },
			{ "xmlns:soap", "http://schemas.xmlsoap.org/wsdl/soap/" } });

	// add wsdl:types
	auto types = wsdl.emplace_back("wsdl:types");

	// add xsd:schema
	auto schema = types->emplace_back(zeem::element{ "xsd:schema",
		{ { "targetNamespace", m_ns },
			{ "elementFormDefault", "qualified" },
			{ "attributeFormDefault", "unqualified" },
			{ "xmlns:xsd", "http://www.w3.org/2001/XMLSchema" } } });

	using namespace std::literals;

	// add wsdl:binding
	auto binding = wsdl.emplace_back(zeem::element{ "wsdl:binding",
		{ { "name", m_service },
			{ "type", "ns:" + m_service + "PortType" } } });

	// add soap:binding
	binding->emplace_back(zeem::element{ "soap:binding",
		{ { "style", "document" },
			{ "transport", "http://schemas.xmlsoap.org/soap/http" } } });

	// add wsdl:portType
	auto portType = wsdl.emplace_back(zeem::element{ "wsdl:portType",
		{ { "name", m_service + "PortType" } } });

	// and the types
	zeem::type_map typeMap;
	message_map messageMap;

	for (auto &mp : m_mountpoints)
		mp->describe(typeMap, messageMap, *portType, *binding);

	for (auto &m : messageMap)
		wsdl.emplace_back(std::move(m.second));

	for (auto &t : typeMap)
		schema->emplace_back(std::move(t.second));

	// finish with the wsdl:service
	auto service = wsdl.emplace_back(zeem::element{ "wsdl:service",
		{ { "name", m_service } } });

	auto port = service->emplace_back(zeem::element{ "wsdl:port",
		{ { "name", m_service },
			{ "binding", "ns:" + m_service } } });

	std::string location = (uri(get_context_name()) / m_location).string();

	port->emplace_back(zeem::element{ "soap:address",
		{ { "location", location } } });

	return wsdl;
}

} // namespace zeep::http
