//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_WRITER_HPP
#define SOAP_XML_WRITER_HPP

#include "zeep/xml/attribute.hpp"
#include "zeep/xml/unicode_support.hpp"

namespace zeep { namespace xml {

class writer
{
  public:
					writer(std::ostream& os);
					
	virtual			~writer();

	virtual void	write_xml_decl(float version, encoding_type enc);

	virtual void	write_empty_element(const std::string& ns,
						const std::string& name, const attribute_list& attrs);

	virtual void	write_start_element(const std::string& ns,
						const std::string& name, const attribute_list& attrs);

	virtual void	write_end_element(const std::string& ns,
						const std::string& name);

	virtual void	write_comment(const std::string& text);

	virtual void	write_processing_instruction(const std::string& target,
						const std::string& text);

	virtual void	write_text(const std::string& text);

  private:
	std::ostream&	m_os;
};

}
}

#endif
