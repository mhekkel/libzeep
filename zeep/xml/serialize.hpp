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
/// zeep::xml::serialize_struct<FindResult>::set_struct_name("FindResult");

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

struct serializer
{
					serializer(element* node, bool make_node = true)
						: m_node(node)
						, m_make_node(make_node) {}

	template<typename T>
	serializer&		operator&(const boost::serialization::nvp<T>& rhs);

	element*		m_node;
	bool			m_make_node;
};

struct deserializer
{
					deserializer(container* node) : m_node(node) {}

	template<typename T>
	deserializer&	operator&(const boost::serialization::nvp<T>& rhs);

	container*		m_node;
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
	wsdl_creator&	operator&(const boost::serialization::nvp<T>& rhs);	

	element*		m_node;
	type_map&		m_types;
	bool			m_make_node;
};

#ifndef LIBZEEP_DOXYGEN_INVOKED

// The actual (de)serializers:

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
struct serialize_arithmetic
{
	typedef typename boost::remove_const<
			typename boost::remove_reference<T>::type >::type	value_type;
	typedef arithmetic_wsdl_name<value_type>					wsdl_name;
	
	static void	serialize(element* node, const std::string& name, T& v)
				{
					node->set_attribute(name, boost::lexical_cast<std::string>(v));
				}

	static void	serialize(element* parent, const std::string& name, T& v, bool)
				{
					element* n(new element(name));
					n->content(boost::lexical_cast<std::string>(v));
					parent->append(n);
				}

	static void	deserialize(const std::string& s, T& v)
				{
					v = boost::lexical_cast<T>(s);
				}

	static void	deserialize(element& n, T& v)
				{
					v = boost::lexical_cast<T>(n.content());
				}

	static element*
				 to_wsdl(type_map& types, element* parent,
					const std::string& name, const T& v, bool)
				{
					element* n(new element("xsd:element"));
					n->set_attribute("name", name);
					n->set_attribute("type", wsdl_name::type_name());
					n->set_attribute("minOccurs", "1");
					n->set_attribute("maxOccurs", "1");
//					n->set_attribute("default", boost::lexical_cast<std::string>(v));
					parent->append(n);
					
					return n;
				}
};

struct serialize_string
{
	static void	serialize(element* node, const std::string& name, const std::string& v)
				{
					node->set_attribute(name, v);
				}

	static void	serialize(element* parent, const std::string& name, const std::string& v, bool)
				{
					element* n(new element(name));
					n->content(v);
					parent->append(n);
				}

	static void	deserialize(const std::string& s, std::string& v)
				{
					v = s;
				}

	static void	deserialize(element& n, std::string& v)
				{
					v = n.content();
				}

	static element*
				to_wsdl(type_map& types, element* parent,
					const std::string& name, const std::string& v, bool)
				{
					element* n(new element("xsd:element"));
					n->set_attribute("name", name);
					n->set_attribute("type", "xsd:string");
					n->set_attribute("minOccurs", "1");
					n->set_attribute("maxOccurs", "1");
//					n->set_attribute("default", v);
					parent->append(n);
					
					return n;
				}
};

struct serialize_bool
{
	static void	serialize(element* node, const std::string& name, bool v)
				{
					node->set_attribute(name, v ? "true" : "false");
				}

	static void	serialize(element* parent, const std::string& name, bool v, bool)
				{
					element* n(new element(name));
					n->content(v ? "true" : "false");
					parent->append(n);
				}

	static void	deserialize(const std::string& s, bool& v)
				{
					v = s == "true" or s == "1";
				}

	static void	deserialize(element& n, bool& v)
				{
					deserialize(n.content(), v);
				}

	static element*
				to_wsdl(type_map& types, element* parent,
					const std::string& name, bool v, bool)
				{
					element* n(new element("xsd:element"));
					n->set_attribute("name", name);
					n->set_attribute("type", "xsd:boolean");
					n->set_attribute("minOccurs", "1");
					n->set_attribute("maxOccurs", "1");
//					n->set_attribute("default", v ? "true" : "false");
					parent->append(n);
					
					return n;
				}
};

