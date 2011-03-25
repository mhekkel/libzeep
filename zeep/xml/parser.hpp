//  Copyright Maarten L. Hekkelman, Radboud University 2010-2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_PARSER_H
#define SOAP_XML_PARSER_H

#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>

#include <zeep/xml/node.hpp>

namespace zeep { namespace xml {

namespace detail {

struct attr
{
	std::string	m_ns;
	std::string	m_name;
	std::string	m_value;
	bool		m_id;		// whether the attribute is defined as type ID in its ATTLIST decl
};

}

class invalid_exception : public zeep::exception
{
  public:
	invalid_exception(const std::string& msg) : exception(msg) {}
	~invalid_exception() throw () {}
};

class not_wf_exception : public zeep::exception
{
  public:
	not_wf_exception(const std::string& msg) : exception(msg) {}
	~not_wf_exception() throw () {}
};

class parser
{
  public:
	typedef detail::attr								attr_type;
	typedef std::list<detail::attr>						attr_list_type;
	
	
						parser(std::istream& is);
						parser(const std::string& s);

	virtual				~parser();
	
	boost::function<void(const std::string& name, const std::string& uri,
				const attr_list_type& atts)>		start_element_handler;

	boost::function<void(const std::string& name, const std::string& uri)>
															end_element_handler;

	boost::function<void(const std::string& data)>			character_data_handler;
	
	boost::function<void(const std::string& target,
				const std::string& data)>					processing_instruction_handler;

	boost::function<void(const std::string& data)>			comment_handler;

	boost::function<void()>									start_cdata_section_handler;

	boost::function<void()>									end_cdata_section_handler;

	boost::function<void(const std::string& prefix,
				const std::string& uri)>					start_namespace_decl_handler;

	boost::function<void(const std::string& prefix)>		end_namespace_decl_handler;

	boost::function<void(const std::string& name,
		const std::string& systemId, const std::string& publicId)>
															notation_decl_handler;

	boost::function<std::istream*(const std::string& base, const std::string& pubid, const std::string& uri)>
															external_entity_ref_handler;

	boost::function<void(const std::string& msg)>			report_invalidation_handler;

	void					parse(bool validate);

  protected:
	friend struct parser_imp;

	virtual void		start_element(const std::string& name,
							const std::string& uri, const attr_list_type& atts);

	virtual void		end_element(const std::string& name, const std::string& uri);

	virtual void		character_data(const std::string& data);
	
	virtual void		processing_instruction(const std::string& target, const std::string& data);

	virtual void		comment(const std::string& data);

	virtual void		start_cdata_section();

	virtual void		end_cdata_section();

	virtual void		start_namespace_decl(const std::string& prefix, const std::string& uri);

	virtual void		end_namespace_decl(const std::string& prefix);

	virtual void		notation_decl(const std::string& name,
							const std::string& systemId, const std::string& publicId);
	
	virtual void		report_invalidation(const std::string& msg);

	virtual std::istream*
						external_entity_ref(const std::string& base,
							const std::string& pubid, const std::string& uri);

	struct parser_imp*	m_impl;
	std::istream*		m_istream;
};

}
}

#endif
