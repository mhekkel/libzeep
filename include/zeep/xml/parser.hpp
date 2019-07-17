// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_PARSER_H
#define SOAP_XML_PARSER_H

#include <functional>

#include <zeep/xml/node.hpp>
#include <zeep/exception.hpp>

namespace zeep
{
namespace xml
{

namespace detail
{

struct attr
{
	std::string m_ns;
	std::string m_name;
	std::string m_value;
	bool m_id; // whether the attribute is defined as type ID in its ATTLIST decl
};

} // namespace detail

/// If an invalid_exception is thrown, it means the XML document is not valid: it does
/// not conform the DTD specified in the XML document.
/// This is only thrown when validation is enabled.
///
/// The what() member of the exception object will contain an explanation.

class invalid_exception : public zeep::exception
{
public:
	invalid_exception(const std::string& msg) : exception(msg) {}
	~invalid_exception() throw() {}
};

/// If an not_wf_exception is thrown, it means the XML document is not well formed.
/// Often this means syntax errors, missing \< or \> characters, non matching open
/// and close tags, etc.
///
/// The what() member of the exception object will contain an explanation.

class not_wf_exception : public zeep::exception
{
public:
	not_wf_exception(const std::string& msg) : exception(msg) {}
	~not_wf_exception() throw() {}
};

/// zeep::xml::parser is a SAX parser. After construction, you should assign
/// call back handlers for the SAX events and then call parse().

class parser
{
public:
#ifndef LIBZEEP_DOXYGEN_INVOKED
	typedef detail::attr attr_type;
	typedef std::list<detail::attr> attr_list_type;
#endif

	parser(std::istream& is);
	parser(const std::string& s);

	virtual ~parser();

	std::function<void(const std::string& name, const std::string& uri, const attr_list_type& atts)>
		start_element_handler;

	std::function<void(const std::string& name, const std::string& uri)>
		end_element_handler;

	std::function<void(const std::string& data)> character_data_handler;

	std::function<void(const std::string& target,
					   const std::string& data)>
		processing_instruction_handler;

	std::function<void(const std::string& data)> comment_handler;

	std::function<void()> start_cdata_section_handler;

	std::function<void()> end_cdata_section_handler;

	std::function<void(const std::string& prefix,
					   const std::string& uri)>
		start_namespace_decl_handler;

	std::function<void(const std::string& prefix)> end_namespace_decl_handler;

	std::function<void(const std::string& name,
					   const std::string& systemId, const std::string& publicId)>
		notation_decl_handler;

	std::function<std::istream *(const std::string& base, const std::string& pubid, const std::string& uri)>
		external_entity_ref_handler;

	std::function<void(const std::string& msg)> report_invalidation_handler;

	void parse(bool validate);

#ifndef LIBZEEP_DOXYGEN_INVOKED
protected:
	friend struct parser_imp;

	virtual void start_element(const std::string& name,
							   const std::string& uri, const attr_list_type& atts);

	virtual void end_element(const std::string& name, const std::string& uri);

	virtual void character_data(const std::string& data);

	virtual void processing_instruction(const std::string& target, const std::string& data);

	virtual void comment(const std::string& data);

	virtual void start_cdata_section();

	virtual void end_cdata_section();

	virtual void start_namespace_decl(const std::string& prefix, const std::string& uri);

	virtual void end_namespace_decl(const std::string& prefix);

	virtual void notation_decl(const std::string& name,
							   const std::string& systemId, const std::string& publicId);

	virtual void report_invalidation(const std::string& msg);

	virtual std::istream*
	external_entity_ref(const std::string& base,
						const std::string& pubid, const std::string& uri);

	struct parser_imp *m_impl;
	std::istream *m_istream;
#endif
};

} // namespace xml
} // namespace zeep

#endif
