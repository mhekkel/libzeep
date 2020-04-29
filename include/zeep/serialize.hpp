//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <regex>
#include <experimental/type_traits>

#include <zeep/exception.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

/** Serialization support in libzeep is split into generic to_string/from_string
 * routines for various basic types and specific complex type related routines
 * that are defined in the xml and el packages.
 */

namespace zeep {

template<typename T>
class name_value_pair
{
  public:
	name_value_pair(const char* name, T& value)
		: m_name(name), m_value(value) {}
	name_value_pair(const name_value_pair& other)
		: m_name(other.m_name), m_value(const_cast<T&>(other.m_value)) {}

	const char* name() const		{ return m_name; }
	T&			value() 			{ return m_value; }
	const T&	value() const		{ return m_value; }

  private:
	const char* m_name;
	T&			m_value;
};

template<typename T>
name_value_pair<T> make_nvp(const char* name, T& v)
{
	return name_value_pair<T>(name, v);
}

template<typename T>
name_value_pair<T> make_attribute_nvp(const char* name, T& v)
{
	return name_value_pair<T>(name, v);
}

template<typename T>
name_value_pair<T> make_element_nvp(const char* name, T& v)
{
	return name_value_pair<T>(name, v);
}

// --------------------------------------------------------------------
/// \brief A template boilerplate for conversion of basic types to or 
/// from strings.
///
/// Each specialization should provide a static to_string and a from_string
/// method as well as a type_name method. This type_name is used in e.g.
/// constructing WSDL files.

template<typename T, typename = void>
struct value_serializer;

template<>
struct value_serializer<bool>
{
	static constexpr const char* type_name() 				{ return "xsd:boolean"; }
	static std::string to_string(bool value)				{ return value ? "true" : "false"; }
	static bool from_string(const std::string& value)		{ return value == "true" or value == "1" or value == "yes"; }
};

template<>
struct value_serializer<std::string>
{
	static constexpr const char* type_name() 				{ return "xsd:string"; }
	static std::string to_string(const std::string& value)	{ return value; }
	static std::string from_string(const std::string& value){ return value; }
};

template<>
struct value_serializer<int8_t>
{
	static constexpr const char* type_name()				{ return "xsd:byte"; }
	static std::string to_string(int8_t value)				{ return std::to_string(value);	}
	static int8_t from_string(const std::string& value)		{ return static_cast<int8_t>(std::stoi(value)); }
};

template<>
struct value_serializer<uint8_t>
{
	static constexpr const char* type_name()				{ return "xsd:unsignedByte"; }
	static std::string to_string(uint8_t value)				{ return std::to_string(value);	}
	static uint8_t from_string(const std::string& value)	{ return static_cast<uint8_t>(std::stoul(value)); }
};

template<>
struct value_serializer<int16_t>
{
	static constexpr const char* type_name()				{ return "xsd:short"; }
	static std::string to_string(int16_t value)				{ return std::to_string(value);	}
	static int16_t from_string(const std::string& value)	{ return static_cast<int16_t>(std::stoi(value)); }
};

template<>
struct value_serializer<uint16_t>
{
	static constexpr const char* type_name()				{ return "xsd:unsignedShort"; }
	static std::string to_string(uint16_t value)			{ return std::to_string(value);	}
	static uint16_t from_string(const std::string& value)	{ return static_cast<uint16_t>(std::stoul(value)); }
};

template<>
struct value_serializer<int32_t>
{
	static constexpr const char* type_name()				{ return "xsd:int"; }
	static std::string to_string(int32_t value)				{ return std::to_string(value);	}
	static int32_t from_string(const std::string& value)	{ return std::stoi(value); }
};

template<>
struct value_serializer<uint32_t>
{
	static constexpr const char* type_name()				{ return "xsd:unsignedInt"; }
	static std::string to_string(uint32_t value)			{ return std::to_string(value);	}
	static uint32_t from_string(const std::string& value)	{ return static_cast<uint32_t>(std::stoul(value)); }
};

template<>
struct value_serializer<int64_t>
{
	static constexpr const char* type_name()				{ return "xsd:long"; }
	static std::string to_string(int64_t value)				{ return std::to_string(value);	}
	static int64_t from_string(const std::string& value)	{ return static_cast<int64_t>(std::stoll(value)); }
};

template<>
struct value_serializer<uint64_t>
{
	static constexpr const char* type_name()				{ return "xsd:unsignedLong"; }
	static std::string to_string(uint64_t value)			{ return std::to_string(value);	}
	static uint64_t from_string(const std::string& value)	{ return static_cast<uint64_t>(std::stoull(value)); }
};

template<>
struct value_serializer<float>
{
	static constexpr const char* type_name()				{ return "xsd:float"; }
	// static std::string to_string(float value)				{ return std::to_string(value);	}
	static std::string to_string(float value)
	{
		std::ostringstream s;
		s << value;
		return s.str();
	}
	static float from_string(const std::string& value)		{ return std::stof(value); }
};

template<>
struct value_serializer<double>
{
	static constexpr const char* type_name()				{ return "xsd:double"; }
	// static std::string to_string(double value)				{ return std::to_string(value);	}
	static std::string to_string(double value)
	{
		std::ostringstream s;
		s << value;
		return s.str();
	}
	static double from_string(const std::string& value)		{ return std::stod(value); }
};

template<typename T>
struct value_serializer<T, std::enable_if_t<std::is_enum_v<T>>>
{
	std::string m_type_name;