template<typename S, typename T>
struct struct_serializer
{
	static void serialize(S& stream, T& data)
	{
		data.serialize(stream, 0U);
	}
};

/// \brief serializer/deserializer for boost::posix_time::ptime
/// boost::posix_time::ptime values are always assumed to be UTC
struct serialize_boost_posix_time_ptime
{
	/// Serialize the boost::posix_time::ptime as YYYY-MM-DDThh:mm:ssZ (zero UTC offset)
	static void	serialize(element* parent, const std::string& name, const boost::posix_time::ptime& v, bool)
	{
		element* n(new element(name));
		n->content( boost::posix_time::to_iso_extended_string(v).append("Z") );
		parent->append(n);
	}

	/// Deserialize according to ISO8601 rules.
	/// If Zulu time is specified, then the parsed xsd:dateTime is returned.
	/// If an UTC offset is present, then the offset is subtracted from the xsd:dateTime, this yields UTC.
	/// If no UTC offset is present, then the xsd:dateTime is assumed to be local time and converted to UTC.
	static void	deserialize(const std::string& s, boost::posix_time::ptime& v)
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
			t += boost::posix_time::microseconds((frac + .5) * 1e6);
		}

		v = boost::posix_time::ptime(d, t);

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
					v -= offs;
				} else {
					v += offs;
				}
			}
		} else {
			// Boost has no clear way of instantiating the *current* timezone, so
			// it's not possible to convert from local to UTC, using boost::local_time classes
			// For now, settle on using mktime...
			std::tm tm = boost::posix_time::to_tm(v);
			tm.tm_isdst = -1;
			std::time_t t = mktime(&tm);
			v = boost::posix_time::from_time_t(t);
		}
	}

	static void	deserialize(element& n, boost::posix_time::ptime& v)
	{
		deserialize(n.content(), v);
	}

	static element*
	to_wsdl(type_map& types, element* parent,
	const std::string& name, const boost::posix_time::ptime& v, bool)
	{
		element* n(new element("xsd:element"));
		n->set_attribute("name", name);
		n->set_attribute("type", "xsd:dateTime");
		n->set_attribute("minOccurs", "1");
		n->set_attribute("maxOccurs", "1");
		parent->append(n);
		return n;
	}
};

/// \brief serializer/deserializer for boost::gregorian::date
/// boost::gregorian::date values are assumed to be floating, i.e. we don't accept timezone info in dates
struct serialize_boost_gregorian_date
{
	/// Serialize the boost::gregorian::date as YYYY-MM-DD
	static void	serialize(element* parent, const std::string& name, const boost::gregorian::date& v, bool)
	{
		element* n(new element(name));
		n->content( boost::gregorian::to_iso_extended_string(v) );
		parent->append(n);
	}

	/// Deserialize boost::gregorian::date according to ISO8601 rules, but without timezone.
	static void	deserialize(const std::string& s, boost::gregorian::date& v)
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

		v = boost::gregorian::date(
		  boost::lexical_cast<int>(m[f_year])
		, boost::lexical_cast<int>(m[f_month])
		, boost::lexical_cast<int>(m[f_day])
		);
	}

	static void	deserialize(element& n, boost::gregorian::date& v)
	{
		deserialize(n.content(), v);
	}

	static element*
	to_wsdl(type_map& types, element* parent,
	const std::string& name, const boost::posix_time::ptime& v, bool)
	{
		element* n(new element("xsd:element"));
		n->set_attribute("name", name);
		n->set_attribute("type", "xsd:date");
		n->set_attribute("minOccurs", "1");
		n->set_attribute("maxOccurs", "1");
		parent->append(n);
		return n;
	}
};

