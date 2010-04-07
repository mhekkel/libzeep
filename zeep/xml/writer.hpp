//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_WRITER_HPP
#define SOAP_XML_WRITER_HPP

#include "zeep/xml/unicode_support.hpp"

namespace zeep { namespace xml {

typedef std::list<std::pair<std::string,std::string> >	attribute_list;

class writer
{
  public:
					writer(std::ostream& os);
					
	virtual			~writer();

	// behaviour
	void			encoding(encoding_type enc)					{ m_encoding = enc; }
	void			xml_decl(bool flag)							{ m_write_xml_decl = flag; }
	void			indent(int indentation)						{ m_indent = indentation; }
	void			wrap(bool flag)								{ m_wrap = flag; }
	void			trim(bool flag)								{ m_trim = flag; }
	void			collapse_empty_elements(bool collapse)		{ m_collapse_empty = collapse; }
	void			escape_whitespace(bool escape)				{ m_escape_whitespace = escape; }

	// actual writing routines
	virtual void	write_xml_decl(bool standalone);

	virtual void	write_start_doctype(const std::string& root, const std::string& dtd);

	virtual void	write_end_doctype();
	
	virtual void	write_empty_doctype(const std::string& root, const std::string& dtd);

	virtual void	write_notation(const std::string& name,
						const std::string& sysid, const std::string& pubid);

	virtual void	write_empty_element(const std::string& name,
						const attribute_list& attrs = attribute_list());

	virtual void	write_start_element(const std::string& name,
						const attribute_list& attrs = attribute_list());

	virtual void	write_end_element(const std::string& name);

	virtual void	write_element(const std::string& name, const std::string& content);

	virtual void	write_element(const std::string& name, const attribute_list& attrs,
						const std::string& content);

	virtual void	write_content(const std::string& content);

	virtual void	write_comment(const std::string& text);

	virtual void	write_processing_instruction(const std::string& target,
						const std::string& text);

	virtual void	write_attribute(const std::string& name, const std::string& value);

  protected:

	std::ostream&	m_os;
	encoding_type	m_encoding;
	float			m_version;
	bool			m_write_xml_decl;
	bool			m_wrap;
	bool			m_collapse_empty;
	bool			m_escape_whitespace;
	bool			m_trim;
	int				m_indent;
	int				m_level;
	bool			m_wrote_element;
};

}
}

#endif