	using value_map_type = std::map<T,std::string>;
	value_map_type m_value_map;

	static value_serializer& instance(const char* name = nullptr) {
		static value_serializer s_instance;
		if (name and s_instance.m_type_name.empty())
			s_instance.m_type_name = name;
		return s_instance;
	}

	value_serializer& operator()(T v, const std::string& name)
	{
		m_value_map[v] = name;
		return *this;
	}

	value_serializer& operator()(const std::string& name, T v)
	{
		m_value_map[v] = name;
		return *this;
	}

	static const char* type_name()
	{
		return instance().m_type_name;
	}

	static std::string to_string(T value)
	{
		return instance().m_value_map[value];
	}

	static T from_string(const std::string& value)
	{
		T result = {};
		for (auto& t: instance().m_value_map)
			if (t.second == value)
			{
				result = t.first;
				break;
			}
		return result;
	}
};

// --------------------------------------------------------------------
// date/time support

/// \brief to_stringr/from_stringr for boost::posix_time::ptime
/// boost::posix_time::ptime values are always assumed to be UTC

template<>
struct value_serializer<boost::posix_time::ptime>
{
	static constexpr const char* type_name() { return "xsd:dateTime"; }
	
	/// to_string the boost::posix_time::ptime as YYYY-MM-DDThh:mm:ssZ (zero UTC offset)
	static std::string to_string(const boost::posix_time::ptime& v)
	{
		return boost::posix_time::to_iso_extended_string(v).append("Z");
	}

