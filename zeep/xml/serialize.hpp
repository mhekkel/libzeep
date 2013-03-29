//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_SERIALIZE_H
#define SOAP_XML_SERIALIZE_H

#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <cassert>
#include <ctime>

#include <zeep/xml/node.hpp>
#include <zeep/exception.hpp>

#include <boost/config.hpp>
#include <boost/foreach.hpp>
#include <boost/mpl/if.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/integral_promotion.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>

//	Lots of template wizardry here...
//
//	The goal is to make a completely transparent XML serializer/deserializer
//	in order to translate SOAP messages into/out of native C++ types.
//
//	The interface for the code below is compatible with the 'serialize' member
//	function required to use boost::serialization. 

/// \def SOAP_XML_ADD_ENUM(e,v)
/// \brief A macro to add the name of an enum value to the serializer
///
/// To be able to correctly use enum values in a WSDL file or when serializing,
/// you have to specify the enum values.
///
/// E.g., if you have a struct name Algorithm with values 'vector', 'dice' and 'jaccard'
/// you would write:
///
///>	enum Algorithm { vector, dice, jaccard };
///>	SOAP_XML_ADD_ENUM(Algorithm, vector);
///>	SOAP_XML_ADD_ENUM(Algorithm, dice);
///>	SOAP_XML_ADD_ENUM(Algorithm, jaccard);
///
/// An alternative (better?) way to do this is:
///
///>	zeep::xml::enum_map<Algorithm>::instance("Algorithm").add_enum()
///>		("vector", vector)
///>		("dice", dice)
///>		("jaccard", jaccard);

/// \def SOAP_XML_SET_STRUCT_NAME(s)
/// \brief A macro to assign a name to a struct used in serialization.
///
/// By default, libzeep uses the typeid(s).name() as the name for an element.
/// That's often not what is intented. Calling this macro will make sure
/// the type name you used in your code will be used instead.
///
/// E.g., struct FindResult { ... } might end up with a fancy name in the
/// WSDL. To use FindResult instead, call SOAP_XML_SET_STRUCT_NAME(FindResult);
///
/// An alternative it to call, which allows different WSDL and struct names:
/// zeep::xml::struct_serializer<FindResult>::set_struct_name("FindResult");

namespace zeep { namespace xml {

#ifndef LIBZEEP_DOXYGEN_INVOKED
const std::string kPrefix = "ns";
#endif

///	All serializers and deserializers work on an object that contains
///	a pointer to the node just above the actual node holding their data. If any.
///
///	This means for the serializer that it has to create a new node, set the content
///	and add it to the passed in node.
///	For deserializers this means looking up the first child matching the name
///	in the passed-in node to fetch the data from.
///
/// Older versions of libzeep used to use boost::serialization::nvp as type to
/// specify name/value pairs. This will continue to work, but to use attributes
/// we come up with a special version of name/value pairs specific for libzeep.

struct serializer;
struct deserializer;

template<typename T>
struct element_nvp : public boost::serialization::nvp<T>
{
	explicit element_nvp(const char* name, T& v) : nvp(name, v) {}
	element_nvp(const element_nvp& rhs) : nvp(rhs) {}
};

template<typename T>
struct attribute_nvp : public boost::serialization::nvp<T>
{
	explicit attribute_nvp(const char* name, T& v) : nvp(name, v) {}
	attribute_nvp(const attribute_nvp& rhs) : nvp(rhs) {}
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

#define ZEEP_ELEMENT_NAME_VALUE(name) \
	zeep::xml::make_element_nvp(BOOST_PP_STRINGIZE(name), name)

#define ZEEP_ATTRIBUTE_NAME_VALUE(name) \
	zeep::xml::make_attribute_nvp(BOOST_PP_STRINGIZE(name), name)

struct serializer
{
					serializer(container* node) : m_node(node) {}

	template<typename T>
	serializer&		operator&(const boost::serialization::nvp<T>& rhs)
					{
						return serialize_element(rhs.name(), rhs.value());
					}
	
