#ifndef XML_SOAP_ENVELOPE_H
#define XML_SOAP_ENVELOPE_H

#include "xml/document.hpp"

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

}
}

#endif
