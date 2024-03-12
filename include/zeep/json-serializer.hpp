//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the serializer classes that help serialize data into and out of zeep::nlohmann::json (JSON) objects

#include <zeep/config.hpp>
#include <zeep/nvp.hpp>
#include <zeep/type-traits.hpp>
#include <zeep/value-serializer.hpp>

#include <nlohmann/json.hpp>

namespace nlohmann
{

template <typename T>
struct adl_serializer<std::optional<T>>
{
	static void to_json(json &j, const std::optional<T> &opt)
	{
		if (opt.has_value())
			j = opt.value();
		else
			j = nullptr;
	}

	static void from_json(const json &j, std::optional<T> &opt)
	{
		if (not j.is_null())
			opt = j.get<T>();
	}
};

} // namespace nlohmann

namespace zeep::json
{

struct serializer
{
	template <typename T>
	serializer &operator&(name_value_pair<T> &&nvp)
	{
		serialize(nvp.name(), nvp.value());
		return *this;
	}

	template <typename T>
	void serialize(const char *name, const T &data)
	{
		nlohmann::json e;
		nlohmann::adl_serializer<T>::to_json(e, data);
		m_json.emplace(name, std::move(e));
	}

	nlohmann::json m_json;
};

struct deserializer
{
	deserializer(const nlohmann::json &json)
		: m_json(json)
	{
	}

	template <typename T>
	deserializer &operator&(name_value_pair<T> &&nvp)
	{
		deserialize(nvp.name(), nvp.value());
		return *this;
	}

	template <typename T>
	void deserialize(const char *name, T &data)
	{
		if (not m_json.is_object() or m_json.empty() or not m_json.contains(name))
			return;

		nlohmann::adl_serializer<T>::from_json(m_json[name], data);
	}

	const nlohmann::json &m_json;
};

} // namespace zeep::json

namespace nlohmann
{

template <typename T>
	requires zeep::has_serialize_v<T, zeep::json::serializer>
struct adl_serializer<T>
{
	static void to_json(json &j, const T &value)
	{
		zeep::json::serializer sr;
		const_cast<T &>(value).serialize(sr, 0);
		std::swap(sr.m_json, j);
	}

	static void from_json(const json &j, T &value)
	{
		zeep::json::deserializer sr(j);
		value.serialize(sr, 0);
	}
};

} // namespace nlohmann