	template<typename T>
	serializer&		operator&(const element_nvp<T>& rhs)
					{
						return serialize_element(rhs.name(), rhs.value());
					}
	
	template<typename T>
	serializer&		operator&(const attribute_nvp<T>& rhs)
					{
						return serialize_attribute(rhs.name(), rhs.value());
					}
	
	template<typename T>
	serializer&		serialize_element(const char* name, const T& data);

	template<typename T>
	serializer&		serialize_attribute(const char* name, const T& data);

	container*		m_node;
};

struct deserializer
{
					deserializer(const container* node) : m_node(node) {}

	template<typename T>
	deserializer&	operator&(const boost::serialization::nvp<T>& rhs)
					{
						return deserialize_element(rhs.name(), rhs.value());
					}

	template<typename T>
	deserializer&	operator&(const element_nvp<T>& rhs)
					{
						return deserialize_element(rhs.name(), rhs.value());
					}
	
	template<typename T>
	deserializer&	operator&(const attribute_nvp<T>& rhs)
					{
						return deserialize_attribute(rhs.name(), rhs.value());
					}
	
	template<typename T>
	deserializer&	deserialize_element(const char* name, T& data);

	template<typename T>
	deserializer&	deserialize_attribute(const char* name, T& data);

	const container* m_node;
};

#ifndef LIBZEEP_DOXYGEN_INVOKED
typedef std::map<std::string,element*> type_map;
#endif

/// wsdl_creator is used by zeep::dispatcher to create WSDL files.

struct wsdl_creator
{
					wsdl_creator(type_map& types, element* node, bool make_node = true)
						: m_node(node), m_types(types), m_make_node(make_node) {}
	
	template<typename T>
	wsdl_creator&	operator&(const boost::serialization::nvp<T>& rhs)
					{
						return add_element(rhs.name(), rhs.value());
					}

	template<typename T>
	wsdl_creator&	operator&(const element_nvp<T>& rhs)
					{
						return add_element(rhs.name(), rhs.value());
					}
	
	template<typename T>
	wsdl_creator&	operator&(const attribute_nvp<T>& rhs)
					{
						return add_attribute(rhs.name(), rhs.value());
					}

	template<typename T>
	wsdl_creator&	add_element(const char* name, const T& value);

	template<typename T>
	wsdl_creator&	add_attribute(const char* name, const T& value);

	element*		m_node;
	type_map&		m_types;
	bool			m_make_node;
};

#ifndef LIBZEEP_DOXYGEN_INVOKED

// The actual (de)serializers
//
//	The serialize objects should all have three methods:
//		static void	serialize(container* n, const T& v);
//		static void	deserialize(const container* n, T& v);
//		static void	wsdl(type_map& types, const std::string& name, const T& v);
//
//	the serialize method can use the convenience routine make_node to create the correct node type
//	(this can be an attribute node, or a regular element node based on the name).

template<typename Derived, typename Value>
struct basic_type_serializer
{
	enum {
		min_occurs		= 1,
		max_occurs		= 1,
	};
	
	typedef Value		value_type;
	
	static void serialize(container* n, const value_type& value)
	{
		n->str(Derived::serialize_value(value));
	}
	
	static void deserialize(const container* n, value_type& value)
	{
		value = Derived::deserialize_value(n->str());
	}
	
