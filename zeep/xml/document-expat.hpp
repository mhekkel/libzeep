//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_DOCUMENT_EXPAT_H
#define SOAP_XML_DOCUMENT_EXPAT_H

#include "zeep/xml/document.hpp"

namespace zeep { namespace xml {

// --------------------------------------------------------------------

class expat_document : public document
{
  public:
						expat_document();
						expat_document(const std::string& s);
						expat_document(std::istream& is);
};

}
}

#endif
