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
						expat_doc();
						expat_doc(const std::string& s);
						expat_doc(std::istream& is);

	virtual				~expat_doc();

	// I/O
	void				read(const std::string& s);
	void				read(std::istream& is);
	void				read(std::istream& is, const boost::filesystem::path& base_dir);

	virtual void		write(writer& w) const;

	// A valid xml expat_doc contains exactly one root element
	root_node*			root() const;
	
	// and this root has one child xml::element
	element*			child() const;
	void				child(element* e);
	
	// helper functions
	element_set			find(const std::string& path);
	element*			find_first(const std::string& path);

	// Compare two xml expat_docs
	bool				operator==(const expat_doc& doc) const;

	bool				operator!=(const expat_doc& doc) const		{ return not operator==(doc); }
	
	// In case the expat_doc needs to locate included DTD files
	void				base_dir(const boost::filesystem::path& path);

	// the encoding to use for output, or the detected encoding in the input
	encoding_type		encoding() const;
	void				encoding(encoding_type enc);

	// to format the output, use the following:
	
	// number of spaces to indent elements:
	int					indent() const;
	void				indent(int indent);
	
	// whether to have each element on a separate line
	bool				wrap() const;
	void				wrap(bool wrap);
	
	// reduce the whitespace in #PCDATA sections
	bool				trim() const;
	void				trim(bool trim);

	// option for parsing
	void				set_validating(bool validate);


  private:
	struct expat_doc_imp*	m_impl;
};

std::istream& operator>>(std::istream& lhs, expat_doc& rhs);
std::ostream& operator<<(std::ostream& lhs, const expat_doc& rhs);

}
}

#endif