	static element* wsdl(type_map& types, const std::string& name)
	{
		element* n(new element("xsd:element"));

		n->set_attribute("name", name);
		n->set_attribute("type", Derived::type_name());
		n->set_attribute("minOccurs", boost::lexical_cast<string>(Derived::min_occurs));
		n->set_attribute("maxOccurs", boost::lexical_cast<string>(Derived::max_occurs));
		
		return n;
	}
};

// arithmetic types are ints, doubles, etc... simply use lexical_cast to convert these
template<typename T, int S = sizeof(T), bool = boost::is_unsigned<T>::value> struct arithmetic_wsdl_name {};

template<typename T> struct arithmetic_wsdl_name<T, 1, false> {
	static const char* type_name() { return "xsd:byte"; }
};
template<typename T> struct arithmetic_wsdl_name<T, 1, true> {
	static const char* type_name() { return "xsd:unsignedByte"; }
};
template<typename T> struct arithmetic_wsdl_name<T, 2, false> {
	static const char* type_name() { return "xsd:short"; }
};
template<typename T> struct arithmetic_wsdl_name<T, 2, true> {
	static const char* type_name() { return "xsd:unsignedShort"; }
};
template<typename T> struct arithmetic_wsdl_name<T, 4, false> {
	static const char* type_name() { return "xsd:int"; }
};
template<typename T> struct arithmetic_wsdl_name<T, 4, true> {
	static const char* type_name() { return "xsd:unsignedInt"; }
};
template<typename T> struct arithmetic_wsdl_name<T, 8, false> {
	static const char* type_name() { return "xsd:long"; }
};
template<typename T> struct arithmetic_wsdl_name<T, 8, true> {
	static const char* type_name() { return "xsd:unsignedLong"; }
};
template<> struct arithmetic_wsdl_name<float> {
	static const char* type_name() { return "xsd:float"; }
};
template<> struct arithmetic_wsdl_name<double> {
	static const char* type_name() { return "xsd:double"; }
};

template<typename T>
struct arithmetic_serializer : public basic_type_serializer<arithmetic_serializer<T>,
										typedef typename boost::remove_const<
											typename boost::remove_reference<T>::type
										>::type>
							 , public arithmetic_wsdl_name<T>
{
	// use promoted type to force writing out char as an integer
	typedef typename boost::integral_promotion<T>::type		promoted_type;

	static std::string serialize_value(const value_type& value)
	{
		return boost::lexical_cast<std::string>(static_cast<promoted_type>(value));
	}
	
	static value_type deserialize_value(const std::string& value)
	{
		return static_cast<value_type>(boost::lexical_cast<promoted_type>(value));
	}
};

struct string_serializer : public basic_type_serializer<string_serializer, std::string>
{
	typedef std::string value_type;
	
	static const char* type_name() { return "xsd:string"; }

	static std::string serialize_value(const std::string& value)
	{
		return value;
	}

	static std::string deserialize_value(const std::string& value)
	{
		return value;
	}
};

struct bool_serializer : public basic_type_serializer<bool_serializer, bool>
{
	typedef bool value_type;

	static const char* type_name() { return "xsd:boolean"; }
	
	static std::string serialize_value(bool value)
	{
		return value ? "true" : "false";
	}

	static bool deserialize_value(const std::string& value)
	{
		return (value == "true" or value == "1");
	}
};

/// \brief serializer/deserializer for boost::posix_time::ptime
/// boost::posix_time::ptime values are always assumed to be UTC
struct boost_posix_time_ptime_serializer : public basic_type_serializer<boost_posix_time_ptime_serializer, boost::posix_time::ptime>
{
	static const char* type_name() { return "xsd:dateTime"; }
	
	/// Serialize the boost::posix_time::ptime as YYYY-MM-DDThh:mm:ssZ (zero UTC offset)
	static std::string serialize_value(const boost::posix_time::ptime& v)
	{
		return boost::posix_time::to_iso_extended_string(v).append("Z");
	}