/// \brief serializer/deserializer for boost::posix_time::time_duration
/// boost::posix_time::time_duration values are assumed to be floating, i.e. we don't accept timezone info in times
struct serialize_boost_posix_time_time_duration
{
	/// Serialize the boost::posix_time::time_duration as hh:mm:ss,ffffff
	static void	serialize(element* parent, const std::string& name, const boost::posix_time::time_duration& v, bool)
	{
		element* n(new element(name));
		n->content( boost::posix_time::to_simple_string(v) );
		parent->append(n);
	}

	/// Deserialize boost::posix_time::time_duration according to ISO8601 rules, but without timezone.
	static void	deserialize(const std::string& s, boost::posix_time::time_duration& v)
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
		v = boost::posix_time::time_duration(hours, minutes, seconds);

		if (m.length(f_have_frac)) {
			double frac = boost::lexical_cast<double>(std::string(".").append(std::string(m[f_frac])));
			v += boost::posix_time::microseconds((frac + .5) * 1e6);
		}
	}

	static void	deserialize(element& n, boost::posix_time::time_duration& v)
	{
		deserialize(n.content(), v);
	}

	static element*
	to_wsdl(type_map& types, element* parent,
	const std::string& name, const boost::posix_time::ptime& v, bool)
	{
		element* n(new element("xsd:element"));
		n->set_attribute("name", name);
		n->set_attribute("type", "xsd:time");
		n->set_attribute("minOccurs", "1");
		n->set_attribute("maxOccurs", "1");
		parent->append(n);
		return n;
	}
};

template<typename T>
struct serialize_struct
{
	static std::string	s_struct_name;
	typedef struct_serializer<serializer,T>		s_serializer;
	typedef struct_serializer<deserializer,T>	s_deserializer;
	typedef struct_serializer<wsdl_creator,T>	s_wsdl_creator;
	
	static void	serialize(element* parent, const std::string& name, T& v)
				{
					throw std::runtime_error("invalid serialization request");
				}

	static void	serialize(element* parent, const std::string& name, T& v, bool make_node)
				{
					if (make_node)
					{
						element* n(new element(name));
						serializer sr(n);
						s_serializer::serialize(sr, v);
						parent->append(n);
					}
					else
					{
						serializer sr(parent);
						s_serializer::serialize(sr, v);
					}
				}

	static void	deserialize(const std::string& s, T& v)
				{
					throw std::runtime_error("invalid deserialization request");
				}

	static void	deserialize(element& n, T& v)
				{
					deserializer ds(&n);
					s_deserializer::serialize(ds, v);
				}

	static element*
				 to_wsdl(type_map& types, element* parent,
					const std::string& name, T& v, bool make_node)
				{
					if (make_node)
					{
						element* result(new element("xsd:element"));
						result->set_attribute("name", name);
						result->set_attribute("type", kPrefix + ':' + s_struct_name);
						result->set_attribute("minOccurs", "1");
						result->set_attribute("maxOccurs", "1");
						parent->append(result);
	
						// we might be known already
						if (types.find(s_struct_name) != types.end())
							return result;
	
						element* n(new element("xsd:complexType"));
						n->set_attribute("name", s_struct_name);
						types[s_struct_name] = n;
						
						element* sequence(new element("xsd:sequence"));
						n->append(sequence);
						
						wsdl_creator wsdl(types, sequence);
						s_wsdl_creator::serialize(wsdl, v);
						
						return result;
					}
					else
					{
						wsdl_creator wc(types, parent);
						s_wsdl_creator::serialize(wc, v);
						return parent;
					}
				}
	
	static void	set_struct_name(const std::string& name)
				{
					s_struct_name = name;
				}
};

template<typename T>
std::string serialize_struct<T>::s_struct_name = typeid(T).name();

#endif

#define SOAP_XML_SET_STRUCT_NAME(s)	zeep::xml::serialize_struct<s>::s_struct_name = BOOST_PP_STRINGIZE(s);

