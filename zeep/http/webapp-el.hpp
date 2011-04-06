// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

typedef int8_t		int8;
typedef uint8_t		uint8;
typedef int16_t		int16;
typedef uint16_t	uint16;
typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int64_t		int64;
typedef uint64_t	uint64;

namespace zeep {
namespace http {
namespace el
{
	
typedef uint32	unicode;

class object
{
  public:
					object();
					object(const object& o);

	explicit		object(const char* s);
	explicit		object(const std::string& s);
	explicit		object(double n);
	explicit		object(int8 n);
	explicit		object(uint8 n);
	explicit		object(int16 n);
	explicit		object(uint16 n);
	explicit		object(int32 n);
	explicit		object(uint32 n);
	explicit		object(int64 n);
	explicit		object(uint64 n);
	explicit		object(bool b);
	explicit		object(const std::vector<object>& a);

	object&			operator=(const object& rhs);					
	object&			operator=(const std::string& rhs);
	object&			operator=(double rhs);
	object&			operator=(bool rhs);

	bool			empty() const;
	bool			undefined() const;

	bool			is_number() const;

	bool			is_array() const;
	uint32			count() const;

					operator std::string() const;
					operator double() const;
					operator bool() const;
	
	const object	operator[](const std::string& name) const;
	const object	operator[](const char* name) const;
	const object	operator[](uint32 ix) const;

	object&			operator[](const std::string& name);
	object&			operator[](const char* name);
	object&			operator[](uint32 ix);

	void			sort(const std::string& sort_field, bool descending);

	friend std::ostream& operator<<(std::ostream& os, const object& o);
	friend object operator<(const object& a, const object& b);
	friend object operator<=(const object& a, const object& b);
	friend object operator>=(const object& a, const object& b);
	friend object operator>(const object& a, const object& b);
	friend object operator!=(const object& a, const object& b);
	friend object operator==(const object& a, const object& b);
	friend object operator+(const object& a, const object& b);
	friend object operator-(const object& a, const object& b);
	friend object operator*(const object& a, const object& b);
	friend object operator%(const object& a, const object& b);
	friend object operator/(const object& a, const object& b);
	friend object operator&&(const object& a, const object& b);
	friend object operator||(const object& a, const object& b);
	friend object operator-(const object& a);
	
	friend std::vector<object>::iterator range_begin(object& x);
	friend std::vector<object>::iterator range_end(object& x);
	friend std::vector<object>::const_iterator range_begin(object const& x);
	friend std::vector<object>::const_iterator range_end(object const& x);

  private:
	enum object_type
	{
		ot_undef,
		ot_number,
		ot_string,
		ot_struct,
		ot_array,
		ot_boolean
	}				m_type;
	std::string		m_string;
	double			m_number;
	std::map<std::string,object>
					m_fields;
	std::vector<object>	m_array;
};

// boost foreach support
inline std::vector<object>::iterator range_begin(object& x)
{
    return x.m_array.begin();
}

inline std::vector<object>::iterator range_end(object& x)
{
    return x.m_array.end();
}

inline std::vector<object>::const_iterator range_begin(object const& x)
{
    return x.m_array.begin();
}

inline std::vector<object>::const_iterator range_end(object const& x)
{
    return x.m_array.end();
}


std::ostream& operator<<(std::ostream& os, const object& o);
object operator<(const object& a, const object& b);
object operator<=(const object& a, const object& b);
object operator>=(const object& a, const object& b);
object operator>(const object& a, const object& b);
object operator!=(const object& a, const object& b);
object operator==(const object& a, const object& b);
object operator+(const object& a, const object& b);
object operator-(const object& a, const object& b);
object operator*(const object& a, const object& b);
object operator%(const object& a, const object& b);
object operator/(const object& a, const object& b);
object operator&&(const object& a, const object& b);
object operator||(const object& a, const object& b);
object operator-(const object& a);

class scope
{
  public:
					scope(const request& req);
	explicit		scope(const scope& next);

	template<typename T>
	void			put(const std::string& name, const T& value);

	template<typename T>
	void			put(const std::string& name, T* begin, T* end);

	const object&	lookup(const std::string& name) const;
	const object&	operator[](const std::string& name) const;

	object&			lookup(const std::string& name);
	object&			operator[](const std::string& name);

	const request&	get_request() const;

  private:

	friend std::ostream& operator<<(std::ostream& lhs, const scope& rhs);

	scope&			operator=(const scope&);

	std::map<std::string,object>
					m_data;
	scope*			m_next;
	const request*	m_req;
};

std::ostream& operator<<(std::ostream& lhs, const scope& rhs);

template<typename T>
inline
void scope::put(
	const std::string&	name,
	const T&		value)
{
	m_data[name] = object(value);
}

template<>
inline
void scope::put(
	const std::string&	name,
	const object&	value)
{
	m_data[name] = value;
}

template<typename T>
inline
void scope::put(
	const std::string&	name,
	T*				begin,
	T*				end)
{
	std::vector<object> elements;
	while (begin != end)
		elements.push_back(object(*begin++));
	m_data[name] = object(elements);
}

struct interpreter
{
					interpreter(
						const scope&			scope)
						: m_scope(scope) {}

	template<class OutputIterator, class Match>
	OutputIterator	operator()(Match& m, OutputIterator out, boost::regex_constants::match_flag_type);

	object			evaluate(
						const std::string&	s);

	void			process(
						std::string&			s);
	
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
	std::string		m_token_string;
	double			m_token_number;
	std::string::const_iterator
					m_ptr, m_end;
};

template<class OutputIterator, class Match>
inline
OutputIterator interpreter::operator()(Match& m, OutputIterator out, boost::regex_constants::match_flag_type)
{
	std::string s(m[1]);

	process(s);
	
	std::copy(s.begin(), s.end(), out);

	return out;
}

}
}
}

// enable foreach (.., array object)
namespace boost
{
    // specialize range_mutable_iterator and range_const_iterator in namespace boost
    template<>
    struct range_mutable_iterator<zeep::http::el::object>
    {
        typedef std::vector<zeep::http::el::object>::iterator type;
    };

    template<>
    struct range_const_iterator<zeep::http::el::object>
    {
        typedef std::vector<zeep::http::el::object>::const_iterator type;
    };
}