	/// Deserialize according to ISO8601 rules.
	/// If Zulu time is specified, then the parsed xsd:dateTime is returned.
	/// If an UTC offset is present, then the offset is subtracted from the xsd:dateTime, this yields UTC.
	/// If no UTC offset is present, then the xsd:dateTime is assumed to be local time and converted to UTC.
	static boost::posix_time::ptime deserialize_value(const std::string& s)
	{
		// We accept 3 general formats:
		//  1: date fields separated with dashes, time fields separated with colons, eg. 2013-02-17T15:25:20,502104+01:00
		//  2: date fields not separated, time fields separated with colons, eg. 20130217T15:25:20,502104+01:00
		//  3: date fields not separated, time fields not separated, eg. 20130217T152520,502104+01:00

		// Apart from the separators, the 3 regexes are basically the same, i.e. they have the same fields
		// Note: boost::regex is threadsafe, so we can declare these statically

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
		static boost::regex re1("^(-?\\d{4})-(\\d{2})-(\\d{2})T(\\d{2})(:(\\d{2})(:(\\d{2})([.,](\\d+))?)?)?((Z)|([-+])(\\d{2})(:(\\d{2}))?)?$");

		// Format 2:
		// ^(-?\d{4})(\d{2})(\d{2})T(\d{2})(:(\d{2})(:(\d{2})([.,]\d+)?)?)?((Z)|([-+])(\d{2})(:(\d{2}))?)?$
		static boost::regex re2("^(-?\\d{4})(\\d{2})(\\d{2})T(\\d{2})(:(\\d{2})(:(\\d{2})([.,]\\d+)?)?)?((Z)|([-+])(\\d{2})(:(\\d{2}))?)?$");

		// Format 3:
		// ^(-?\d{4})(\d{2})(\d{2})T(\d{2})((\d{2})((\d{2})([.,]\d+)?)?)?((Z)|([-+])(\d{2})(:(\d{2}))?)?$
		static boost::regex re3("^(-?\\d{4})(\\d{2})(\\d{2})T(\\d{2})((\\d{2})((\\d{2})([.,]\\d+)?)?)?((Z)|([-+])(\\d{2})(:(\\d{2}))?)?$");

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

		boost::smatch m;
		if (!boost::regex_match(s, m, re1)) {
			if (!boost::regex_match(s, m, re2)) {
				if (!boost::regex_match(s, m, re3)) {
					throw exception("Bad dateTime format");
				}
			}
		}

		boost::gregorian::date d(
		  boost::lexical_cast<int>(m[f_year])
		, boost::lexical_cast<int>(m[f_month])
		, boost::lexical_cast<int>(m[f_day])
		);

		int hours = boost::lexical_cast<int>(m[f_hours]);
		int minutes = 0, seconds = 0;
		if (m.length(f_have_minutes)) {
			minutes = boost::lexical_cast<int>(m[f_minutes]);
			if (m.length(f_have_seconds)) {
				seconds = boost::lexical_cast<int>(m[f_seconds]);
			}
		}
		boost::posix_time::time_duration t(hours, minutes, seconds);

		if (m.length(f_have_frac)) {
			double frac = boost::lexical_cast<double>(std::string(".").append(std::string(m[f_frac])));
			t += boost::posix_time::microseconds(static_cast<int64_t>((frac + .5) * 1e6));
		}

		boost::posix_time::ptime result = boost::posix_time::ptime(d, t);

		if (m.length(f_have_tz)) {
			if (!m.length(f_zulu)) {
				std::string sign = m[f_offs_sign];
				int hours = boost::lexical_cast<int>(m[f_offs_hours]);
				int minutes = 0;
				if (m.length(f_have_offs_minutes)) {
					minutes = boost::lexical_cast<int>(m[f_offs_minutes]);
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

	static element* wsdl(type_map& types, const std::string& name)
	{
		element* n(new element("xsd:element"));
		n->set_attribute("name", name);
		n->set_attribute("type", "xsd:dateTime");
		n->set_attribute("minOccurs", "1");
		n->set_attribute("maxOccurs", "1");
		return n;
	}
};

/// \brief serializer/deserializer for boost::gregorian::date
/// boost::gregorian::date values are assumed to be floating, i.e. we don't accept timezone info in dates
struct boost_gregorian_date_serializer : public basic_type_serializer<boost_gregorian_date_serializer, boost::gregorian::date>
{
	static const char* type_name() { return "xsd:date"; }

	/// Serialize the boost::gregorian::date as YYYY-MM-DD
	static std::string serialize_value(container* parent, const std::string& name, const boost::gregorian::date& v)
	{
		return boost::gregorian::to_iso_extended_string(v);
	}

	/// Deserialize boost::gregorian::date according to ISO8601 rules, but without timezone.
	static boost::gregorian::date deserialize_value(const std::string& s)
	{
		// We accept 2 general formats:
		//  1: date fields separated with dashes, eg. 2013-02-17
		//  2: date fields not separated, eg. 20130217

		// Apart from the separators, the 2 regexes are basically the same, i.e. they have the same fields
		// Note: boost::regex is threadsafe, so we can declare these statically

		// Format 1:
		// ^(-?\d{4})-(\d{2})-(\d{2})$
		//  ^         ^       ^
		//  |         |       |
		//  |         |       |
		//  |         |       [3] day
		//  |         [2] month
		//  [1] year
		static boost::regex re1("^(-?\\d{4})-(\\d{2})-(\\d{2})$");

		// Format 2:
		// ^(-?\d{4})(\d{2})(\d{2})$
		static boost::regex re2("^(-?\\d{4})(\\d{2})(\\d{2})$");

		static const int f_year              =  1;
		static const int f_month             =  2;
		static const int f_day               =  3;

		boost::smatch m;
		if (!boost::regex_match(s, m, re1)) {
			if (!boost::regex_match(s, m, re2)) {
				throw exception("Bad date format");
			}
		}

		return boost::gregorian::date(
				  boost::lexical_cast<int>(m[f_year])
				, boost::lexical_cast<int>(m[f_month])
				, boost::lexical_cast<int>(m[f_day])
				);
	}

	static element* wsdl(type_map& types, const std::string& name)
	{
		element* n(new element("xsd:element"));
		n->set_attribute("name", name);
		n->set_attribute("type", "xsd:date");
		n->set_attribute("minOccurs", "1");
		n->set_attribute("maxOccurs", "1");
		return n;
	}
};

/// \brief serializer/deserializer for boost::posix_time::time_duration
/// boost::posix_time::time_duration values are assumed to be floating, i.e. we don't accept timezone info in times
struct boost_posix_time_time_duration_serializer : public basic_type_serializer<boost_posix_time_time_duration_serializer, boost::posix_time::time_duration>
{
	static const char* type_name() { return "xsd:time"; }

	/// Serialize the boost::posix_time::time_duration as hh:mm:ss,ffffff
	static std::string serialize_value(const boost::posix_time::time_duration& v)
	{
		return boost::posix_time::to_simple_string(v);
	}

	/// Deserialize boost::posix_time::time_duration according to ISO8601 rules, but without timezone.
	static boost::posix_time::time_duration deserialize_value(const std::string& s)
	{
		// We accept 2 general formats:
		//  1: time fields separated with colons, eg. 15:25:20,502104
		//  2: time fields not separated, eg. 152520,502104

		// Apart from the separators, the 2 regexes are basically the same, i.e. they have the same fields
		// Note: boost::regex is threadsafe, so we can declare these statically

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
		static boost::regex re1("^(\\d{2})(:(\\d{2})(:(\\d{2})([.,](\\d+))?)?)?$");

		// Format 2:
		// ^(\d{2})((\d{2})((\d{2})([.,](\d+))?)?)?$
		static boost::regex re2("^(\\d{2})((\\d{2})((\\d{2})([.,](\\d+))?)?)?$");

		static const int f_hours             =  1;
		static const int f_have_minutes      =  2;
		static const int f_minutes           =  3;
		static const int f_have_seconds      =  4;
		static const int f_seconds           =  5;
		static const int f_have_frac         =  6;
		static const int f_frac              =  7;

		boost::smatch m;
		if (!boost::regex_match(s, m, re1)) {
			if (!boost::regex_match(s, m, re2)) {
				throw exception("Bad time format");
			}
		}

		int hours = boost::lexical_cast<int>(m[f_hours]);
		int minutes = 0, seconds = 0;
		if (m.length(f_have_minutes)) {
			minutes = boost::lexical_cast<int>(m[f_minutes]);
			if (m.length(f_have_seconds)) {
				seconds = boost::lexical_cast<int>(m[f_seconds]);
			}
		}

		boost::posix_time::time_duration result = boost::posix_time::time_duration(hours, minutes, seconds);

		if (m.length(f_have_frac)) {
			double frac = boost::lexical_cast<double>(std::string(".").append(std::string(m[f_frac])));
			result += boost::posix_time::microseconds(static_cast<int64_t>((frac + .5) * 1e6));
		}
		
		return result;
	}

	static element* wsdl(type_map& types, const std::string& name)
	{
		element* n(new element("xsd:element"));
		n->set_attribute("name", name);
		n->set_attribute("type", "xsd:time");
		n->set_attribute("minOccurs", "1");
		n->set_attribute("maxOccurs", "1");
		return n;
	}
};

template<typename S, typename T>
struct struct_serializer_archive
{
	static void serialize(S& stream, T& data)
	{
		data.serialize(stream, 0U);
	}
};

template<typename Derived, typename Struct>
struct struct_serializer_base
{
	typedef typename Struct		value_type;
	static std::string			s_struct_name;
	
	static void serialize(container* n, const value_type& value)
	{
		typedef typename struct_serializer_archive<serializer,value_type> archive;
		
		serializer sr(n);
		archive::serialize(sr, const_cast<value_type&>(value));
	}

	static void	deserialize(const container* n, value_type& v)
	{
		typedef typename struct_serializer_archive<deserializer,value_type>	archive;

		deserializer ds(n);
		archive::serialize(ds, v);
	}
	
	static element* wsdl(type_map& types, const std::string& name)
	{
		element* result(new element("xsd:element"));
		result->set_attribute("name", name);
		result->set_attribute("type", kPrefix + ':' + s_struct_name);
		result->set_attribute("minOccurs", "1");
		result->set_attribute("maxOccurs", "1");

		// we might be known already
		if (types.find(s_struct_name) == types.end())
		{
			element* n(new element("xsd:complexType"));
			n->set_attribute("name", s_struct_name);
			types[s_struct_name] = n;
			
			element* sequence(new element("xsd:sequence"));
			n->append(sequence);

			typedef typename struct_serializer_archive<wsdl_creator,value_type>	archive;
		
			wsdl_creator wsdl(types, sequence);

			value_type v;
			archive::serialize(wsdl, v);
		}
		
		return result;
	}
};

template<typename Struct>
struct struct_serializer_impl : public struct_serializer_base<struct_serializer_impl<Struct>, Struct>
{
	static void	set_struct_name(const std::string& name)
	{
		s_struct_name = name;
	}
};

template<typename Derived, typename Struct>
std::string struct_serializer_base<Derived,Struct>::s_struct_name = typeid(Struct).name();

#endif

#define SOAP_XML_SET_STRUCT_NAME(s)	zeep::xml::struct_serializer_impl<s>::s_struct_name = BOOST_PP_STRINGIZE(s);

#ifndef LIBZEEP_DOXYGEN_INVOKED

//template<typename T>
//struct serialize_boost_optional
//{
//	static void	serialize(container* parent, const std::string& name, boost::optional<T>& v);
//	static void	deserialize(const std::string& s, boost::optional<T>& v);
//
//	static element*	to_wsdl(type_map& types, element* parent, const std::string& name, boost::optional<T>& v);
//};

template<typename T>
struct enum_map
{
	typedef typename std::map<T,std::string>	name_mapping_type;
	
	name_mapping_type							m_name_mapping;
	std::string									m_name;
	
	static enum_map&
				instance(const char* name = NULL)
				{
					static enum_map s_instance;
					if (name and s_instance.m_name.empty())
						s_instance.m_name = name;
					return s_instance;
				}

	class add_enum_helper
	{
		friend struct enum_map;
					add_enum_helper(name_mapping_type& mapping)
						: m_mapping(mapping) {}
		
		name_mapping_type&
					m_mapping;

	  public:
		add_enum_helper&
					operator()(const std::string& name, T value)
					{
						m_mapping[value] = name;
						return *this;
					}
	};
	
	add_enum_helper	add_enum()
					{
						return add_enum_helper(m_name_mapping);
					}
};

#endif

#define SOAP_XML_ADD_ENUM(e,v)	zeep::xml::enum_map<e>::instance(BOOST_PP_STRINGIZE(e)).m_name_mapping[v] = BOOST_PP_STRINGIZE(v);

#ifndef LIBZEEP_DOXYGEN_INVOKED

template<typename T>
struct enum_serializer : public basic_type_serializer<enum_serializer<T>, T>
{
	typedef enum_map<T>					t_enum_map;
	typedef std::map<T,std::string>		t_map;
	
	static std::string serialize_value(const T& value)
	{
		return t_enum_map::instance().m_name_mapping[value];
	}

	static T deserialize_value(const std::string& value)
	{
		T result = T();
		
		t_map& m = t_enum_map::instance().m_name_mapping;
		for (typename t_map::iterator e = m.begin(); e != m.end(); ++e)
		{
			if (e->second == value)
			{
				result = e->first;
				break;
			}
		}
		
		return result;
	}

	static element* wsdl(type_map& types, const std::string& name)
	{
		std::string type_name = t_enum_map::instance().m_name;
		
		element* result(new element("xsd:element"));
		result->set_attribute("name", name);
		result->set_attribute("type", kPrefix + ':' + type_name);
		result->set_attribute("minOccurs", "1");
		result->set_attribute("maxOccurs", "1");
		
		// we might be known already
		if (types.find(type_name) == types.end())
		{
			element* n(new element("xsd:simpleType"));
			n->set_attribute("name", type_name);
			types[type_name] = n;
			
			element* restriction(new element("xsd:restriction"));
			restriction->set_attribute("base", "xsd:string");
			n->append(restriction);
			
			t_map& m = t_enum_map::instance().m_name_mapping;
			for (typename t_map::iterator e = m.begin(); e != m.end(); ++e)
			{
				element* en(new element("xsd:enumeration"));
				en->set_attribute("value", e->second);
				restriction->append(en);
			}
		}
		
		return result;
	}
};

// now create a type factory for these serializers

template<typename T, typename S = typename boost::mpl::if_c<
									boost::is_arithmetic<T>::value,
									arithmetic_serializer<T>,
									typename boost::mpl::if_c<
										boost::is_enum<T>::value,
										enum_serializer<T>,
										struct_serializer_impl<T>
									>::type
								>::type>
struct serializer_basic_type
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef S type_serializer_type;

	static const char* type_name() { return type_serializer_type::type_name(); }

	static void serialize_type(container* n, const char* name, const value_type& value)
	{
		element* e = new element(name);
		type_serializer_type::serialize(e, value);
		n->append(e);
	}

	static void deserialize_type(const container* n, const char* name, value_type& value)
	{
		element* e = n->find_first(name);
		if (e != nullptr)
			type_serializer_type::deserialize(e, value);
	}
	
	static element* wsdl(type_map& types, const std::string& name)
	{
		return type_serializer_type::wsdl(types, name);
	}
};

template<typename T>
struct serializer_type : public serializer_basic_type<T>
{
};

template<>
struct serializer_type<bool> : public serializer_basic_type<bool, bool_serializer>
{
};

template<>
struct serializer_type<std::string> : public serializer_basic_type<std::string, string_serializer>
{
};

template<>
struct serializer_type<boost::posix_time::ptime>
	: public serializer_basic_type<boost::posix_time::ptime, boost_posix_time_ptime_serializer>
{
};

template<>
struct serializer_type<boost::gregorian::date>
	: public serializer_basic_type<boost::gregorian::date, boost_gregorian_date_serializer>
{
};

template<>
struct serializer_type<boost::posix_time::time_duration>
	: public serializer_basic_type<boost::posix_time::time_duration, boost_posix_time_time_duration_serializer>
{
};

template<typename C>
struct serialize_container_type
{
	typedef C container_type;
	typedef typename container_type::value_type value_type;
	typedef serializer_type<value_type> base_serializer_type;
	
	static void serialize_type(container* n, const char* name, const container_type& value)
	{
		BOOST_FOREACH (const value_type& v, value)
		{
			element* e = new element(name);
			base_serializer_type::serialize_type(e, name, v);
			n->append(e);
		}
	}

	static void deserialize_type(const container* n, const char* name, container_type& value)
	{
		BOOST_FOREACH (const element* e, *n)
		{
			if (e->name() != name)
				continue;
			
			value_type v;
			base_serializer_type::deserialize_type(e, name, v);
			value.push_back(v);
		}
	}

	static element* wsdl(type_map& types, const std::string& name)
	{
		element* result = base_serializer_type::wsdl(types, name);
	
		result->remove_attribute("minOccurs");
		result->set_attribute("minOccurs", "0");
		
		result->remove_attribute("maxOccurs");
		result->set_attribute("maxOccurs", "unbounded");
	
		return result;
	}
};

template<typename T>
struct serializer_type<std::vector<T>> : public serialize_container_type<std::vector<T>>
{
};

template<typename T>
struct serializer_type<std::list<T>> : public serialize_container_type<std::list<T>>
{
};

template<typename T>
struct serializer_type<boost::optional<T>>
{
	typedef T							value_type;
	typedef serializer_type<value_type>	base_serializer_type;
	
	static void serialize_type(container* n, const char* name, const boost::optional<value_type>& value)
	{
		if (value.is_initialized())
			base_serializer_type::serialize_type(n, name, value.get());
	}

	static void deserialize_type(const container* n, const char* name, boost::optional<value_type>& value)
	{
		element* e = n->find_first(name);
		if (e != nullptr)
		{
			value_type v;
			base_serializer_type::deserialize_type(n, name, v);
			value = v;
		}
	}

	static element* wsdl(type_map& types, const std::string& name)
	{
		element* result = base_serializer_type::wsdl(types, name);
	
		result->remove_attribute("minOccurs");
		result->set_attribute("minOccurs", "0");
		
		result->remove_attribute("maxOccurs");
		result->set_attribute("maxOccurs", "1");
	
		return result;
	}
};

// and the wrappers for serializing

template<typename T>
serializer& serializer::serialize_element(const char* name, const T& value)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serializer_type<value_type>											type_serializer;

	type_serializer::serialize_type(m_node, name, value);

	return *this;
}

template<typename T>
serializer& serializer::serialize_attribute(const char* name, const T& value)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serializer_type<value_type>::type_serializer_type						type_serializer;

	element* e = dynamic_cast<element*>(m_node);
	if (e == nullptr)
		throw exception("can only create attributes for elements");
	e->set_attribute(name, type_serializer::serialize_value(value));

	return *this;
}

template<typename T>
deserializer& deserializer::deserialize_element(const char* name, T& value)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serializer_type<value_type>											type_serializer;
	
	type_serializer::deserialize_type(m_node, name, value);

	return *this;
}

template<typename T>
deserializer& deserializer::deserialize_attribute(const char* name, T& value)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serializer_type<value_type>::type_serializer_type						type_serializer;

	const element* e = dynamic_cast<const element*>(m_node);
	if (e == nullptr)
		throw exception("can only deserialize attributes for elements");
	else
	{
		std::string attr = e->get_attribute(name);
		if (not attr.empty())
			value = type_serializer::deserialize_value(attr);
	}

	return *this;
}

template<typename T>
wsdl_creator& wsdl_creator::add_element(const char* name, const T& value)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serializer_type<value_type>											type_serializer;
	
	m_node->append(type_serializer::wsdl(m_types, name));

	return *this;
}

template<typename T>
wsdl_creator& wsdl_creator::add_attribute(const char* name, const T& value)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serializer_type<value_type>											type_serializer;
	
	element* n(new element("xsd:attribute"));

	n->set_attribute("name", name);
	n->set_attribute("type", type_serializer::type_name());

	m_node->append(n);

	return *this;
}

#endif

}
}

#endif
