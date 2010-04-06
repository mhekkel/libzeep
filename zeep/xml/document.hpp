//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_DOCUMENT_H
#define SOAP_XML_DOCUMENT_H

#include <boost/filesystem/path.hpp>

#include "zeep/xml/node.hpp"
#include "zeep/xml/unicode_support.hpp"

namespace zeep { namespace xml {

class document : public element
{
  public:
						document();
						document(const std::string& s);
						document(std::istream& is);

	virtual				~document();

	virtual document*	doc();
	virtual const document*
						doc() const;

	// I/O
	void				read(const std::string& s);
	void				read(std::istream& is);
	void				read(std::istream& is, const boost::filesystem::path& base_dir);

	virtual void		write(writer& w) const;

	// A valid xml document contains exactly one root element
	void				root(element* root);
	element*			root() const;

	// Compare two xml documents
	bool				operator==(const document& doc) const;

	bool				operator!=(const document& doc) const		{ return not operator==(doc); }
	
	// In case the document needs to locate included DTD files
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
	struct document_imp*m_impl;
};

std::istream& operator>>(std::istream& lhs, document& rhs);
std::ostream& operator<<(std::ostream& lhs, const document& rhs);

}
}

#endif
