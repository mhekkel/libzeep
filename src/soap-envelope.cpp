//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/exception.hpp"
#include "zeep/envelope.hpp"

using namespace std;

namespace zeep {

envelope::envelope()
{
}

envelope::envelope(xml::document& data)
{
	xml::node_ptr env = data.root();	// envelope
	if (env->name() != "Envelope" or env->ns() != "http://schemas.xmlsoap.org/soap/envelope/")
		throw exception("Invalid SOAP envelope");
	
	m_body = env->children();
	if (not m_body or m_body->ns() != "http://schemas.xmlsoap.org/soap/envelope/")
		throw exception("Invalid SOAP envelope");
}

xml::node_ptr envelope::request()
{
	return m_body->children();
}

xml::node_ptr make_envelope(xml::node_ptr data)
{
	xml::node_ptr env(new xml::node("Envelope", "http://schemas.xmlsoap.org/soap/envelope/", "env"));
	env->add_attribute("xmlns:env", "http://schemas.xmlsoap.org/soap/envelope/");
	xml::node_ptr body(new xml::node("env:Body"));
	env->add_child(body);
	body->add_child(data);
	
	return env;
}

xml::node_ptr make_fault(const string& what)
{
	xml::node_ptr fault(new xml::node("env:Fault"));
	
	xml::node_ptr faultCode(new xml::node("faultcode"));
	faultCode->content("env:Server");
	fault->add_child(faultCode);
	
	xml::node_ptr faultString(new xml::node("faultstring"));
	faultString->content(what);
	fault->add_child(faultString);

	return make_envelope(fault);
}

xml::node_ptr make_fault(const std::exception& ex)
{
	return make_fault(ex.what());
}

}

