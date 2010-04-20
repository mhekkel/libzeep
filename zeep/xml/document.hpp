//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_DOCUMENT_H
#define SOAP_XML_DOCUMENT_H

#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>

#include "zeep/config.hpp"
#include "zeep/xml/node.hpp"
#include "zeep/xml/unicode_support.hpp"

namespace zeep { namespace xml {

// You can choose between the libzeep parser and expat.
enum parser_type
{
	parser_zeep,
#if SOAP_XML_HAS_EXPAT_SUPPORT
	parser_expat
#endif
};

struct document_imp;

class document
{
  public:
						document();
						document(const std::string& s);
						document(std::istream& is);

	virtual				~document();

	// I/O
	void				read(const std::string& s);
	void				read(std::istream& is);
	void				read(std::istream& is, const boost::filesystem::path& base_dir);

	virtual void		write(writer& w) const;

	// A valid xml document contains exactly one root element
	root_node*			root() const;
	
	// and this root has one child xml::element
	element*			child() const;
	void				child(element* e);
	
	// helper functions
	element_set			find(const std::string& path);
	element*			find_first(const std::string& path);

	// Compare two xml documents
	bool				operator==(const document& doc) const;
	bool				operator!=(const document& doc) const		{ return not operator==(doc); }
	
	// In case the document needs to locate included DTD files
	void				base_dir(const boost::filesystem::path& path);

	// The default for libzeep is to locate the external reference based
	// on sysid and base_dir. Only local files are loaded this way.
	// You can specify a entity loader here.
	boost::function<std::istream*(const std::string& base, const std::string& pubid, const std::string& sysid)>
						external_entity_ref_handler;

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

	// suppress writing out comments
	bool				no_comment() const;
	void				no_comment(bool no_comment);

	// option for parsing
	void				set_validating(bool validate);
	
	// By default, libzeep uses its own parser implementation. But if you prefer
	// expat you can specify that here.
	static void			set_parser_type(parser_type type);

  protected:
						document(struct document_imp* impl);

	static parser_type	s_parser_type;
	document_imp*		create_imp(document* doc);

	document_imp*		m_impl;

  private:
						document(const document&);
	document&			operator=(const document&);
};

std::istream& operator>>(std::istream& lhs, document& rhs);
std::ostream& operator<<(std::ostream& lhs, const document& rhs);

}
}

#endif
