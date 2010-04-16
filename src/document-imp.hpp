//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_DOCUMENT_IMP_H
#define SOAP_XML_DOCUMENT_IMP_H

#include <iostream>

#include "zeep/xml/document.hpp"

namespace zeep { namespace xml {

struct document_imp
{
					document_imp(document* doc);
	virtual			~document_imp();

	virtual void	parse(std::istream& data) = 0;

	std::string		prefix_for_namespace(const std::string& ns);
	
	bool			find_external_dtd(const std::string& uri,
						boost::filesystem::path& path);

	root_node		m_root;
	boost::filesystem::path
					m_dtd_dir;
	
	// some content information
	encoding_type	m_encoding;
	bool			m_standalone;
	int				m_indent;
	bool			m_empty;
	bool			m_wrap;
	bool			m_trim;
	bool			m_escape_whitespace;
	
	bool			m_validating;

	struct notation
	{
		std::string	m_name;
		std::string	m_sysid;
		std::string	m_pubid;
	};

	document*		m_doc;
	element*		m_cur;		// construction
	std::vector<std::pair<std::string,std::string> >
					m_namespaces;
	std::list<notation>	m_notations;
};

}
}

#endif
