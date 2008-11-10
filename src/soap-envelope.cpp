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

}
}

