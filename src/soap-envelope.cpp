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
	xml::element* env = data.root();	// envelope
	if (env->local_name() != "Envelope" or env->ns_name() != "http://schemas.xmlsoap.org/soap/envelope/")
		throw exception("Invalid SOAP envelope");
	
	if (env->children().empty())
		throw exception("Invalid SOAP envelope, missing body");
	
	m_body = dynamic_cast<xml::element*>(env->children().front());
	if (not m_body or m_body->ns_name() != "http://schemas.xmlsoap.org/soap/envelope/")
		throw exception("Invalid SOAP envelope");
}

xml::element* envelope::request()
{
	if (m_body->children().empty())
		throw exception("Invalid SOAP envelope, missing request");
	
	return dynamic_cast<xml::element*>(m_body->children().front());
}

xml::element* make_envelope(xml::element* data)
{
	xml::element* env(new xml::element("env:Envelope"));
	env->set_name_space("env", "http://schemas.xmlsoap.org/soap/envelope/");
	xml::element* body(new xml::element("env:Body"));
	env->append(body);
	body->append(data);
	
	return env;
}

xml::element* make_fault(const string& what)
{
	xml::element* fault(new xml::element("env:Fault"));
	
	xml::element* faultCode(new xml::element("faultcode"));
	faultCode->content("env:Server");
	fault->append(faultCode);
	
	xml::element* faultString(new xml::element("faultstring"));
	faultString->content(what);
	fault->append(faultString);

	return make_envelope(fault);
}

xml::element* make_fault(const std::exception& ex)
{
	return make_fault(ex.what());
}

}

