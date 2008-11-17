#ifndef XML_DOCUMENT_H
#define XML_DOCUMENT_H

#include "soap/xml/node.hpp"

namespace soap { namespace xml {

class document : public boost::noncopyable
{
  public:
					document(
						std::istream&		data);
					
					document(
						const std::string&	data);

					document(
						node_ptr			data);

	virtual			~document();

	node_ptr		root() const;

  private:
	struct document_imp*	impl;
};

std::ostream& operator<<(std::ostream& lhs, const document& rhs);

}
}

#endif
