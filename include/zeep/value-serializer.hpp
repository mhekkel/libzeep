//        Copyright Maarten L. Hekkelman, 2014-2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/*! \file zeep/value-serializer.hpp
    \brief File containing the common serialization code in libzeep

    Serialization in libzeep is used by both the XML and the JSON sub libraries.
    Code that is common is found here.
*/

#include <zeep/config.hpp>

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <map>
#include <regex>
#include <sstream>

#include <zeep/exception.hpp>

#if __has_include(<date/date.h>)
#include <date/date.h>
#endif

namespace zeep
{

// --------------------------------------------------------------------
/// \brief A template boilerplate for conversion of basic types to or
/// from strings.
///
/// Each specialization should provide a static to_string and a from_string
/// method as well as a type_name method. This type_name is used in e.g.
/// constructing WSDL files.

template <typename T, typename = void>
struct value_serializer;

/// @ref value_serializer implementation for booleans
template <>
struct value_serializer<bool>
{
	static constexpr const char *type_name() { return "xsd:boolean"; }
	static std::string to_string(bool value) { return value ? "true" : "false"; }
	static bool from_string(const std::string &value) { return value == "true" or value == "1" or value == "yes"; }
};

/// @ref value_serializer implementation for std::strings
template <>
struct value_serializer<std::string>
{
	static constexpr const char *type_name() { return "xsd:string"; }
	static std::string to_string(const std::string &value) { return value; }
	static std::string from_string(const std::string &value) { return value; }
};

template<typename T>
struct char_conv_serializer
{
	using value_type = T;

	static constexpr const char *derived_type_name()
	{
		using value_serializer_type = value_serializer<value_type>;
		return value_serializer_type::type_name();
	}

	static std::string to_string(value_type value) { return std::to_string(value); }
	static value_type from_string(const std::string &value)
	{
		value_type result{};

		auto r = std::from_chars(value.data(), value.data() + value.length(), result);

		if (r.ec != std::errc{} or r.ptr != value.data() + value.length())
			throw std::system_error(std::make_error_code(r.ec), "Error converting value '" + value + "' to type " + derived_type_name());

		return result;
	}
};

/// @ref value_serializer implementation for small int8_t
template <>
struct value_serializer<int8_t> : char_conv_serializer<int8_t>
{
	static constexpr const char *type_name() { return "xsd:byte"; }
};

/// @ref value_serializer implementation for uint8_t
template <>
struct value_serializer<uint8_t> : char_conv_serializer<uint8_t>
{
	static constexpr const char *type_name() { return "xsd:unsignedByte"; }
};

/// @ref value_serializer implementation for int16_t
template <>
struct value_serializer<int16_t> : char_conv_serializer<int16_t>
{
	static constexpr const char *type_name() { return "xsd:short"; }
};

/// @ref value_serializer implementation for uint16_t
template <>
struct value_serializer<uint16_t> : char_conv_serializer<uint16_t>
{
	static constexpr const char *type_name() { return "xsd:unsignedShort"; }
};

/// @ref value_serializer implementation for int32_t
template <>
struct value_serializer<int32_t> : char_conv_serializer<int32_t>
{
	static constexpr const char *type_name() { return "xsd:int"; }
};

/// @ref value_serializer implementation for uint32_t
template <>
struct value_serializer<uint32_t> : char_conv_serializer<uint32_t>
{
	static constexpr const char *type_name() { return "xsd:unsignedInt"; }
};

/// @ref value_serializer implementation for int64_t
template <>
struct value_serializer<int64_t> : char_conv_serializer<int64_t>
{
	static constexpr const char *type_name() { return "xsd:long"; }
};

/// @ref value_serializer implementation for uint64_t
template <>
struct value_serializer<uint64_t> : char_conv_serializer<uint64_t>
{
	static constexpr const char *type_name() { return "xsd:unsignedLong"; }
};

/// @ref value_serializer implementation for float
template <>
struct value_serializer<float>
{
	static constexpr const char *type_name() { return "xsd:float"; }
	// static std::string to_string(float value)				{ return std::to_string(value);	}
	static std::string to_string(float value)
	{
		std::ostringstream s;
		s << value;
		return s.str();
	}
	static float from_string(const std::string &value) { return std::stof(value); }
};

/// @ref value_serializer implementation for double
template <>
struct value_serializer<double>
{
	static constexpr const char *type_name() { return "xsd:double"; }
	// static std::string to_string(double value)				{ return std::to_string(value);	}
	static std::string to_string(double value)
	{
		std::ostringstream s;
		s << value;
		return s.str();
	}
	static double from_string(const std::string &value) { return std::stod(value); }
};

/// \brief value_serializer for enum values
///
/// This class is used to (de-)serialize enum values. To map enum
/// values to a string you should use the singleton instance
/// accessible through instance() and then call the operator()
/// members assinging each of the enum values with their respective
/// string.
///
/// A recent addition is the init() call to initialize the instance

template <typename T>
struct value_serializer<T, std::enable_if_t<std::is_enum_v<T>>>
{
	std::string m_type_name;

