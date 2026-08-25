// SPDX-FileCopyrightText: Maarten L. Hekkelman 2025-2026
// SPDX-License-Identifier: BSL-1.0

#include "zeep/http/scope.hpp"
#include "zeep/exception.hpp"
#include "zeep/http/server.hpp"

#include <ostream>
#include <stdexcept>

namespace zeep::http
{
// --------------------------------------------------------------------
// scope

std::ostream &operator<<(std::ostream &lhs, const scope &rhs)
{
	const scope *s = &rhs;
	while (s != nullptr)
	{
		for (const scope::data_map::value_type& e : s->m_data)
			lhs << e.first << " = " << e.second << '\n';
		s = s->m_next;
	}
	return lhs;
}

scope::scope()
	: m_next(nullptr)
	, m_depth(0)
	, m_req(nullptr)
	, m_server(nullptr)
{
}

scope::scope(const scope &next)
	: m_next(const_cast<scope *>(&next))
	, m_depth(next.m_depth + 1)
	, m_req(next.m_req)
	, m_server(next.m_server)
{
	if (m_depth > 1000)
		throw exception("scope stack overflow");
}

scope::scope(const request &req)
	: m_next(nullptr)
	, m_depth(0)
	, m_req(&req)
	, m_server(nullptr)
{
}

scope::scope(const basic_server *server, const request &req)
	: m_next(nullptr)
	, m_depth(0)
	, m_req(&req)
	, m_server(server)
{
}

void scope::add_path_param(std::string name, std::string value)
{
	m_path_parameters.emplace_back(std::move(name), std::move(value));
}

el::object &scope::operator[](const std::string &name)
{
	return lookup(name);
}

const el::object &scope::lookup(const std::string &name, bool includeSelected) const
{
	const el::object *result = nullptr;

	auto i = m_data.find(name);
	if (i != m_data.end())
		result = &i->second;
	else if (includeSelected and m_selected.contains(name))
		result = &m_selected.at(name);
	else if (m_next != nullptr)
		result = &m_next->lookup(name, includeSelected);

	if (result == nullptr)
	{
		static const el::object s_null;
		result = &s_null;
	}

	return *result;
}

const el::object &scope::operator[](const std::string &name) const
{
	return lookup(name);
}

el::object &scope::lookup(const std::string &name)
{
	el::object *result = nullptr;

	auto i = m_data.find(name);
	if (i != m_data.end())
		result = &i->second;
	else if (m_next != nullptr)
		result = &m_next->lookup(name);

	if (result == nullptr)
	{
		m_data[name] = el::object();
		result = &m_data[name];
	}

	return *result;
}

std::string scope::get_context_name() const
{
	return m_server ? m_server->get_context_name() : "";
}

el::object scope::get_credentials() const
{
	if (m_req == nullptr or m_server == nullptr)
		throw zeep::exception("Invalid scope, no request, no server");
	return m_req->get_credentials();
}

bool scope::has_role(std::string_view role) const
{
	auto credentials = get_credentials();
	return credentials.is_object() and credentials["role"].is_array() and credentials["role"].contains(role);
}

void scope::select_object(const el::object &o)
{
	m_selected = o;
}

auto scope::get_nodeset(const std::string &name) const -> node_set_type
{
	auto i = m_nodesets.find(name);
	if (i != m_nodesets.end())
		return i->second;

	if (m_next == nullptr)
		return {};

	return m_next->get_nodeset(name);
}

void scope::set_nodeset(const std::string &name, node_set_type &&nodes)
{
	m_nodesets.emplace(std::make_pair(name, std::move(nodes)));
}

std::string scope::get_csrf_token() const
{
	return get_request().get_cookie("csrf-token");
}

} // namespace zeep::http
