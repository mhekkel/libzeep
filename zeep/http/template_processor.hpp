//            Copyright Maarten L. Hekkelman, 2014.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
//	Split off out of webapp, to allow usage without the need of an
//	http server instance.
//

#pragma once

#include <map>
#include <string>
#include <vector>

#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include <zeep/exception.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/webapp/el.hpp>

// --------------------------------------------------------------------
//

namespace zeep {
namespace http {

class parameter_value
{
  public:
				parameter_value() : m_defaulted(false) {}
				parameter_value(const std::string& v, bool defaulted)
					: m_v(v), m_defaulted(defaulted) {}

	template<class T>
	T			as() const;
	bool		empty() const								{ return m_v.empty(); }
	bool		defaulted() const							{ return m_defaulted; }

  private:
	std::string	m_v;
	bool		m_defaulted;
};

/// parameter_map is used to pass parameters from forms. The parameters can have 'any' type.
/// Works a bit like the program_options code in boost.

class parameter_map : public std::multimap<std::string, parameter_value>
{
  public:
	
	/// add a name/value pair as a string formatted as 'name=value'
	void		add(const std::string& param);
	void		add(std::string name, std::string value);
	void		replace(std::string name, std::string value);

	template<class T>
	const parameter_value&
				get(const std::string& name, T defaultValue);

};

/// template_processor is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

class template_processor
{
  public:
	template_processor(const std::string& ns = "http://www.cmbi.ru.nl/libzeep/ml",
		const boost::filesystem::path& docroot = ".");

	virtual ~template_processor();

	virtual void set_docroot(const boost::filesystem::path& docroot);
	boost::filesystem::path get_docroot() const		{ return m_docroot; }
	
  protected:

	/// Use load_template to fetch the XHTML template file
	virtual void load_template(const std::string& file, xml::document& doc);
	void load_template(const boost::filesystem::path& file, xml::document& doc)
	{
		load_template(file.string(), doc);
	}

	/// create a reply based on a template
	virtual void create_reply_from_template(const std::string& file, const el::scope& scope, reply& reply);
	
	/// process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void process_xml(xml::node* node, const el::scope& scope, boost::filesystem::path dir);

	typedef boost::function<void(xml::element* node, const el::scope& scope, boost::filesystem::path dir)> processor_type;

	/// To add additional processors
	virtual void add_processor(const std::string& name, processor_type processor);

	/// default handler for mrs:include tags
	virtual void process_include(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:if tags
	virtual void process_if(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:iterate tags
	virtual void process_iterate(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:for tags
	virtual void process_for(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:number tags
	virtual void process_number(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:options tags
	virtual void process_options(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:option tags
	virtual void process_option(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:checkbox tags
	virtual void process_checkbox(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:url tags
	virtual void process_url(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:param tags
	virtual void process_param(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// default handler for mrs:embed tags
	virtual void process_embed(xml::element* node, const el::scope& scope, boost::filesystem::path dir);

	/// Initialize the el::scope object
	virtual void init_scope(el::scope& scope);

	/// Return the original parameters as found in the current request
	virtual void get_parameters(const el::scope& scope, parameter_map& parameters);

  private:
	typedef std::map<std::string,processor_type>	processor_map;
	
	std::string m_ns;
	boost::filesystem::path m_docroot;
	processor_map m_processor_table;
};

}
}