	/// from_string according to ISO8601 rules.
	/// If Zulu time is specified, then the parsed xsd:dateTime is returned.
	/// If an UTC offset is present, then the offset is subtracted from the xsd:dateTime, this yields UTC.
	/// If no UTC offset is present, then the xsd:dateTime is assumed to be local time and converted to UTC.
	static boost::posix_time::ptime from_string(const std::string& s)
	{
		// We accept 3 general formats:
		//  1: date fields separated with dashes, time fields separated with colons, eg. 2013-02-17T15:25:20,502104+01:00
		//  2: date fields not separated, time fields separated with colons, eg. 20130217T15:25:20,502104+01:00
		//  3: date fields not separated, time fields not separated, eg. 20130217T152520,502104+01:00

		// Apart from the separators, the 3 regexes are basically the same, i.e. they have the same fields
		// Note: std::regex is threadsafe, so we can declare these statically

		// Format 1:
		// ^(-?\d{4})-(\d{2})-(\d{2})T(\d{2})(:(\d{2})(:(\d{2})([.,](\d+))?)?)?((Z)|([-+])(\d{2})(:(\d{2}))?)?$
		//  ^         ^       ^       ^      ^ ^      ^ ^      ^    ^          ^^   ^     ^      ^ ^
		//  |         |       |       |      | |      | |      |    |          ||   |     |      | |
		//  |         |       |       |      | |      | |      |    |          ||   |     |      | [16] UTC minutes offset
		//  |         |       |       |      | |      | |      |    |          ||   |     |      [15] have UTC minutes offset?
		//  |         |       |       |      | |      | |      |    |          ||   |     [14] UTC hour offset
		//  |         |       |       |      | |      | |      |    |          ||   [13] UTC offset sign
		//  |         |       |       |      | |      | |      |    |          |[12] Zulu time
		//  |         |       |       |      | |      | |      |    |          [11] have time zone?
		//  |         |       |       |      | |      | |      |    [10] fractional seconds
		//  |         |       |       |      | |      | |      [9] have fractional seconds
		//  |         |       |       |      | |      | [8] seconds
		//  |         |       |       |      | |      [7] have seconds?
		//  |         |       |       |      | [6] minutes
		//  |         |       |       |      [5] have minutes?
		//  |         |       |       [4] hours
		//  |         |       [3] day
		//  |         [2] month
		//  [1] year
		static std::regex re1("^(-?\\d{4})-(\\d{2})-(\\d{2})T(\\d{2})(:(\\d{2})(:(\\d{2})([.,](\\d+))?)?)?((Z)|([-+])(\\d{2})(:(\\d{2}))?)?$");

		// Format 2:
		// ^(-?\d{4})(\d{2})(\d{2})T(\d{2})(:(\d{2})(:(\d{2})([.,]\d+)?)?)?((Z)|([-+])(\d{2})(:(\d{2}))?)?$
		static std::regex re2("^(-?\\d{4})(\\d{2})(\\d{2})T(\\d{2})(:(\\d{2})(:(\\d{2})([.,]\\d+)?)?)?((Z)|([-+])(\\d{2})(:(\\d{2}))?)?$");

		// Format 3:
		// ^(-?\d{4})(\d{2})(\d{2})T(\d{2})((\d{2})((\d{2})([.,]\d+)?)?)?((Z)|([-+])(\d{2})(:(\d{2}))?)?$
		static std::regex re3("^(-?\\d{4})(\\d{2})(\\d{2})T(\\d{2})((\\d{2})((\\d{2})([.,]\\d+)?)?)?((Z)|([-+])(\\d{2})(:(\\d{2}))?)?$");

		static const int f_year              =  1;
		static const int f_month             =  2;
		static const int f_day               =  3;
		static const int f_hours             =  4;
		static const int f_have_minutes      =  5;
		static const int f_minutes           =  6;
		static const int f_have_seconds      =  7;
		static const int f_seconds           =  8;
		static const int f_have_frac         =  9;
		static const int f_frac              = 10;
		static const int f_have_tz           = 11;
		static const int f_zulu              = 12;
		static const int f_offs_sign         = 13;
		static const int f_offs_hours        = 14;
		static const int f_have_offs_minutes = 15;
		static const int f_offs_minutes      = 16;

		std::smatch m;
		if (not std::regex_match(s, m, re1)) {
			if (not std::regex_match(s, m, re2)) {
				if (not std::regex_match(s, m, re3)) {
					throw exception("Bad dateTime format");
				}
			}
		}

		boost::gregorian::date d(
		  std::stoi(m[f_year])
		, std::stoi(m[f_month])
		, std::stoi(m[f_day])
		);

		int hours = std::stoi(m[f_hours]);
		int minutes = 0, seconds = 0;
		if (m.length(f_have_minutes)) {
			minutes = std::stoi(m[f_minutes]);
			if (m.length(f_have_seconds)) {
				seconds = std::stoi(m[f_seconds]);
			}
		}
		boost::posix_time::time_duration t(hours, minutes, seconds);

		if (m.length(f_have_frac)) {
			double frac = std::stod(std::string(".").append(std::string(m[f_frac])));
			t += boost::posix_time::microseconds(static_cast<int64_t>((frac + .5) * 1e6));
		}

		boost::posix_time::ptime result = boost::posix_time::ptime(d, t);

		if (m.length(f_have_tz)) {
			if (not m.length(f_zulu)) {
				std::string sign = m[f_offs_sign];
				int hours = std::stoi(m[f_offs_hours]);
				int minutes = 0;
				if (m.length(f_have_offs_minutes)) {
					minutes = std::stoi(m[f_offs_minutes]);
				}
				boost::posix_time::time_duration offs(hours, minutes, 0);
				if (sign == "+") {
					result -= offs;
				} else {
					result += offs;
				}
			}
		} else {
			// Boost has no clear way of instantiating the *current* timezone, so
			// it's not possible to convert from local to UTC, using boost::local_time classes
			// For now, settle on using mktime...
			std::tm tm = boost::posix_time::to_tm(result);
			tm.tm_isdst = -1;
			std::time_t t = mktime(&tm);
			result = boost::posix_time::from_time_t(t);
		}

		return result;
	}
};

/// \brief to_stringr/from_stringr for boost::gregorian::date
/// boost::gregorian::date values are assumed to be floating, i.e. we don't accept timezone info in dates

template<>
struct value_serializer<boost::gregorian::date>
{
	static constexpr const char* type_name() { return "xsd:date"; }

