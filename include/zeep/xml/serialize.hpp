// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <regex>
#include <experimental/type_traits>
#include <optional>

#include <zeep/xml/node.hpp>
#include <zeep/exception.hpp>
#include <zeep/serialize.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <zeep/type_traits.hpp>

namespace zeep::xml
{

struct serializer;
struct deserializer;

template<typename T>
struct element_nvp : public name_value_pair<T>
{
	explicit element_nvp(const char* name, T& v) : name_value_pair<T>(name, v) {}
	element_nvp(const element_nvp& rhs) : name_value_pair<T>(rhs) {}
};

template<typename T>
struct attribute_nvp : public name_value_pair<T>
{
	explicit attribute_nvp(const char* name, T& v) : name_value_pair<T>(name, v) {}
	attribute_nvp(const attribute_nvp& rhs) : name_value_pair<T>(rhs) {}
};

template<typename T>
inline element_nvp<T> make_element_nvp(const char* name, T& v)
{
	return element_nvp<T>(name, v);
}
	
template<typename T>
inline attribute_nvp<T> make_attribute_nvp(const char* name, T& v)
{
	return attribute_nvp<T>(name, v);
}

#define ZEEP_ELEMENT_NAME_VALUE(name)		zeep::xml::make_element_nvp(#name, name)
#define ZEEP_ATTRIBUTE_NAME_VALUE(name)		zeep::xml::make_attribute_nvp(#name, name)
	
/// serializer, deserializer and schema_creator are classes that can be used
/// to initiate the serialization. They are the Archive classes that are
/// the first parameter to the templated function 'serialize' in the classes
/// that can be serialized. (See boost::serialization for more info).

/// serializer is the class that initiates the serialization process.

struct serializer
{
	serializer(element& node) : m_node(node) {}

	template<typename T>
	serializer& operator&(const name_value_pair<T>& rhs)
	{
		return serialize_element(rhs.name(), rhs.value());
	}
	
	template<typename T>
	serializer& operator&(const element_nvp<T>& rhs)
	{
		return serialize_element(rhs.name(), rhs.value());
	}
	
	template<typename T>
	serializer& operator&(const attribute_nvp<T>& rhs)
	{
		return serialize_attribute(rhs.name(), rhs.value());
	}
	
	template<typename T>
	serializer& serialize_element(const T& data);

	template<typename T>
	serializer& serialize_element(const char* name, const T& data);

	template<typename T>
	serializer& serialize_attribute(const char* name, const T& data);

	element& m_node;
};

/// deserializer is the class that initiates the deserialization process.

struct deserializer
{
	deserializer(const element& node) : m_node(node) {}

	template<typename T>
	deserializer& operator&(const name_value_pair<T>& rhs)
	{
		return deserialize_element(rhs.name(), rhs.value());
	}

	template<typename T>
	deserializer& operator&(const element_nvp<T>& rhs)
	{
		return deserialize_element(rhs.name(), rhs.value());
	}
	
	template<typename T>
	deserializer& operator&(const attribute_nvp<T>& rhs)
	{
		return deserialize_attribute(rhs.name(), rhs.value());
	}
	
	template<typename T>
	deserializer& deserialize_element(T& data);

	template<typename T>
	deserializer& deserialize_element(const char* name, T& data);

	template<typename T>
	deserializer& deserialize_attribute(const char* name, T& data);

	const element& m_node;
};

using type_map = std::map<std::string,element>;

/// schema_creator is used by zeep::dispatcher to create schema files.

struct schema_creator
{
	schema_creator(type_map& types, element& node)
		: m_node(node), m_types(types) {}
		
	template<typename T>
	schema_creator& operator&(const name_value_pair<T>& rhs)
	{
		return add_element(rhs.name(), rhs.value());
	}

	template<typename T>
	schema_creator& operator&(const element_nvp<T>& rhs)
	{
		return add_element(rhs.name(), rhs.value());
	}
	
	template<typename T>
	schema_creator& operator&(const attribute_nvp<T>& rhs)
	{
		return add_attribute(rhs.name(), rhs.value());
	}

	template<typename T>
	schema_creator& add_element(const char* name, const T& value);

	template<typename T>
	schema_creator& add_attribute(const char* name, const T& value);

	element& m_node;
	type_map& m_types;
	std::string m_prefix = "ns";
};

// --------------------------------------------------------------------

template<typename T, typename = void>
struct type_serializer
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using value_serializer_type = value_serializer<value_type>;

	static constexpr const char* type_name() { return value_serializer_type::type_name(); }

	static std::string serialize_value(const T& value)
	{
		return value_serializer_type::to_string(value);
	}

	static T deserialize_value(const std::string& value)
	{
		return value_serializer_type::from_string(value);
	}

	static void serialize_child(element& n, const char* name, const value_type& value)
	{
		assert(name);

		if (strlen(name) == 0 or strcmp(name, ".") == 0)
			n.set_content(value_serializer_type::to_string(value));
		else
			n.emplace_back(name).set_content(value_serializer_type::to_string(value));
	}