#ifndef LIBZEEP_DOXYGEN_INVOKED

template<typename T, typename C = std::vector<T> >
struct serialize_container
{
	static void	serialize(element* parent, const std::string& name, C& v)
				{
					throw std::runtime_error("invalid serialization request");
				}
	static void	serialize(element* parent, const std::string& name, C& v, bool);

	static void	deserialize(const std::string& s, C& v)
				{
					throw std::runtime_error("invalid deserialization request");
				}

	static void	deserialize(element& n, C& v);

	static element*
				to_wsdl(type_map& types, element* parent,
					const std::string& name, const C& v, bool);
};

template<typename T>
struct serialize_boost_optional
{
	static void	serialize(element* parent, const std::string& name, boost::optional<T>& v, bool);

	static void	deserialize(const std::string& s, boost::optional<T>& v);
	static void	deserialize(element& n, boost::optional<T>& v);

	static element*	to_wsdl(type_map& types, element* parent, const std::string& name, boost::optional<T>& v, bool);
};

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
struct serialize_enum
{
	typedef enum_map<T>					t_enum_map;
	typedef std::map<T,std::string>		t_map;
	
	static void	serialize(element* node, const std::string& name, T& v)
				{
					node->set_attribute(name, t_enum_map::instance().m_name_mapping[v]);
				}

	static void	serialize(element* parent, const std::string& name, T& v, bool)
				{
					element* n(new element(name));
					n->content(t_enum_map::instance().m_name_mapping[v]);
					parent->append(n);
				}

	static void	deserialize(const std::string& s, T& v)
				{
					t_map& m = t_enum_map::instance().m_name_mapping;
					for (typename t_map::iterator e = m.begin(); e != m.end(); ++e)
					{
						if (e->second == s)
						{
							v = e->first;
							break;
						}
					}
				}

	static void	deserialize(element& n, T& v)
				{
					deserialize(n.content(), v);
				}

	static element*
				to_wsdl(type_map& types, element* parent, const std::string& name, const T& v, bool)
				{
					std::string type_name = t_enum_map::instance().m_name;
					
					element* result(new element("xsd:element"));
					result->set_attribute("name", name);
					result->set_attribute("type", kPrefix + ':' + type_name);
					result->set_attribute("minOccurs", "1");
					result->set_attribute("maxOccurs", "1");
//					result->set_attribute("default", t_enum_map::instance().m_name_mapping[v]);
					parent->append(result);
					
					// we might be known already
					if (types.find(type_name) != types.end())
						return result;
										
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
					
					return result;
				}
};

// now create a type factory for these serializers

template<typename T>
struct serialize_type
{
	typedef typename boost::mpl::if_c<
			boost::is_arithmetic<T>::value,
			serialize_arithmetic<T>,
			typename boost::mpl::if_c<
				boost::is_enum<T>::value,
				serialize_enum<T>,
				serialize_struct<T>
			>::type
		>::type					type;

	enum { is_container = false };
};

template<>
struct serialize_type<bool>
{
	typedef serialize_bool		type;

	enum { is_container = false };
};

template<>
struct serialize_type<std::string>
{
	typedef serialize_string	type;

	enum { is_container = false };
};

template<>
struct serialize_type<boost::posix_time::ptime>
{
	typedef serialize_boost_posix_time_ptime	type;

	enum { is_container = false };
};

template<>
struct serialize_type<boost::gregorian::date>
{
	typedef serialize_boost_gregorian_date	type;

	enum { is_container = false };
};

template<>
struct serialize_type<boost::posix_time::time_duration>
{
	typedef serialize_boost_posix_time_time_duration	type;

	enum { is_container = false };
};

template<typename T>
struct serialize_type<std::vector<T> >
{
	typedef serialize_container<T, std::vector<T> > type;

	enum { is_container = true };
};

template<typename T>
struct serialize_type<std::list<T> >
{
	typedef serialize_container<T, std::list<T> >	 type;

