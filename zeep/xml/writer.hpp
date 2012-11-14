//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_WRITER_HPP
#define SOAP_XML_WRITER_HPP

#include <stack>
#include <zeep/xml/unicode_support.hpp>

namespace zeep { namespace xml {

#ifndef LIBZEEP_DOXYGEN_INVOKED
extern std::string k_empty_string;
#endif

//! Use xml::writer to write XML documents to a stream

//! The zeep::xml::writer class is used to write XML documents. It has
//! several options that influence the way the data is written. E.g. it
//! is possible to specify whether to wrap the elements and what indentation
//! to use. The writer keeps track of opened elements and therefore knows
//! how to close an element.
//! 
//! The writer writes out XML 1.0 files. 

class writer
{
  public:
					//! The constructor takes a std::ostream as argument
					writer(std::ostream& os);
					writer(std::ostream& os, bool write_decl, bool standalone = false);
					
	virtual			~writer();

	// behaviour
	//! set_encoding is not yet implemented, we only support UTF-8 for now
//	void			set_encoding(encoding_type enc)					{ m_encoding = enc; }

	//! the xml declaration flag (<?xml version...) is not written by default
	void			set_xml_decl(bool flag)							{ m_write_xml_decl = flag; }

	//! the indentation in number of spaces, default is two.
	void			set_indent(int indentation)						{ m_indent = indentation; }

	//! default is to wrap XML files
	void			set_wrap(bool flag)								{ m_wrap = flag; }

	//! default is to wrap XML files
	void			set_wrap_prolog(bool flag)						{ m_wrap_prolog = flag; }

	//! if the trim flag is set, all whitespace will be trimmed to one space exactly
	void			set_trim(bool flag)								{ m_trim = flag; }

	//! collapsing empty elements (<empyt/>) is the default behaviour
	void			set_collapse_empty_elements(bool collapse)		{ m_collapse_empty = collapse; }

	//! escape whitespace into character refences can be specified.
	void			set_escape_whitespace(bool escape)				{ m_escape_whitespace = escape; }

	//! do not write out comments
	void			set_no_comment(bool no_comment)					{ m_no_comment = no_comment; }

	// actual writing routines
	
	//! write a xml declaration, version will be 1.0, standalone can be specified.
	virtual void	xml_decl(bool standalone);

	//! To write an empty DOCTYPE specifying an external subset
	virtual void	empty_doctype(const std::string& root, const std::string& dtd);

	//! This opens a DOCTYPE declaration. The root parameter is the name of the base element.
	virtual void	doctype(const std::string& root, const std::string& pubid, const std::string& dtd);
	virtual void	start_doctype(const std::string& root, const std::string& dtd);

	//! To write a NOTATION declaration
	virtual void	notation(const std::string& name,
						const std::string& sysid, const std::string& pubid);

	//! End a DOCTYPE declaration.
	virtual void	end_doctype();
	
					// writing elements
	virtual void	start_element(const std::string& name);
	virtual void	end_element();
	virtual void	attribute(const std::string& name, const std::string& value);
	virtual void	content(const std::string& content);
	
	void			element(const std::string& name, const std::string& s)
					{
						start_element(name);
						content(s);
						end_element();
					}
	
	virtual void	cdata(const std::string& text);
	virtual void	comment(const std::string& text);

	virtual void	processing_instruction(const std::string& target,
						const std::string& text);

  protected:

	std::ostream&	m_os;
	encoding_type	m_encoding;
	float			m_version;
	bool			m_write_xml_decl;
	bool			m_wrap;
	bool			m_wrap_prolog;
	bool			m_collapse_empty;
	bool			m_escape_whitespace;
	bool			m_trim;
	bool			m_no_comment;
	int				m_indent;
	int				m_level;
	bool			m_element_open;
	std::stack<std::string>
					m_stack;
	bool			m_wrote_element;
	bool			m_prolog;

#ifndef LIBZEEP_DOXYGEN_INVOKED
  private:
					writer(const writer&);
	writer&			operator=(const writer&);
#endif
};

}
}

#endif
