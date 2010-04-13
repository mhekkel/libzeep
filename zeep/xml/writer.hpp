//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_WRITER_HPP
#define SOAP_XML_WRITER_HPP

#include <stack>
#include "zeep/xml/unicode_support.hpp"

namespace zeep { namespace xml {

extern std::string k_empty_string;

class writer
{
  public:
					writer(std::ostream& os);
					
	virtual			~writer();

	// behaviour
	void			set_encoding(encoding_type enc)					{ m_encoding = enc; }
	void			set_xml_decl(bool flag)							{ m_write_xml_decl = flag; }
	void			set_indent(int indentation)						{ m_indent = indentation; }
	void			set_wrap(bool flag)								{ m_wrap = flag; }
	void			set_trim(bool flag)								{ m_trim = flag; }
	void			set_collapse_empty_elements(bool collapse)		{ m_collapse_empty = collapse; }
	void			set_escape_whitespace(bool escape)				{ m_escape_whitespace = escape; }

	// actual writing routines
	virtual void	xml_decl(bool standalone);

	virtual void	start_doctype(const std::string& root, const std::string& dtd);

	virtual void	end_doctype();
	
	virtual void	empty_doctype(const std::string& root, const std::string& dtd);

	virtual void	notation(const std::string& name,
						const std::string& sysid, const std::string& pubid);

					// ... but more often you will want to use the following
	virtual void	start_element(const std::string& name);
	virtual void	end_element();
	virtual void	attribute(const std::string& name, const std::string& value);
	virtual void	content(const std::string& content);

					// a couple of convenience routines to do all the four above in one call
	typedef std::pair<std::string,std::string>		attrib;
	virtual void	element(const std::string& name, const std::string& s = k_empty_string);
	virtual void	element(const std::string& name, const attrib& a1, const std::string& s = k_empty_string);
	virtual void	element(const std::string& name, const attrib& a1, const attrib& a2, const std::string& s = k_empty_string);
	virtual void	element(const std::string& name, const attrib& a1, const attrib& a2, const attrib& a3, const std::string& s = k_empty_string);
	virtual void	element(const std::string& name, const attrib& a1, const attrib& a2, const attrib& a3, const attrib& a4, const std::string& s = k_empty_string);

	virtual void	comment(const std::string& text);

	virtual void	processing_instruction(const std::string& target,
						const std::string& text);

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
	bool			m_element_open;
	std::stack<std::string>
					m_stack;
	bool			m_wrote_element;
};

class write_element
{
  public:
				write_element(writer& w, const std::string& name)
					: m_writer(w), m_name(name)
				{
					m_writer.start_element(name);
				}

				~write_element()
				{
					m_writer.end_element();
				}

  private:
	writer&		m_writer;
	std::string	m_name;
};

}
}

#endif