	using value_map_type = std::map<T, std::string>;
	using value_map_value_type = typename value_map_type::value_type;

	value_map_type m_value_map;

	/// \brief Initialize a new instance of value_serializer for this enum, with name and a set of name/value pairs
	static void init(const char *name, std::initializer_list<value_map_value_type> values)
	{
		instance(name).m_value_map = value_map_type(values);
	}

	/// \brief Initialize a new anonymous instance of value_serializer for this enum with a set of name/value pairs
	static void init(std::initializer_list<value_map_value_type> values)
	{
		instance().m_value_map = value_map_type(values);
	}

	static value_serializer &instance(const char *name = nullptr)
	{
		static value_serializer s_instance;
		if (name and s_instance.m_type_name.empty())
			s_instance.m_type_name = name;
		return s_instance;
	}

	value_serializer &operator()(T v, const std::string &name)
	{
		m_value_map[v] = name;
		return *this;
	}

	value_serializer &operator()(const std::string &name, T v)
	{
		m_value_map[v] = name;
		return *this;
	}

	static const char *type_name()
	{
		return instance().m_type_name;
	}

	static std::string to_string(T value)
	{
		return instance().m_value_map[value];
	}

	static T from_string(const std::string &value)
	{
		T result = {};
		for (auto &t : instance().m_value_map)
			if (t.second == value)
			{
				result = t.first;
				break;
			}
		return result;
	}

	static bool empty()
	{
		return instance().m_value_map.empty();
	}
};

// --------------------------------------------------------------------
// date/time support
// We're using Howard Hinands date functions here. If available...

#if __has_include(<date/date.h>)

/// \brief to_string/from_string for std::chrono::system_clock::time_point
/// time is always assumed to be UTC
/// For a specification, see https://www.iso20022.org/standardsrepository/type/ISODateTime

template <>
struct value_serializer<std::chrono::system_clock::time_point>
{
	using time_type = std::chrono::system_clock::time_point;

	static constexpr const char *type_name() { return "xsd:dateTime"; }

	/// to_string the time as YYYY-MM-DDThh:mm:ssZ (zero UTC offset)
	static std::string to_string(const time_type &v)
	{
		std::ostringstream ss;
		ss << date::format("%FT%TZ", v);
		return ss.str();
	}

	/// from_string according to ISO8601 rules.
	/// If Zulu time is specified, then the parsed xsd:dateTime is returned.
	/// If an UTC offset is present, then the offset is subtracted from the xsd:dateTime, this yields UTC.
	/// If no UTC offset is present, then the xsd:dateTime is assumed to be local time and converted to UTC.
	static time_type from_string(const std::string &s)
	{
		time_type result;

		std::regex kRX(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}(?::\d{2}(?:\.\d+)?)?(Z|[-+]\d{2}:\d{2})?)");
		std::smatch m;

		if (not std::regex_match(s, m, kRX))
			throw std::runtime_error("Invalid date format");

		std::istringstream is(s);

		if (m[1].matched)
		{
			if (m[1] == "Z")
				date::from_stream(is, "%FT%TZ", result);
			else
				date::from_stream(is, "%FT%T%0z", result);
		}
		else
			date::from_stream(is, "%FT%T", result);

		if (is.bad() or is.fail())
			throw std::runtime_error("invalid formatted date");

		return result;
	}
};

/// \brief to_string/from_string for date::sys_days
/// For a specification, see https://www.iso20022.org/standardsrepository/type/ISODateTime

template <>
struct value_serializer<date::sys_days>
{
	static constexpr const char *type_name() { return "xsd:date"; }

	/// to_string the date as YYYY-MM-DD
	static std::string to_string(const date::sys_days &v)
	{
		std::ostringstream ss;
		date::to_stream(ss, "%F", v);
		return ss.str();
	}

	/// from_string according to ISO8601 rules.
	static date::sys_days from_string(const std::string &s)
	{
		date::sys_days result;

		std::istringstream is(s);
		date::from_stream(is, "%F", result);

		if (is.bad() or is.fail())
			throw std::runtime_error("invalid formatted date");
		
		return result;
	}
};

#endif

} // namespace zeep
