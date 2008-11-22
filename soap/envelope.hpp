//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_ENVELOPE_H
#define SOAP_ENVELOPE_H

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
