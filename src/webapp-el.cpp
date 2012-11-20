// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp::el is a set of classes used to implement an 'expression language'
// A script language used in the XHTML templates used by the zeep webapp.
//

#include <zeep/config.hpp>

#include <iomanip>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>
#include <boost/tr1/cmath.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/http/webapp/el.hpp>
#include <zeep/xml/unicode_support.hpp>

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace http {
namespace el
{
	
#define ZEEP_DEFINE_AS_INT(T) \
template<> T object::as<T>() const					\
{													\
	T result = 0;									\
													\
	if (m_impl)										\
		result = static_cast<T>(m_impl->to_int());	\
													\
	return result;									\
}

ZEEP_DEFINE_AS_INT(int8)
ZEEP_DEFINE_AS_INT(uint8)
ZEEP_DEFINE_AS_INT(int16)
ZEEP_DEFINE_AS_INT(uint16)
ZEEP_DEFINE_AS_INT(int32)
ZEEP_DEFINE_AS_INT(uint32)
ZEEP_DEFINE_AS_INT(int64)
ZEEP_DEFINE_AS_INT(uint64)	
	
namespace detail
{

int64 object_impl::to_int() const
{
	throw exception("cannot convert to requested type");
}

double object_impl::to_double() const
{
	throw exception("cannot convert to requested type");
}

string object_impl::to_str() const
{
	throw exception("cannot convert to requested type");
}

string object_impl::to_JSON() const
{
	throw exception("cannot convert to JSON string");
}

object& base_array_object_impl::at(uint32 ix)
{
	throw exception("method 'at' not implemented");
}

const object base_array_object_impl::at(uint32 ix) const
{
	throw exception("method 'at' not implemented");
}

}

class int_object_impl : public detail::object_impl
{
  public:
					int_object_impl(int64 v)
						: detail::object_impl(object::number_type)
						, m_v(v)	{}

	virtual void	print(ostream& os) const			{ os << m_v; }
	virtual int		compare(object_impl* rhs) const;

	virtual int64	to_int() const						{ return m_v; }
	virtual double	to_double() const					{ return static_cast<double>(m_v); }
	virtual string	to_str() const						{ return boost::lexical_cast<string>(m_v); }
	virtual string	to_JSON() const						{ return boost::lexical_cast<string>(m_v); }

	int64			m_v;
};

class bool_object_impl : public int_object_impl
{
  public:
					bool_object_impl(bool v)
						: int_object_impl(object::number_type) {}

	virtual void	print(ostream& os) const			{ os << (m_v ? "true" : "false"); }
	virtual string	to_str() const						{ return m_v ? "true" : "false"; }
	virtual string	to_JSON() const						{ return m_v ? "true" : "false"; }
};

class float_object_impl : public detail::object_impl
{
  public:
					float_object_impl(double v)
						: detail::object_impl(object::number_type)
						, m_v(v) {}

	virtual void	print(ostream& os) const		{ os << m_v; }
	virtual int		compare(object_impl* rhs) const;

	virtual int64	to_int() const						{ return static_cast<int64>(tr1::round(m_v)); }
	virtual double	to_double() const					{ return m_v; }
	virtual string	to_str() const						{ return boost::lexical_cast<string>(m_v); }
	virtual string	to_JSON() const						{ return boost::lexical_cast<string>(m_v); }
	
	double			m_v;
};

class string_object_impl : public detail::object_impl
{
  public:
					string_object_impl(const string& v)
						: detail::object_impl(object::string_type)
						, m_v(v) {}

	virtual void	print(ostream& os) const		{ os << '"' << m_v << '"'; }
	virtual int		compare(object_impl* rhs) const;

	virtual int64	to_int() const						{ return boost::lexical_cast<int64>(m_v); }
	virtual double	to_double() const					{ return boost::lexical_cast<double>(m_v); }
	virtual string	to_str() const						{ return m_v; }
	virtual string	to_JSON() const
					{
						stringstream s;
						s << '"';
						foreach (char ch, m_v)
						{
							switch (ch)
							{
								case '"':	s << "\\\""; break;
								case '\\':	s << "\\\\"; break;
								case '/':	s << "\\/"; break;
								case 010:	s << "\\b"; break;
								case '\t':	s << "\\t"; break;
								case '\n':	s << "\\n"; break;
								case 014:	s << "\\f"; break;
								case '\r':	s << "\\r"; break;
								default:
								{
									if ((unsigned int)(ch) < 040)
										s << "\\u" << hex << setfill('0') << setw(4) << int(ch);
									else
										s << ch;
								}
							}
						}
						s << '"';
						return s.str();
					}
	
	string			m_v;
};

class vector_object_iterator_impl : public detail::object_iterator_impl
{
  public:

					vector_object_iterator_impl(vector<object>::const_iterator i)
						: m_i(i)		{}
	
	virtual void	increment()			{ ++m_i; }
	virtual object&	dereference()		{ return const_cast<object&>(*m_i); }
	virtual bool	equal(const object_iterator_impl* other)
					{
						const vector_object_iterator_impl* rhs = dynamic_cast<const vector_object_iterator_impl*>(other);
						if (rhs == nullptr)
							throw exception("comparing unequal iterators");
						return rhs->m_i == m_i;
					}

  private:
	vector<object>::const_iterator
					m_i;
};

class vector_object_impl : public detail::base_array_object_impl
{
  public:
					vector_object_impl(const vector<object>& v)
						: m_v(v) {}

					vector_object_impl(const vector<string>& v)
					{
						foreach (const string& s, v)
							m_v.push_back(object(s));
					}

	virtual object&	at(uint32 ix)
					{
						return m_v[ix];
					}
					
	virtual const object
					at(uint32 ix) const
					{
						return m_v[ix];
					}

	virtual object&	operator[](const object& index)
					{
						return m_v[index.as<uint32>()];
					}

	virtual const object
					operator[](const object& index) const
					{
						return m_v[index.as<uint32>()];
					}

	virtual void	print(ostream& os) const
					{
						os << '[';
						bool first = true;
						foreach (const object& o, m_v)
						{
							if (not first)
								os << ',';
							first = false;
							os << o;
						}
						os << ']';
					}
	virtual int		compare(object_impl* rhs) const;
	virtual size_t	count() const					{ return m_v.size(); }

	virtual detail::object_iterator_impl*
					create_iterator(bool begin) const
					{
						if (begin)
							return new vector_object_iterator_impl(m_v.begin());
						else
							return new vector_object_iterator_impl(m_v.end());
					}
	
	virtual string	to_JSON() const
					{
						stringstream s;
						s << '[';
						for (uint32 ix = 0; ix < m_v.size(); ++ix)
						{
							if (ix > 0)
								s << ',';
							s << m_v[ix].toJSON();
						}
						s << ']';
						return s.str();
					}

	vector<object>	m_v;
};

class struct_object_impl : public detail::base_struct_object_impl
{
  public:
	virtual void	print(ostream& os) const
					{
						os << '{';
						bool first = true;
						typedef pair<string,object> value_type;
						foreach (const value_type& o, m_v)
						{
							if (not first)
								os << ',';
							first = false;
							os << o.first << ':' << o.second;
							
						}
						os << '}';
					}

	virtual int		compare(object_impl* rhs) const;

	virtual object&	field(const string& name)
					{
//						icpc creates crashing code on this line:
//						pair<map<string,object>::iterator,bool> i = m_v.insert(make_pair(name, object()));

						map<string,object>::iterator i = m_v.find(name);
						if (i == m_v.end())
						{
							m_v[name] = object();
							i = m_v.find(name);
						}
						
						return i->second;
					}

	virtual const object
					field(const string& name) const
					{
						object result;
					
						map<string,object>::const_iterator i = m_v.find(name);
						if (i != m_v.end())
							result = i->second;
						
						return result;
					}

	
	virtual string	to_JSON() const
					{
						stringstream s;
						s << '{';
						for (map<string,object>::const_iterator o = m_v.begin(); o != m_v.end(); ++o)
						{
							if (o != m_v.begin())
								s << ',';
							s << '"' << o->first << '"' << ':' << o->second.toJSON();
						}
						s << '}';
						return s.str();
					}

  private:
	map<string,object>
					m_v;
};

// --------------------------------------------------------------------
// compare methods

int int_object_impl::compare(object_impl* rhs) const
{
	int result = 0;
	
	if (dynamic_cast<int_object_impl*>(rhs))
	{
		if (m_v < static_cast<int_object_impl*>(rhs)->m_v)
			result = -1;
		else if (m_v > static_cast<int_object_impl*>(rhs)->m_v)
			result = 1;
	}
	else if (dynamic_cast<float_object_impl*>(rhs))
	{
		if (m_v < static_cast<float_object_impl*>(rhs)->m_v)
			result = -1;
		else if (m_v > static_cast<float_object_impl*>(rhs)->m_v)
			result = 1;
	}
	else if (dynamic_cast<string_object_impl*>(rhs))
	{
		double rv = rhs->to_double();
		if (m_v < rv)
			result = -1;
		else if (m_v > rv)
			result = 1;
	}
	else
		throw exception("incompatible types for compare");
	
	return result;
}

int float_object_impl::compare(object_impl* rhs) const
{
	int result = 0;
	
	if (dynamic_cast<int_object_impl*>(rhs))
	{
		if (m_v < static_cast<int_object_impl*>(rhs)->m_v)
			result = -1;
		else if (m_v > static_cast<int_object_impl*>(rhs)->m_v)
			result = 1;
	}
	else if (dynamic_cast<float_object_impl*>(rhs))
	{
		if (m_v < static_cast<float_object_impl*>(rhs)->m_v)
			result = -1;
		else if (m_v > static_cast<float_object_impl*>(rhs)->m_v)
			result = 1;
	}
	else if (dynamic_cast<string_object_impl*>(rhs))
	{
		double rv = rhs->to_double();
		if (m_v < rv)
			result = -1;
		else if (m_v > rv)
			result = 1;
	}
	else
		throw exception("incompatible types for compare");
	
	return result;
}

int string_object_impl::compare(object_impl* rhs) const
{
	int result = 0;
	
	if (dynamic_cast<int_object_impl*>(rhs))
	{
		double v = to_double();
		if (v < static_cast<int_object_impl*>(rhs)->m_v)
			result = -1;
		else if (v > static_cast<int_object_impl*>(rhs)->m_v)
			result = 1;
	}
	else if (dynamic_cast<float_object_impl*>(rhs))
	{
		double v = to_double();
		if (v < static_cast<float_object_impl*>(rhs)->m_v)
			result = -1;
		else if (v > static_cast<float_object_impl*>(rhs)->m_v)
			result = 1;
	}
	else if (dynamic_cast<string_object_impl*>(rhs))
		result = m_v.compare(static_cast<string_object_impl*>(rhs)->m_v);
	else
		throw exception("incompatible types for compare");
	
	return result;
}

int vector_object_impl::compare(object_impl* rhs) const
{
	int result = 0;
	
	if (dynamic_cast<vector_object_impl*>(rhs))
	{
		if (m_v < static_cast<vector_object_impl*>(rhs)->m_v)
			result = -1;
		else if (m_v > static_cast<vector_object_impl*>(rhs)->m_v)
			result = -1;
	}
	else
		throw exception("incompatible types for compare");
	
	return result;
}

int struct_object_impl::compare(object_impl* rhs) const
{
	int result = 0;
	
	if (dynamic_cast<struct_object_impl*>(rhs))
	{
		if (m_v != static_cast<struct_object_impl*>(rhs)->m_v)
			result = 1;
	}
	else
		throw exception("incompatible types for compare");
	
	return result;
}

// --------------------------------------------------------------------
// basic object methods

object::object()
	: m_impl(nullptr)
{
}

object::object(detail::object_impl* impl)
	: m_impl(impl)
{
}

object::object(const object& o)
	: m_impl(o.m_impl)
{
	if (m_impl != nullptr)
		m_impl->reference();
}

object::~object()
{
	if (m_impl != nullptr)
		m_impl->release();
}

object& object::operator=(const object& o)
{
	if (this != &o)
	{
		if (m_impl != nullptr)
			m_impl->release();
		m_impl = o.m_impl;
		if (m_impl != nullptr)
			m_impl->reference();
	}
	return *this;
}

// --------------------------------------------------------------------
// explicit constructors and assignment

object::object(const vector<object>& v)
	: m_impl(new vector_object_impl(v))
{
}

object::object(const vector<string>& v)
	: m_impl(new vector_object_impl(v))
{
}

object::object(bool v)
//	: m_impl(new bool_object_impl(v))
	: m_impl(new int_object_impl(v))
{
}

object::object(int8 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(uint8 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(int16 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(uint16 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(int32 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(uint32 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(int64 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(uint64 v)
	: m_impl(new int_object_impl(v))
{
}

object::object(float v)
	: m_impl(new float_object_impl(v))
{
}

object::object(double v)
	: m_impl(new float_object_impl(v))
{
}

object::object(const char* v)
	: m_impl(nullptr)
{
	if (v != nullptr)
		m_impl = new string_object_impl(v);
}

object::object(const string& v)
	: m_impl(new string_object_impl(v))
{
}

object& object::operator=(const vector<object>& v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new vector_object_impl(v);
	return *this;
}

object& object::operator=(const vector<string>& v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new vector_object_impl(v);
	return *this;
}

object& object::operator=(bool v)
{
	if (m_impl != nullptr)
		m_impl->release();
//	m_impl = new bool_object_impl(v);
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(int8 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(uint8 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(int16 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(uint16 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(int32 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(uint32 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(int64 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(uint64 v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new int_object_impl(v);
	return *this;
}

object& object::operator=(float v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new float_object_impl(v);
	return *this;
}

object& object::operator=(double v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new float_object_impl(v);
	return *this;
}

object& object::operator=(const char* v)
{
	if (m_impl != nullptr)
		m_impl->release();
	if (v == nullptr)
		m_impl = nullptr;
	else
		m_impl = new string_object_impl(v);
	return *this;
}

object& object::operator=(const string& v)
{
	if (m_impl != nullptr)
		m_impl->release();
	m_impl = new string_object_impl(v);
	return *this;
}

object::object_type object::type() const
{
	object_type result = null_type;
	if (m_impl != nullptr)
		result = m_impl->type();
	return result;
}

size_t object::count() const
{
	if (type() != array_type)
		throw exception("count/length is only defined for array types");
	
	return static_cast<const detail::base_array_object_impl*>(m_impl)->count();
}

bool object::empty() const
{
	bool result = true;
	
	switch (type())
	{
		case null_type:		result = true; break;
		case string_type:	result = m_impl->to_str().empty(); break;
		case array_type:	result = static_cast<detail::base_array_object_impl*>(m_impl)->count() == 0; break;
		default:			result = true; break;
	}
	
	return result;
}

template<> string object::as<string>() const
{
	string result;

	if (m_impl)
		result = m_impl->to_str();

	return result;
}

template<> bool object::as<bool>() const
{
	bool result = false;

	if (dynamic_cast<int_object_impl*>(m_impl))
		result = static_cast<int_object_impl*>(m_impl)->m_v != 0;
	else if (dynamic_cast<float_object_impl*>(m_impl))
		result = static_cast<float_object_impl*>(m_impl)->m_v != 0;
	else if (type() == array_type)
		result = static_cast<detail::base_array_object_impl*>(m_impl)->count() != 0;
	else if (type() == string_type)
		result = not as<string>().empty() and as<string>() != "false";
	else if (type() == struct_type)
		result = true;

	return result;
}

template<> double object::as<double>() const
{
	double result = 0;

	if (m_impl)
		result = m_impl->to_double();

	return result;
}

const object object::operator[](const string& name) const
{
	object result;

	if (type() == struct_type)
	{
		const detail::base_struct_object_impl* impl = static_cast<const detail::base_struct_object_impl*>(m_impl);
		result = impl->field(name);
	}

	return result;
}

const object object::operator[](const char* name) const
{
	if (name != nullptr)
		return operator[](string(name));
	return object();
}

const object object::operator[](const object& index) const
{
	object result;

	switch (type())
	{
		case array_type:
		{
			const detail::base_array_object_impl* impl = static_cast<const detail::base_array_object_impl*>(m_impl);
			if (impl != nullptr and index.as<uint32>() < impl->count())
				result = impl->at(index.as<uint32>());
			break;
		}
		
		case struct_type:
		{
			const detail::base_struct_object_impl* impl = static_cast<const detail::base_struct_object_impl*>(m_impl);
			result = impl->field(index.as<string>());
			break;
		}
	}
	
	return result;
}

object& object::operator[](const string& name)
{
	if (name.empty())
		throw exception("invalid empty name for structure object");
	return operator[](object(name));
}

object& object::operator[](const char* name)
{
	if (name == nullptr)
		throw exception("invalid empty name for structure object");
	return operator[](object(name));
}

object& object::operator[](const object& index)
{
	if (type() == array_type)
	{
		vector_object_impl* impl = static_cast<vector_object_impl*>(m_impl);
		return impl->at(index.as<uint32>());
	}
	else if (type() == struct_type)
	{
		struct_object_impl* impl = static_cast<struct_object_impl*>(m_impl);
		return impl->field(index.as<string>());
	}
	else
	{
		if (m_impl != nullptr)
			m_impl->release();

		struct_object_impl* impl;
		m_impl = impl = new struct_object_impl();

		return impl->field(index.as<string>());	
	}
}

bool object::operator<(const object& rhs) const
{
	bool result = false;
	if (m_impl != nullptr and rhs.m_impl != nullptr)
		result = m_impl->compare(rhs.m_impl) < 0;
	return result;
}

bool object::operator==(const object& rhs) const
{
	bool result = false;
	if (m_impl != nullptr and rhs.m_impl != nullptr)
		result = m_impl->compare(rhs.m_impl) == 0;
	return result;
}

bool operator<=(const object& a, const object& b)
{
	return a < b or a == b;
}

struct compare_object
{
	compare_object(const string& field, bool descending)
		: m_field(field)
		, m_descending(descending)
	{
	}
	
	bool		operator()(const object& a, const object& b) const;

	string		m_field;
	bool		m_descending;
};

//void object::sort(const string& sort_field, bool descending)
//{
//	if (m_type == ot_array)
//		sort(m_array.begin(), m_array.end(), compare_object(sort_field, descending));
//}

ostream& operator<<(ostream& os, const object& o)
{
	if (o.m_impl != nullptr)
		o.m_impl->print(os);
	else
		os << "null";
	return os;
}

string object::toJSON() const
{
	string result;
	if (m_impl != nullptr)
		result = m_impl->to_JSON();
	return result;
}

object operator+(const object& a, const object& b)
{
	object result;
	
	if (typeid(*a.m_impl) == typeid(*b.m_impl))
	{
		if (dynamic_cast<int_object_impl*>(a.m_impl))
			result = static_cast<int_object_impl*>(a.m_impl)->m_v + static_cast<int_object_impl*>(b.m_impl)->m_v;
		else if (dynamic_cast<float_object_impl*>(a.m_impl))
			result = static_cast<float_object_impl*>(a.m_impl)->m_v + static_cast<float_object_impl*>(b.m_impl)->m_v;
		else if (dynamic_cast<string_object_impl*>(a.m_impl))
			result = static_cast<string_object_impl*>(a.m_impl)->m_v + static_cast<string_object_impl*>(b.m_impl)->m_v;
		else
			throw exception("incompatible types in add operator");
	}
	else if (dynamic_cast<string_object_impl*>(a.m_impl) or dynamic_cast<string_object_impl*>(b.m_impl))
		result = a.as<string>() + b.as<string>();
	else if (dynamic_cast<float_object_impl*>(a.m_impl) or dynamic_cast<float_object_impl*>(b.m_impl))
		result = a.as<double>() + b.as<double>();
	else
		result = a.as<int64>() + b.as<int64>();
	
	return result;
}

object operator-(const object& a, const object& b)
{
	object result;
	
	if (dynamic_cast<float_object_impl*>(a.m_impl) or dynamic_cast<float_object_impl*>(b.m_impl))
		result = a.as<double>() - b.as<double>();
	else
		result = a.as<int64>() - b.as<int64>();
	
	return result;
}

object operator*(const object& a, const object& b)
{
	object result;
	
	if (dynamic_cast<float_object_impl*>(a.m_impl) or dynamic_cast<float_object_impl*>(b.m_impl))
		result = a.as<double>() * b.as<double>();
	else
		result = a.as<int64>() * b.as<int64>();
	
	return result;
}

object operator/(const object& a, const object& b)
{
	return object(a.as<double>() * b.as<double>());
}

object operator%(const object& a, const object& b)
{
	object result;
	
	if (dynamic_cast<int_object_impl*>(a.m_impl) or dynamic_cast<int_object_impl*>(b.m_impl))
		result = a.as<int64>() % b.as<int64>();
	
	return result;
}

object operator-(const object& a)
{
	object result;
	
	if (dynamic_cast<float_object_impl*>(a.m_impl))
		result = - a.as<double>();
	else
		result = - a.as<int64>();
	
	return result;
}

bool compare_object::operator()(const object& a, const object& b) const
{
	if (m_descending)
		return b[m_field] < a[m_field];
	else
		return a[m_field] < b[m_field];
}

// --------------------------------------------------------------------
// interpreter for expression language

struct interpreter
{
	typedef uint32	unicode;
	
					interpreter(
						const scope&			scope)
						: m_scope(scope) {}

	template<class OutputIterator, class Match>
	OutputIterator	operator()(Match& m, OutputIterator out, boost::regex_constants::match_flag_type);

	object			evaluate(
						const string&	s);

	void			process(
						string&			s);
	
	void			match(
						uint32					t);

	unsigned char	next_byte();
	unicode			get_next_char();
	void			retract();
	void			get_next_token();

	object			parse_expr();					// or_expr ( '?' expr ':' expr )?
	object			parse_or_expr();				// and_expr ( 'or' and_expr)*
	object			parse_and_expr();				// equality_expr ( 'and' equality_expr)*
	object			parse_equality_expr();			// relational_expr ( ('=='|'!=') relational_expr )?
	object			parse_relational_expr();		// additive_expr ( ('<'|'<='|'>='|'>') additive_expr )*
	object			parse_additive_expr();			// multiplicative_expr ( ('+'|'-') multiplicative_expr)*
	object			parse_multiplicative_expr();	// unary_expr (('%'|'/') unary_expr)*
	object			parse_unary_expr();				// ('-')? primary_expr
	object			parse_primary_expr();			// '(' expr ')' | number | string
	
	const scope&	m_scope;
	uint32			m_lookahead;
	string		m_token_string;
	double			m_token_number;
	string::const_iterator
					m_ptr, m_end;
};

template<class OutputIterator, class Match>
inline
OutputIterator interpreter::operator()(Match& m, OutputIterator out, boost::regex_constants::match_flag_type)
{
	string s(m[1]);

	process(s);
	
	copy(s.begin(), s.end(), out);

	return out;
}



enum token_type
{
	elt_undef,
	elt_eof,
	elt_number,
	elt_string,
	elt_object,
	elt_and, elt_or, elt_not, elt_empty,
	elt_eq, elt_ne, elt_lt, elt_le, elt_ge, elt_gt,
	elt_plus, elt_minus, elt_div, elt_mod, elt_mult,
	elt_lparen, elt_rparen, elt_lbracket, elt_rbracket,
	elt_if, elt_else,
	elt_dot
};

object interpreter::evaluate(
	const string&	s)
{
	object result;

	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();
		
		if (m_lookahead != elt_eof)
			result = parse_expr();
		match(elt_eof);
	}
	catch (exception& /* e */)
	{
//		if (VERBOSE)
//			cerr << e.what() << endl;
	}
	
	return result;
}

void interpreter::process(
	string&			s)
{
	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();
		
		object result;
		
		if (m_lookahead != elt_eof)
			result = parse_expr();
		match(elt_eof);
		
		s = result.as<string>();
	}
	catch (exception& e)
	{
//		if (VERBOSE)
//			cerr << e.what() << endl;

		s = "error in el expression: ";
		s += e.what();
	}
}

void interpreter::match(
	uint32		t)
{
	if (t != m_lookahead)
		throw zeep::exception("syntax error");
	get_next_token();
}

unsigned char interpreter::next_byte()
{
	char result = 0;
	
	if (m_ptr < m_end)
	{
		result = *m_ptr;
		++m_ptr;
	}

	m_token_string += result;

	return static_cast<unsigned char>(result);
}

// We assume all paths are in valid UTF-8 encoding
interpreter::unicode interpreter::get_next_char()
{
	unicode result = 0;
	unsigned char ch[5];
	
	ch[0] = next_byte();
	
	if ((ch[0] & 0x080) == 0)
		result = ch[0];
	else if ((ch[0] & 0x0E0) == 0x0C0)
	{
		ch[1] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F);
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F);
	}

	if (result > 0x10ffff)
		throw zeep::exception("invalid utf-8 character (out of range)");
	
	return result;
}

void interpreter::retract()
{
	string::iterator c = m_token_string.end();

	// skip one valid character back in the input buffer
	// since we've arrived here, we can safely assume input
	// is valid UTF-8
	do --c; while ((*c & 0x0c0) == 0x080);
	
	if (m_ptr != m_end or *c != 0)
		m_ptr -= m_token_string.end() - c;
	m_token_string.erase(c, m_token_string.end());
}

void interpreter::get_next_token()
{
	enum State {
		els_Start,
		els_Equals,
		els_ExclamationMark,
		els_LessThan,
		els_GreaterThan,
		els_Number,
		els_NumberFraction,
		els_Name,
		els_Literal
	} start, state;
	start = state = els_Start;

	token_type token = elt_undef;
	double fraction = 1.0;
	unicode quoteChar = 0;

	m_token_string.clear();
	
	while (token == elt_undef)
	{
		unicode ch = get_next_char();
		
		switch (state)
		{
			case els_Start:
				switch (ch)
				{
					case 0:		token = elt_eof; break;
					case '(':	token = elt_lparen; break;
					case ')':	token = elt_rparen; break;
					case '[':	token = elt_lbracket; break;
					case ']':	token = elt_rbracket; break;
					case ':':	token = elt_else; break;
					case '?':	token = elt_if; break;
					case '*':	token = elt_mult; break;
					case '/':	token = elt_div; break;
					case '+':	token = elt_plus; break;
					case '-':	token = elt_minus; break;
					case '.':	token = elt_dot; break;
					case '=':	state = els_Equals; break;
					case '!':	state = els_ExclamationMark; break;
					case '<':	state = els_LessThan; break;
					case '>':	state = els_GreaterThan; break;
					case ' ':
					case '\n':
					case '\r':
					case '\t':
						m_token_string.clear();
						break;
					case '\'':	quoteChar = ch; state = els_Literal; break;
					case '"':	quoteChar = ch; state = els_Literal; break;

					default:
						if (ch >= '0' and ch <= '9')
						{
							m_token_number = ch - '0';
							state = els_Number;
						}
						else if (zeep::xml::is_name_start_char(ch))
							state = els_Name;
						else
							throw zeep::exception((boost::format("invalid character (%1%) in expression") % ch).str());
				}
				break;
			
			case els_Equals:
				if (ch != '=')
					retract();
				token = elt_eq;
				break;
			
			case els_ExclamationMark:
				if (ch != '=')
				{
					retract();
					throw zeep::exception("unexpected character ('!') in expression");
				}
				token = elt_ne;
				break;
			
			case els_LessThan:
				if (ch == '=')
					token = elt_le;
				else
				{
					retract();
					token = elt_lt;
				}
				break;

			case els_GreaterThan:
				if (ch == '=')
					token = elt_ge;
				else
				{
					retract();
					token = elt_gt;
				}
				break;
			
			case els_Number:
				if (ch >= '0' and ch <= '9')
					m_token_number = 10 * m_token_number + (ch - '0');
				else if (ch == '.')
				{
					fraction = 0.1;
					state = els_NumberFraction;
				}
				else
				{
					retract();
					token = elt_number;
				}
				break;
			
			case els_NumberFraction:
				if (ch >= '0' and ch <= '9')
				{
					m_token_number += fraction * (ch - '0');
					fraction /= 10;
				}
				else
				{
					retract();
					token = elt_number;
				}
				break;
			
			case els_Name:
				if (ch == '.' or not zeep::xml::is_name_char(ch))
				{
					retract();
					if (m_token_string == "div")
						token = elt_div;
					else if (m_token_string == "mod")
						token = elt_mod;
					else if (m_token_string == "and")
						token = elt_and;
					else if (m_token_string == "or")
						token = elt_or;
					else if (m_token_string == "not")
						token = elt_not;
					else if (m_token_string == "empty")
						token = elt_empty;
					else if (m_token_string == "lt")
						token = elt_lt;
					else if (m_token_string == "le")
						token = elt_le;
					else if (m_token_string == "ge")
						token = elt_ge;
					else if (m_token_string == "gt")
						token = elt_gt;
					else if (m_token_string == "ne")
						token = elt_ne;
					else if (m_token_string == "eq")
						token = elt_eq;
					else
						token = elt_object;
				}
				break;
			
			case els_Literal:
				if (ch == 0)
					throw zeep::exception("run-away string, missing quote character?");
				else if (ch == quoteChar)
				{
					token = elt_string;
					m_token_string = m_token_string.substr(1, m_token_string.length() - 2);
				}
				break;
		}
	}
	
	m_lookahead = token;
}

object interpreter::parse_expr()
{
	object result = parse_or_expr();
	if (m_lookahead == elt_if)
	{
		match(m_lookahead);
		object a = parse_expr();
		match(elt_else);
		object b = parse_expr();
		if (result.as<bool>())
			result = a;
		else
			result = b;
	}
	return result;
}

object interpreter::parse_or_expr()
{
	object result = parse_and_expr();
	while (m_lookahead == elt_or)
	{
		match(m_lookahead);
		bool b1 = result.as<bool>();
		bool b2 = parse_and_expr().as<bool>();
		result = b1 or b2;
	}
	return result;
}

object interpreter::parse_and_expr()
{
	object result = parse_equality_expr();
	while (m_lookahead == elt_and)
	{
		match(m_lookahead);
		bool b1 = result.as<bool>();
		bool b2 = parse_equality_expr().as<bool>();
		result = b1 and b2;
	}
	return result;
}

object interpreter::parse_equality_expr()
{
	object result = parse_relational_expr();
	if (m_lookahead == elt_eq)
	{
		match(m_lookahead);
		result = (result == parse_relational_expr());
	}
	else if (m_lookahead == elt_ne)
	{
		match(m_lookahead);
		result = not (result == parse_relational_expr());
	}
	return result;
}

object interpreter::parse_relational_expr()
{
	object result = parse_additive_expr();
	switch (m_lookahead)
	{
		case elt_lt:	match(m_lookahead); result = (result < parse_additive_expr()); break;
		case elt_le:	match(m_lookahead); result = (result <= parse_additive_expr()); break;
		case elt_ge:	match(m_lookahead); result = (parse_additive_expr() <= result); break;
		case elt_gt:	match(m_lookahead); result = (parse_additive_expr() < result); break;
		default:		break;
	}
	return result;
}

object interpreter::parse_additive_expr()
{
	object result = parse_multiplicative_expr();
	while (m_lookahead == elt_plus or m_lookahead == elt_minus)
	{
		if (m_lookahead == elt_plus)
		{
			match(m_lookahead);
			result = (result + parse_multiplicative_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result - parse_multiplicative_expr());
		}
	}
	return result;
}

object interpreter::parse_multiplicative_expr()
{
	object result = parse_unary_expr();
	while (m_lookahead == elt_div or m_lookahead == elt_mod or m_lookahead == elt_mult)
	{
		if (m_lookahead == elt_mult)
		{
			match(m_lookahead);
			result = (result * parse_unary_expr());
		}
		else if (m_lookahead == elt_div)
		{
			match(m_lookahead);
			result = (result / parse_unary_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result % parse_unary_expr());
		}
	}
	return result;
}

object interpreter::parse_unary_expr()
{
	object result;
	if (m_lookahead == elt_minus)
	{
		match(m_lookahead);
		result = -(parse_primary_expr());
	}
	else if (m_lookahead == elt_not)
	{
		match(m_lookahead);
		result = not parse_primary_expr().as<bool>();
	}
	else
		result = parse_primary_expr();
	return result;
}

object interpreter::parse_primary_expr()
{
	object result;
	switch (m_lookahead)
	{
		case elt_number:	result = m_token_number; match(m_lookahead); break;
		case elt_string:	result = m_token_string; match(m_lookahead); break;
		case elt_lparen:
			match(m_lookahead);
			result = parse_expr();
			match(elt_rparen);
			break;
		case elt_object:
			result = m_scope.lookup(m_token_string);
			match(elt_object);
			for (;;)
			{
				if (m_lookahead == elt_dot)
				{
					match(m_lookahead);
					if (result.type() == object::array_type and (m_token_string == "count" or m_token_string == "length"))
						result = object(result.count());
					else
						result = const_cast<const object&>(result)[m_token_string];
					match(elt_object);
					continue;
				}
				
				if (m_lookahead == elt_lbracket)
				{
					match(m_lookahead);
					
					object index = parse_expr();
					match(elt_rbracket);
					
					if (index.empty() or (result.type() != object::array_type and result.type() != object::struct_type))
						result = object();
					else
						result = const_cast<object&>(result)[index];
					continue;
				}

				break;
			}
			break;
		case elt_empty:
			match(m_lookahead);
			if (m_lookahead != elt_object)
				throw zeep::exception("syntax error, expected an object after operator 'empty'");
			result = parse_primary_expr().empty();
			break;
		default:
			throw zeep::exception("syntax error, expected number, string or object");
	}
	return result;
}

// --------------------------------------------------------------------
// interpreter calls

bool process_el(
	const el::scope&			scope,
	string&						text)
{
	static const boost::regex re("\\$\\{([^}]+)\\}");

	el::interpreter interpreter(scope);
	
	ostringstream os;
	ostream_iterator<char, char> out(os);
	boost::regex_replace(out, text.begin(), text.end(), re, interpreter,
		boost::match_default | boost::format_all);
	
	bool result = false;
	if (os.str() != text)
	{
//cerr << "processed \"" << text << "\" => \"" << os.str() << '"' << endl;
		text = os.str();
		result = true;
	}
	
	return result;
}

void evaluate_el(
	const el::scope&			scope,
	const string&				text,
	el::object&					result)
{
	static const boost::regex re("^\\$\\{([^}]+)\\}$");
	
	boost::smatch m;
	if (boost::regex_match(text, m, re))
	{
		el::interpreter interpreter(scope);
		result = interpreter.evaluate(m[1]);
//cerr << "evaluated \"" << text << "\" => \"" << result << '"' << endl;
	}
	else
		result = text;
}

bool evaluate_el(
	const el::scope&			scope,
	const string&				text)
{
	el::object result;
	evaluate_el(scope, text, result);
	return result.as<bool>();
}

// --------------------------------------------------------------------
// scope



ostream& operator<<(ostream& lhs, const scope& rhs)
{
	const scope* s = &rhs;
	while (s != nullptr)
	{
		foreach (scope::data_map::value_type e, s->m_data)
			lhs << e.first << " = " << e.second << endl;
		s = s->m_next;
	}
	return lhs;
}

scope::scope(const scope& next)
	: m_next(const_cast<scope*>(&next))
	, m_req(nullptr)
{
}

scope::scope(const request& req)
	: m_next(nullptr)
	, m_req(&req)
{
}

object& scope::operator[](
	const string& name)
{
	return lookup(name);
}

const object& scope::lookup(
	const string&	name) const
{
	map<string,object>::const_iterator i = m_data.find(name);
	if (i != m_data.end())
		return i->second;
	else if (m_next != nullptr)
		return m_next->lookup(name);
	
	static object s_null;
	return s_null;
}

const object& scope::operator[](
	const string& name) const
{
	return lookup(name);
}

object& scope::lookup(
	const string&	name)
{
	object* result = nullptr;

	map<string,object>::iterator i = m_data.find(name);
	if (i != m_data.end())
		result = &i->second;
	else if (m_next != nullptr)
		result = &m_next->lookup(name);
	else
	{
		m_data[name] = object();
		result = &m_data[name];
	}
	 
	 return *result;
}

const request& scope::get_request() const
{
	if (m_next)
		return m_next->get_request();
	if (m_req == nullptr)
		throw zeep::exception("Invalid scope, no request");
	return *m_req;
}

}
}
}