	static void deserialize_child(const element& n, const char* name, value_type& value)
	{
		assert(name);

		value = {};

		if (strlen(name) == 0 or strcmp(name, ".") == 0)
			value = value_serializer_type::from_string(n.get_content());
		else
		{
			auto e = std::find_if(n.begin(), n.end(), [name](auto& e) { return e.name() == name; });
			if (e != n.end())
				value = value_serializer_type::from_string(e->get_content());
		}
	}

	static element schema(const std::string& name, const std::string& prefix)
	{
		return {
			"xsd:element",
			{
				{ "name", name },
				{ "type", prefix + ':' + type_name() },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" }
			}
		};
	}

	static void register_type(type_map& types)
	{
	}
};

template<typename T, size_t N>
struct type_serializer<T[N]>
{
	using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
	using type_serializer_type = type_serializer<value_type>;

	static constexpr const char* type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element& n, const char* name, const value_type(&value)[N])
	{
		assert(name);

		for (const value_type& v : value)
			type_serializer_type::serialize_child(n, name, v);
	}

	static void deserialize_child(const element& n, const char* name, value_type(&value)[N])
	{
		assert(name);

		size_t ix = 0;
		for (auto& e: n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			value[ix] = std::move(v);
			++ix;

			if (ix >= N)
				break;
		}
	}

	static element schema(const std::string& name, const std::string& prefix)
	{
		element result = type_serializer_type::schema(name, prefix);
		result.set_attribute("minOccurs", std::to_string(N));
		result.set_attribute("maxOccurs", std::to_string(N));
		return result;
	}

	static void register_type(type_map& types)
	{
		type_serializer_type::register_type(types);
	}
};

template<typename T>
struct type_serializer<T, std::enable_if_t<std::is_enum_v<T>>>
	: public value_serializer<T>
{
	using value_type = T;
	using value_serializer_type = value_serializer<T>;
	using value_serializer_type::type_name;

	static std::string serialize_value(const T& value)
	{
		return value_serializer_type::to_string(value);
	}

	static T deserialize_value(const std::string& value)
	{
		return value_serializer_type::from_string(value);
	}

	static void serialize_child(element& n, const char* name, const value_type& value)
	{
		assert(name);

		if (strlen(name) == 0 or strcmp(name, ".") == 0)
			n.set_content(value_serializer_type::to_string(value));
		else
			n.emplace_back(name).set_content(value_serializer_type::to_string(value));
	}

	static void deserialize_child(const element& n, const char* name, value_type& value)
	{
		assert(name);

		value = value_type();

		if (std::strlen(name) == 0 or std::strcmp(name, ".") == 0)
			value = value_serializer_type::from_string(n.get_content());
		else
		{
			auto e = std::find_if(n.begin(), n.end(), [name](auto& e) { return e.name() == name; });
			if (e != n.end())
				value = value_serializer_type::from_string(e->get_content());
		}
	}

	static element schema(const std::string& name, const std::string& prefix)
	{
		return {
			"xsd:element",
			{
				{ "name", name },
				{ "type", prefix + ':' + type_name() },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" }
			}
		};
	}
	
	static void register_type(type_map& types)
	{
		element n("xsd:simpleType",
		{
			{ "name", type_name() }
		});
		
		element restriction("xsd:restriction", { { "base", "xsd:string" } });
		
		for (auto& e: value_serializer_type::instance().m_name_mapping)
		{
			restriction.emplace_back(
				"xsd:enumeration",
				{
					{ "value", e.second }
				});
		}

		n.emplace_back(std::move(restriction));
		types.emplace(type_name(), std::move(n));
	}
};

template<typename T>
struct type_serializer<T, std::enable_if_t<has_serialize_v<T,serializer>>>
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;

	// the name of this type
	std::string m_type_name;

	static const char* type_name()					{ return instance().m_type_name.c_str(); }
	void type_name(const std::string& name)			{ m_type_name = name; }

	static type_serializer& instance()
	{
		static type_serializer s_instance{typeid(value_type).name()};
		return s_instance;
	}

	static void serialize_child(element& n, const char* name, const value_type& value)
	{
		assert(name);

		if (strlen(name) == 0 or strcmp(name, ".") == 0)
		{
			serializer sr(n);
			const_cast<value_type&>(value).serialize(sr, 0Ul);
		}
		else
		{
			element& e = n.emplace_back(name);
			serializer sr(e);
			const_cast<value_type&>(value).serialize(sr, 0Ul);
		}
	}

	static void deserialize_child(const element& n, const char* name, value_type& value)
	{
		assert(name);

		value = value_type();

		if (strlen(name) == 0 or strcmp(name, ".") == 0)
		{
			deserializer sr(n);
			value.serialize(sr, 0UL);
		}
		else
		{
			auto e = std::find_if(n.begin(), n.end(), [name](auto& e) { return e.name() == name; });
			if (e != n.end())
			{
				deserializer sr(*e);
				value.serialize(sr, 0UL);
			}
		}
	}

	static element schema(const std::string& name, const std::string& prefix)
	{
		return {
			"xsd:element",
			{
				{ "name", name },
				{ "type", prefix + ':' + type_name() },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" }
			}
		};
	}
	
	static void register_type(type_map& types)
	{
		element sequence("xsd:sequence");
		schema_creator schema(types, sequence);

		value_type v;
		schema.add_element("type", v);

		element type("xsd:complexType");
		type.emplace_back(std::move(sequence));
		types.emplace(type_name(), std::move(type));
	}
};