	enum { is_container = true };
};

template<typename T>
struct serialize_type<boost::optional<T> >
{
	typedef serialize_boost_optional<T>	type;

	enum { is_container = false };
};

// and the wrappers for serializing

template<typename T>
serializer& serializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;

	if (rhs.name()[0] == '@')
		s_type::serialize(m_node, rhs.name() + 1, rhs.value(), m_make_node);
	else
		s_type::serialize(m_node, rhs.name(), rhs.value(), m_make_node);

	return *this;
}

template<typename T>
deserializer& deserializer::operator&(
	const boost::serialization::nvp<T>&	rhs)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;

	if (serialize_type<value_type>::is_container)
	{
		BOOST_FOREACH (element* e, *m_node)
		{
			if (e->name() == rhs.name())
				s_type::deserialize(*e, rhs.value());
		}
	}
	else if (rhs.name()[0] == '@' and dynamic_cast<element*>(m_node) != nullptr)
		s_type::deserialize(static_cast<element*>(m_node)->get_attribute(rhs.name() + 1), rhs.value());
	else
	{
		element* n = m_node->find_first(rhs.name());
		if (n)
			s_type::deserialize(*n, rhs.value());
	}

	return *this;
}

template<typename T>
wsdl_creator& wsdl_creator::operator&(const boost::serialization::nvp<T>& rhs)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;

	s_type::to_wsdl(m_types, m_node, rhs.name(), rhs.value(), m_make_node);

	return *this;
}

// and some deferred implementations for the container types

template<typename T, typename C>
void serialize_container<T,C>::serialize(
	element* parent, const std::string& name, C& v, bool)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;
	
	for (typename C::iterator i = v.begin(); i != v.end(); ++i)
		s_type::serialize(parent, name, *i, true);
}

template<typename T, typename C>
void serialize_container<T,C>::deserialize(element& n, C& v)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;
	
	value_type e;
	s_type::deserialize(n, e);
	v.push_back(e);
}

template<typename T, typename C>
element* serialize_container<T,C>::to_wsdl(type_map& types,
	element* parent, const std::string& name, const C& v, bool)
{
	typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type	value_type;
	typedef typename serialize_type<value_type>::type	s_type;
	
	value_type e;
	element* result = s_type::to_wsdl(types, parent, name, e, true);

	result->remove_attribute("minOccurs");
	result->set_attribute("minOccurs", "0");
	
	result->remove_attribute("maxOccurs");
	result->set_attribute("maxOccurs", "unbounded");
	
	result->remove_attribute("default");

	return result;
}

// and some deferred implementations for the boost::optional type

template<typename T>
void serialize_boost_optional<T>::serialize(element* parent, const std::string& name, boost::optional<T>& rhs, bool)
{
	typedef typename serialize_type<T>::type		s_type;

	if (rhs.is_initialized()) {
		s_type::serialize(parent, name, rhs.get(), true);
	}
}

template<typename T>
void serialize_boost_optional<T>::deserialize(const std::string& s, boost::optional<T>& rhs)
{
	typedef typename serialize_type<T>::type		s_type;

	T e;
	s_type::deserialize(s, e);
	rhs = e;
}

template<typename T>
void serialize_boost_optional<T>::deserialize(element& n, boost::optional<T>& rhs)
{
	typedef typename serialize_type<T>::type		s_type;

	T e;
	s_type::deserialize(n, e);
	rhs = e;
}

template<typename T>
element* serialize_boost_optional<T>::to_wsdl(type_map& types, element* parent, const std::string& name, boost::optional<T>&, bool)
{
	typedef typename serialize_type<T>::type		s_type;

	T e;
	element* result = s_type::to_wsdl(types, parent, name, e, true);

	result->remove_attribute("minOccurs");
	result->set_attribute("minOccurs", "0");
		
	return result;
}

#endif

}
}

#endif
