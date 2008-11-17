#ifndef XML_SOAP_ENVELOPE_H
#define XML_SOAP_ENVELOPE_H

#include "xml/document.hpp"
#include "xml/exception.hpp"

namespace xml
{
namespace soap
{

class envelope : public boost::noncopyable
{
  public:
				envelope();

				envelope(
					document&	data);

	node_ptr	request();

  private:
	node_ptr	body_;
};

node_ptr make_envelope(
			node_ptr data);

node_ptr make_fault(
			const std::string&		message);

node_ptr make_fault(
			const std::exception&	ex);

}
}

#endif