template<typename T>
struct type_serializer<std::optional<T>>
{
	using value_type = T;
	using container_type = std::optional<value_type>;
	using type_serializer_type = type_serializer<value_type>;

	static constexpr const char* type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element& n, const char* name, const container_type& value)
	{
		assert(name);

		if (value)
			type_serializer_type::serialize_child(n, name, *value);
	}

	static void deserialize_child(const element& n, const char* name, container_type& value)
	{
		assert(name);

		for (auto& e: n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);
			value.emplace(std::move(v));
		}
	}

	static element schema(const std::string& name, const std::string& prefix)
	{
		return {
			"xsd:element",
			{
				{ "name", name },
				{ "type", prefix + ':' + type_name() },
				{ "minOccurs", "0" },
				{ "maxOccurs", "1" }
			}
		};
	}

	static void register_type(type_map& types)
	{
		type_serializer_type::register_type(types);
	}
};

// nice trick to enforce order in template selection
template<unsigned N> struct priority_tag : priority_tag < N - 1 > {};
template<> struct priority_tag<0> {};

template<typename T>
struct type_serializer<T, std::enable_if_t<is_serializable_array_type_v<T,serializer>>>
{
	using container_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using value_type = value_type_t<container_type>;
	using type_serializer_type = type_serializer<value_type>;

	static constexpr const char* type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element& n, const char* name, const container_type& value)
	{
		assert(name);

		for (const value_type& v : value)
			type_serializer_type::serialize_child(n, name, v);
	}

	template<size_t N>
	static auto deserialize_array(const element& n, const char* name,
		std::array<value_type, N>& value, priority_tag<2>)
	{
		assert(name);

		size_t ix = 0;
		for (auto& e: n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			value[ix] = std::move(v);
			++ix;

			if (ix >= N)
				break;
		}
	}

	template<typename A>
	static auto deserialize_array(const element& n, const char* name, A& arr, priority_tag<1>)
		-> decltype(
			arr.reserve(std::declval<typename container_type::size_type>()),
			void()
		)
	{
		arr.reserve(n.size());

		assert(name);

		for (auto& e: n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			arr.emplace_back(std::move(v));
		}
	}

	static void deserialize_array(const element& n, const char* name, container_type& arr, priority_tag<0>)
	{
		assert(name);

		for (auto& e: n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			arr.emplace_back(std::move(v));
		}
	}

	static void deserialize_child(const element& n, const char* name, container_type& value)
	{
		type_serializer::deserialize_array(n, name, value, priority_tag<2>{});
	}
};

// And finally, the implementation of serializer, deserializer and schema_creator.

template<typename T>
serializer& serializer::serialize_element(const T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::serialize_child(m_node, "", value);

	return *this;
}

template<typename T>
serializer& serializer::serialize_element(const char* name, const T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::serialize_child(m_node, name, value);

	return *this;
}

template<typename T>
serializer& serializer::serialize_attribute(const char* name, const T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	m_node.attributes().emplace(name, type_serializer::serialize_value(value));

	return *this;
}

template<typename T>
deserializer& deserializer::deserialize_element(T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;
	
	type_serializer::deserialize_child(m_node, "", value);

	return *this;
}

template<typename T>
deserializer& deserializer::deserialize_element(const char* name, T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;
	
	type_serializer::deserialize_child(m_node, name, value);

	return *this;
}

template<typename T>
deserializer& deserializer::deserialize_attribute(const char* name, T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	std::string attr = m_node.get_attribute(name);
	if (not attr.empty())
		value = type_serializer::deserialize_value(attr);

	return *this;
}

template<typename T>
schema_creator& schema_creator::add_element(const char* name, const T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type, serializer>;
	
	m_node.emplace(type_serializer::schema(name, m_prefix));

	std::string type_name = type_serializer::type_name();

	// might be known already
	if (m_types.find(type_name) == m_types.end())
		type_serializer::register_type(m_types);

	return *this;
}

template<typename T>
schema_creator& schema_creator::add_attribute(const char* name, const T& value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;
	
	std::string type_name = type_serializer::type_name();

	assert(m_node.parent() != nullptr);
	m_node.parent()->emplace_back(
		element("xsd:attribute",
		{
			{ "name", name },
			{ "type", type_name }
		}));

	if (m_types.find(type_name) == m_types.end())
		type_serializer::register_type(m_types);

	return *this;
}

}
