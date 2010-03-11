//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_PARSER_H
#define SOAP_XML_PARSER_H

#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>

#include "zeep/xml/node.hpp"

namespace zeep { namespace xml {

class parser : public boost::noncopyable
{
  public:
					parser(
						std::istream&		data);
					
					parser(
						const std::string&	data);

	virtual			~parser();

	boost::function<void(const std::string& name,
				const attribute_list& atts)>				start_element_handler;

	boost::function<void(const std::string& name)>			end_element_handler;

	boost::function<void(const std::string& data)>			character_data_handler;
	
	boost::function<void(const std::string& target,
				const std::string& data)>					processing_instruction_handler;

	boost::function<void(const std::string& data)>			comment_handler;

	boost::function<void()>									start_cdata_section_handler;

	boost::function<void()>									end_cdata_section_handler;

	boost::function<void(const std::string& prefix,
				const std::string& uri)>					start_namespace_decl_handler;

	boost::function<void(const std::string& prefix)>		end_namespace_decl_handler;

	boost::function<bool(const std::string& uri, boost::filesystem::path& path)>
															find_external_dtd;

	void			parse();

  private:
	struct parser_imp*	m_impl;
	std::istream*		m_istream;
};

}
}

#endif
