#include "xml/exception.hpp"
#include "xml/soap/envelope.hpp"

using namespace std;

namespace xml
{
namespace soap
{

envelope::envelope()
{
}

envelope::envelope(
	document&		data)
{
	xml::node_ptr env = data.root();	// envelope
	if (env->name() != "Envelope" or env->ns() != "http://schemas.xmlsoap.org/soap/envelope/")
		throw exception("Invalid SOAP envelope");
	
	body_ = env->children();
	if (not body_ or body_->ns() != "http://schemas.xmlsoap.org/soap/envelope/")
		throw exception("Invalid SOAP envelope");
}

node_ptr envelope::request()
{
	return body_->children();
}

node_ptr make_envelope(node_ptr data)
{
	xml::node_ptr env(new node("Envelope", "http://schemas.xmlsoap.org/soap/envelope/", "env"));
	env->add_attribute("xmlns:env", "http://schemas.xmlsoap.org/soap/envelope/");
	xml::node_ptr body(new node("env:Body"));
	env->add_child(body);
	body->add_child(data);
	
	return env;
}

node_ptr make_fault(
	const string&		what)
{
	node_ptr fault(new node("env:Fault"));
	
	node_ptr faultCode(new node("faultcode"));
	faultCode->content("env:Server");
	fault->add_child(faultCode);
	
	node_ptr faultString(new node("faultstring"));
	faultString->content(what);
	fault->add_child(faultString);

	return make_envelope(fault);
}

node_ptr make_fault(
	const std::exception&	ex)
{
	return make_fault(ex.what());
}

}
}

