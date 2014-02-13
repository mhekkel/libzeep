// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
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

/// webapps can use authentication, this exception is thrown for unauthorized access

struct unauthorized_exception : public std::exception
{
				unauthorized_exception(bool stale, const std::string& realm)
					: m_stale(stale)
				{
					std::string::size_type n = realm.length();
					if (n >= sizeof(m_realm))
						n = sizeof(m_realm) - 1;
					realm.copy(m_realm, n);
					m_realm[n] = 0;
				}
	bool		m_stale;			///< Is true when the authorization information is valid but stale (too old)
	char		m_realm[256];		///< Realm for which the authorization failed
};

#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
namespace el { class scope; class object; }
#endif

/// basic_webapp is used to create XHTML web pages based on the contents of a
/// template file and the parameters passed in the request and calculated data stored
/// in a scope object.

/// class to use in authentication
struct auth_info;
typedef std::list<auth_info> auth_info_list;

class basic_webapp
{
  public:
	/// first parameter to constructor is the
	/// namespace to use in template XHTML files.
	basic_webapp(const std::string& ns = "http://www.cmbi.ru.nl/libzeep/ml",
		const boost::filesystem::path& docroot = ".");

	virtual ~basic_webapp();

	virtual void set_docroot(const boost::filesystem::path& docroot);
	boost::filesystem::path get_docroot() const		{ return m_docroot; }

	/// Support for HTTP Authentication, sets the username field in the request
	virtual void validate_authentication(request& request, const std::string& realm)
	{
		request.username = validate_authentication(request.get_header("Authorization"), request.method, request.uri, realm);
	}

	/// Support for HTTP Authentication, returns the validated user name
	virtual std::string validate_authentication(const std::string& authorization,
		const std::string& method, const std::string& uri, const std::string& realm);

	/// Subclasses should implement this to return the password for the user, 
	/// result should be the MD5 hash of the string username + ':' + realm + ':' + password
	virtual std::string get_hashed_password(const std::string& username, const std::string& realm);

	/// Create an error reply for the error containing a validation header
	virtual void create_unauth_reply(const request& req, bool stale, const std::string& realm,
		const std::string& authentication, reply& rep);

	/// Create an error reply for the error
	virtual void create_error_reply(const request& req, status_type status, reply& rep);

	/// Create an error reply for the error with an additional message for the user
	virtual void create_error_reply(const request& req, status_type status, const std::string& message, reply& rep);

	/// Dispatch and handle the request
	virtual void handle_request(const request& req, reply& rep);

  protected:

	virtual void create_unauth_reply(const request& req, bool stale, const std::string& realm,
		reply& rep)
	{
		create_unauth_reply(req, stale, realm, "WWW-Authenticate", rep);
	}
	
	// webapp works with 'handlers' that are methods 'mounted' on a path in the requested URI
	
	typedef boost::function<void(const request& request, const el::scope& scope, reply& reply)> handler_type;

	/// assign a handler function to a path in the server's namespace
	/// Usually called like this:
	///
	///   mount("page", boost::bind(&page_handler, this, _1, _2, _3));
	///
	/// Where page_handler is defined as:
	///
	/// void my_server::page_handler(const request& request, const el::scope& scope, reply& reply);
	///
	void mount(const std::string& path, handler_type handler);
	
	/// version of mount that requires authentication
	void mount(const std::string& path, const std::string& realm, handler_type handler);

	/// Default handler for serving files out of our doc root
	virtual void handle_file(const request& request, const el::scope& scope, reply& reply);

	/// Use load_template to fetch the XHTML template file
	virtual void load_template(const std::string& file, xml::document& doc);
	void load_template(const boost::filesystem::path& file, xml::document& doc)
	{
		load_template(file.string(), doc);
	}

	/// Return a parameter_map containing the cookies as found in the current request
	virtual void get_cookies(const el::scope& scope, parameter_map& cookies);

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

	struct mount_point
	{
		std::string path;
		std::string realm;
		handler_type handler;
	};

	typedef std::vector<mount_point> mount_point_list;
	typedef std::map<std::string,processor_type> processor_map;
	
	mount_point_list m_dispatch_table;
	std::string m_ns;
	boost::filesystem::path m_docroot;
	processor_map m_processor_table;
	auth_info_list m_auth_info;
	boost::mutex m_auth_mutex;
};

// --------------------------------------------------------------------

/// webapp is derived from both zeep::http::server and basic_webapp, it is used to create
/// interactive web applications.

class webapp : public http::server, public basic_webapp
{
  public:
	webapp(const std::string& ns = "http://www.cmbi.ru.nl/libzeep/ml",
		const boost::filesystem::path& docroot = ".");
	virtual ~webapp();

	virtual void handle_request(const request& req, reply& rep);
};

// --------------------------------------------------------------------

template<class T>
inline
T parameter_value::as() const
{
	T result;
	
	if (boost::is_arithmetic<T>::value and m_v.empty())
		result = 0;
	else
		result = boost::lexical_cast<T>(m_v);

	return result;
}

template<>
inline
std::string parameter_value::as<std::string>() const
{
	return m_v;
}

template<>
inline
bool parameter_value::as<bool>() const
{
	bool result = false;
	
	if (not m_v.empty() and m_v != "false")
	{
		if (m_v == "true")
			result = true;
		else
		{
			try { result = boost::lexical_cast<int>(m_v) != 0; } catch (...) {}
		}
	}
	
	return result;
}

template<class T>
inline
const parameter_value&
parameter_map::get(
	const std::string&	name,
	T					defaultValue)
{
	iterator i = lower_bound(name);
	if (i == end() or i->first != name)
		i = insert(std::make_pair(name, parameter_value(boost::lexical_cast<std::string>(defaultValue), true)));
	return i->second;
}

// specialisation for const char*
template<>
inline
const parameter_value&
parameter_map::get(
	const std::string&	name,
	const char*			defaultValue)
{
	if (defaultValue == nullptr)
		defaultValue = "";
	
	iterator i = lower_bound(name);
	if (i == end() or i->first != name)
		i = insert(std::make_pair(name, parameter_value(defaultValue, true)));
	else if (i->second.empty())
		i->second = parameter_value(defaultValue, true);

	return i->second;
}

// specialisation for bool (if missing, value is false)
template<>
inline
const parameter_value&
parameter_map::get(
	const std::string&	name,
	bool				defaultValue)
{
	iterator i = lower_bound(name);
	if (i == end() or i->first != name)
		i = insert(std::make_pair(name, parameter_value("false", true)));
	else if (i->second.empty())
		i->second = parameter_value("false", true);

	return i->second;
}

}
}

