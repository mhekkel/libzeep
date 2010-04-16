//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/xml/document.hpp"

namespace zeep { namespace xml {

// --------------------------------------------------------------------

class libxml2_document : public document
{
  public:
						libxml2_document();
						libxml2_document(const std::string& s);
						libxml2_document(std::istream& is);
};

}
}
