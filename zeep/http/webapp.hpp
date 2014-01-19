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
#include <zeep/http/template_processor.hpp>

// --------------------------------------------------------------------
//

namespace zeep {
namespace http {

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
	char		m_realm[256];			///< Realm for which the authorization failed
};

#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
namespace el { class scope; class object; }
#endif

/// webapp is a specialization of zeep::http::server, it is used to create
/// interactive web applications.

class webapp : public http::server, public template_processor
{
  public:
					/// first parameter to constructor is the
					/// namespace to use in template XHTML files.
					webapp(const std::string& ns = "http://www.cmbi.ru.nl/libzeep/ml",
						const boost::filesystem::path& docroot = ".");

	virtual			~webapp();

  protected:
	
	virtual void	handle_request(const request& req, reply& rep);
	virtual void	create_unauth_reply(bool stale, const std::string& realm, reply& rep);

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
	void			mount(const std::string& path, handler_type handler);

	/// Default handler for serving files out of our doc root
	virtual void	handle_file(const request& request, const el::scope& scope, reply& reply);

	/// Use load_template to fetch the XHTML template file
	virtual void	load_template(const std::string& file, xml::document& doc);

	/// Return a parameter_map containing the cookies as found in the current request
	virtual void	get_cookies(const el::scope& scope, parameter_map& cookies);

  private:
	typedef std::map<std::string,handler_type>		handler_map;
	
	handler_map		m_dispatch_table;
};

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

