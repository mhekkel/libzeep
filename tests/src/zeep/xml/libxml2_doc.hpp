//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_LIBXML2DOCUMENT_H
#define SOAP_XML_LIBXML2DOCUMENT_H

#include "zeep/xml/node.hpp"

namespace zeep { namespace xml {

class libxml2_doc : public boost::noncopyable
{
  public:
					libxml2_doc(
						std::istream&		data);
					
					libxml2_doc(
						const std::string&	data);

					libxml2_doc(
						node_ptr			data);

	virtual			~libxml2_doc();

	node_ptr		root() const;

  private:
	struct libxml2_doc_imp*	impl;
};

std::ostream& operator<<(std::ostream& lhs, const libxml2_doc& rhs);

}
}

#endif
