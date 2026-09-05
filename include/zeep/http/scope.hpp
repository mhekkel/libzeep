// SPDX-FileCopyrightText: Maarten L. Hekkelman 2025-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the scope class that stores variables for template and expression processing

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include "zeep/el/object.hpp"
# include "zeep/http/header.hpp"
# include "zeep/http/request.hpp"

# include <zeem/zeem.hpp>

# include <iosfwd>
# include <locale>
# include <map>
# include <optional>
# include <ranges>
# include <string>
# include <string_view>
# include <type_traits>
# include <utility>
# include <vector>
#endif

namespace zeep::http
{

ZEEP_EXPORT class basic_server;

// --------------------------------------------------------------------

/// \brief The class that stores variables for the current scope
///
/// When processing tags and in expression language constructs we use
/// variables. These are stored in scope instances.

ZEEP_EXPORT class scope
{
  public:
	scope &operator=(const scope &) = delete;

	/// \brief A single parameter, stored as a header (name/value pair)
	using param = header;

	/// \brief simple constructor, used where there's no request available
	scope();

	/// \brief constructor to be used only in debugging
	///
	/// \param req		The incomming HTTP request
	explicit scope(const request &req);

	/// \brief constructor used in a HTTP request context
	///
	/// \param server	The server that handles the incomming request
	/// \param req		The incomming HTTP request
	scope(const basic_server &server, const request &req)
		: scope(&server, req)
	{
	}

	/// \brief constructor used in a HTTP request context
	///
	/// \param server	The server that handles the incomming request, pointer version
	/// \param req		The incomming HTTP request
	scope(const basic_server *server, const request &req);

	/// \brief chaining constructor
	///
	/// Scopes can be nested, introducing new namespaces
	/// \param next	The next scope up the chain.
	explicit scope(const scope &next);

	/// \brief add a path parameter, should only be used by controller::handle_request
	/// \param name   The name of the path parameter
	/// \param value  The value of the path parameter
	void add_path_param(std::string name, std::string value);

	/// \brief Return the list of headers
	[[nodiscard]] const auto &get_headers() const { return get_request().get_headers(); }

	/// \brief Return the named header
	[[nodiscard]] std::string get_header(std::string_view name) const { return get_request().get_header(name); }

	/// \brief Return the payload
	[[nodiscard]] const std::string &get_payload() const { return get_request().get_payload(); }

	/// \brief Return the Accept-Language header value in the request as a std::locale object
	[[nodiscard]] std::locale get_locale() const { return get_request().get_locale(); }

	/// \brief get the optional parameter value for @a name
	[[nodiscard]] std::optional<std::string> get_parameter(std::string_view name) const
	{
		std::optional<std::string> result;

		auto p = std::ranges::find_if(m_path_parameters,
			[name](auto &pp)
			{ return pp.name == name; });

		if (p == m_path_parameters.end())
			result = get_request().get_parameter(name);
		else if (not p->value.empty())
			result = p->value;

		return result;
	}

	/// \brief get all parameter values for @a name
	[[nodiscard]] std::vector<std::string> get_parameters(std::string_view name) const
	{
		auto p = std::ranges::find_if(m_path_parameters,
			[name](auto &pp)
			{ return pp.name == name; });
		if (p != m_path_parameters.end())
			return { p->value };
		else
		{
			std::vector<std::string> result;

			for (const auto &[p_name, p_value] : get_request().get_parameters())
			{
				if (p_name != name)
					continue;

				result.push_back(p_value);
			}

			return result;
		}
	}

	/// \brief get the file parameter value for @a name
	[[nodiscard]] file_param get_file_parameter(std::string name) const
	{
		return get_request().get_file_parameter(std::move(name));
	}

	/// \brief get all file parameters value for @a name
	[[nodiscard]] std::vector<file_param> get_file_parameters(std::string name) const
	{
		return get_request().get_file_parameters(std::move(name));
	}

	/// \brief put variable in the scope with \a name and \a value
	/// \tparam T  A type assignable to \a el::object
	template <typename T>
	void put(const std::string &name, const T &value)
		requires(std::is_assignable_v<el::object, T>)
	{
		m_data[name] = value;
	}

	/// \brief put variable in the scope with \a name and \a value
	/// \param name   The name of the variable
	/// \param value  The value to store
	void put(const std::string &name, el::object &&value)
	{
		m_data[name] = std::move(value);
	}

	/// \brief put variable in the scope with \a name and \a value
	/// \param name   The name of the variable
	/// \param value  The value to store
	void put(const std::string &name, const el::object &value)
	{
		m_data[name] = value;
	}

	/// \brief put variable of type array in the scope with \a name and values from \a begin to \a end
	/// \tparam ForwardIterator  An input iterator type
	/// \param name    The name of the variable
	/// \param begin   Start of the range
	/// \param end     End of the range
	template <typename ForwardIterator>
	void put(const std::string &name, ForwardIterator begin, ForwardIterator end);

	/// \brief return variable with \a name
	///
	/// \param name				The name of the variable to return
	/// \param includeSelected	If this is true, and the variable was not found as a regular variable
	///							in the current scope, the selected el::objects will be searched for members
	///							with \a name This is used by the tag processing lib v2 in _z2:el::object_
	/// \return					The value found or null if there was no such variable.
	[[nodiscard]] const el::object &lookup(const std::string &name, bool includeSelected = false) const;

	/// \brief return variable with \a name
	/// \param name  The name of the variable to return
	/// \return      The value found or null if there was no such variable
	[[nodiscard]] const el::object &operator[](const std::string &name) const;

	/// \brief return variable with \a name
	///
	/// \param name				The name of the variable to return
	/// \return					The value found or null if there was no such variable.
	[[nodiscard]] el::object &lookup(const std::string &name);

	/// \brief return variable with \a name
	/// \param name  The name of the variable to return
	/// \return      The value found or null if there was no such variable
	[[nodiscard]] el::object &operator[](const std::string &name);

	/// \brief return the HTTP request, will throw if the scope chain was not created with a request
	[[nodiscard]] const request &get_request() const
	{
		if (m_req == nullptr)
			throw zeep::exception("Invalid scope, no request");
		return *m_req;
	}

	/// \brief return the context_name of the server
	[[nodiscard]] uri get_context_path() const;

	/// \brief return the credentials of the current user
	[[nodiscard]] el::object get_credentials() const;

	/// \brief returns whether the current user has role \a role
	/// \param role  The role to check
	/// \result      True if the current user has the specified role
	[[nodiscard]] bool has_role(std::string_view role) const;

	/// \brief select el::object \a o , used in z2:el::object constructs
	/// \param o  The object to select
	void select_object(const el::object &o);

	/// \brief a nodeset for a selector, cached to avoid recusive expansion
	///
	/// In tag processors it is sometimes needed to take a selection of zeem::nodes
	/// and reuse these, as a copy when inserting templates e.g.
	using node_set_type = zeem::element;

	/// \brief return the node_set_type with name \a name
	/// \param name  The name of the nodeset
	/// \return      The nodeset associated with \a name
	[[nodiscard]] node_set_type get_nodeset(const std::string &name) const;

	/// \brief store node_set_type \a nodes with name \a name
	/// \param name   The name to associate with the nodeset
	/// \param nodes  The nodeset to store
	void set_nodeset(const std::string &name, node_set_type &&nodes);

	/// \brief return whether a node_set with name \a name is stored
	[[nodiscard]] bool has_nodeset(const std::string &name) const
	{
		return m_nodesets.count(name) or (m_next != nullptr and m_next->has_nodeset(name));
	}

	/// \brief get the CSRF token from the request burried in \a scope
	[[nodiscard]] std::string get_csrf_token() const;

  private:
	/// @cond

	/// for debugging purposes
	friend std::ostream &operator<<(std::ostream &lhs, const scope &rhs);

	using data_map = std::map<std::string, el::object>;

	data_map m_data;
	scope *m_next;
	unsigned m_depth;
	const request *m_req;
	const basic_server *m_server;

	std::vector<param> m_path_parameters;

	el::object m_selected;

	using nodeset_map = std::map<std::string, node_set_type>;

	nodeset_map m_nodesets;

	/// @endcond
};

template <typename ForwardIterator>
inline void scope::put(const std::string &name, ForwardIterator begin, ForwardIterator end)
{
	std::vector<el::object> elements;
	while (begin != end)
		elements.push_back(el::object(*begin++));
	m_data.emplace(name, std::move(elements));
}

// --------------------------------------------------------------------
} // namespace zeep::http
