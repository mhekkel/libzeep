// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/exception.hpp>
#include <zeep/soap/controller.hpp>

namespace zeep::soap
{

bool controller::handle_request(http::request& req, http::reply& reply)
{
	bool result = false;

	std::string p = req.uri;
	while (p.front() == '/')
		p.erase(0, 1);
	
	if (req.method == http::method_type::POST and p == m_prefix_path)
	{
		result = true;

		try
		{
			xml::document envelope(req.payload);

			auto request = envelope.find_first(
				"/Envelope[namespace-uri()='http://schemas.xmlsoap.org/soap/envelope/']/Body[position()=1]/*[position()=1]");
			if (request == envelope.end())
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
			reply.set_content(soap::make_fault(e));
			reply.set_status(http::internal_server_error);
		}
		catch (http::status_type& s)
		{
			reply.set_content(soap::make_fault(get_status_description(s)));
			reply.set_status(s);
		}
	}
	else if (req.method == http::method_type::GET and p == m_prefix_path + "/wsdl")
	{
		reply.set_content(make_wsdl());
		reply.set_status(http::ok);
	}

	return result;
}

/// \brief Create a WSDL based on the registered actions
xml::element controller::make_wsdl()
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
	
	// // // and the types
	// // xml::type_map typeMap;
	// // detail::message_map messageMap;
	
	// // for (auto& mp: m_mountpoints)
	// // 	mp->collect(typeMap, messageMap, portType, binding);
	
	// // for (detail::message_map::iterator m = messageMap.begin(); m != messageMap.end(); ++m)
	// // 	wsdl->append(m->second);
	
	// // for (xml::type_map::iterator t = typeMap.begin(); t != typeMap.end(); ++t)
	// // 	schema->append(t->second);
	
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
	
	port.emplace_back("soap:address",
	{
		{ "location", m_location }
	});
	
	return wsdl;
}

}
