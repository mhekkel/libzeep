//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_EXPATDOCUMENT_H
#define SOAP_XML_EXPATDOCUMENT_H

#include "zeep/xml/node.hpp"

namespace zeep { namespace xml {

class expat_doc : public boost::noncopyable
{
  public:
					expat_doc(
						std::istream&		data);
					
					expat_doc(
						const std::string&	data);

					expat_doc(
						node_ptr			data);

	virtual			~expat_doc();

	node_ptr		root() const;

  private:
	struct expat_doc_imp*	impl;
};

std::ostream& operator<<(std::ostream& lhs, const expat_doc& rhs);

}
}

#endif
