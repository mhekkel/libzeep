#ifndef XML_SOAP_ENVELOPE_H
#define XML_SOAP_ENVELOPE_H

#include "soap/xml/document.hpp"
#include "soap/exception.hpp"

namespace soap
{

class envelope : public boost::noncopyable
{
  public:
					envelope();
	
					envelope(xml::document& data);

	xml::node_ptr	request();

  private:
	xml::node_ptr	m_body;
};

xml::node_ptr make_envelope(xml::node_ptr data);
xml::node_ptr make_fault(const std::string& message);
xml::node_ptr make_fault(const std::exception& ex);

}

#endif