	/// to_string the boost::gregorian::date as YYYY-MM-DD
	static std::string to_string(const boost::gregorian::date& v)
	{
		return boost::gregorian::to_iso_extended_string(v);
	}

	/// from_string boost::gregorian::date according to ISO8601 rules, but without timezone.
	static boost::gregorian::date from_string(const std::string& s)
	{
		// We accept 2 general formats:
		//  1: date fields separated with dashes, eg. 2013-02-17
		//  2: date fields not separated, eg. 20130217

		// Apart from the separators, the 2 regexes are basically the same, i.e. they have the same fields
		// Note: std::regex is threadsafe, so we can declare these statically

		// Format 1:
		// ^(-?\d{4})-(\d{2})-(\d{2})$
		//  ^         ^       ^
		//  |         |       |
		//  |         |       |
		//  |         |       [3] day
		//  |         [2] month
		//  [1] year
		static std::regex re1("^(-?\\d{4})-(\\d{2})-(\\d{2})$");

		// Format 2:
		// ^(-?\d{4})(\d{2})(\d{2})$
		static std::regex re2("^(-?\\d{4})(\\d{2})(\\d{2})$");

		static const int f_year              =  1;
		static const int f_month             =  2;
		static const int f_day               =  3;

		std::smatch m;
		if (not std::regex_match(s, m, re1)) {
			if (not std::regex_match(s, m, re2)) {
				throw exception("Bad date format");
			}
		}

		return boost::gregorian::date(
				  std::stoi(m[f_year])
				, std::stoi(m[f_month])
				, std::stoi(m[f_day])
				);
	}
};

/// \brief to_stringr/from_stringr for boost::posix_time::time_duration
/// boost::posix_time::time_duration values are assumed to be floating, i.e. we don't accept timezone info in times

template<>
struct value_serializer<boost::posix_time::time_duration>
{
	static constexpr const char* type_name() { return "xsd:time"; }

	/// to_string the boost::posix_time::time_duration as hh:mm:ss,ffffff
	static std::string to_string(const boost::posix_time::time_duration& v)
	{
		return boost::posix_time::to_simple_string(v);
	}

	/// from_string boost::posix_time::time_duration according to ISO8601 rules, but without timezone.
	static boost::posix_time::time_duration from_string(const std::string& s)
	{
		// We accept 2 general formats:
		//  1: time fields separated with colons, eg. 15:25:20,502104
		//  2: time fields not separated, eg. 152520,502104

		// Apart from the separators, the 2 regexes are basically the same, i.e. they have the same fields
		// Note: std::regex is threadsafe, so we can declare these statically

		// Format 1:
		// ^(\d{2})(:(\d{2})(:(\d{2})([.,](\d+))?)?)?$
		//  ^      ^ ^      ^ ^      ^    ^
		//  |      | |      | |      |    |
		//  |      | |      | |      |    [7] fractional seconds
		//  |      | |      | |      [6] have fractional seconds
		//  |      | |      | [5] seconds
		//  |      | |      [4] have seconds?
		//  |      | [3] minutes
		//  |      [2] have minutes?
		//  [1] hours
		static std::regex re1("^(\\d{2})(:(\\d{2})(:(\\d{2})([.,](\\d+))?)?)?$");

		// Format 2:
		// ^(\d{2})((\d{2})((\d{2})([.,](\d+))?)?)?$
		static std::regex re2("^(\\d{2})((\\d{2})((\\d{2})([.,](\\d+))?)?)?$");

		static const int f_hours             =  1;
		static const int f_have_minutes      =  2;
		static const int f_minutes           =  3;
		static const int f_have_seconds      =  4;
		static const int f_seconds           =  5;
		static const int f_have_frac         =  6;
		static const int f_frac              =  7;

		std::smatch m;
		if (not std::regex_match(s, m, re1)) {
			if (not std::regex_match(s, m, re2)) {
				throw exception("Bad time format");
			}
		}

		int hours = std::stoi(m[f_hours]);
		int minutes = 0, seconds = 0;
		if (m.length(f_have_minutes)) {
			minutes = std::stoi(m[f_minutes]);
			if (m.length(f_have_seconds)) {
				seconds = std::stoi(m[f_seconds]);
			}
		}

		boost::posix_time::time_duration result = boost::posix_time::time_duration(hours, minutes, seconds);

		if (m.length(f_have_frac)) {
			double frac = std::stod(std::string(".").append(std::string(m[f_frac])));
			result += boost::posix_time::microseconds(static_cast<int64_t>((frac + .5) * 1e6));
		}
		
		return result;
	}
};

} // namespace zeep
