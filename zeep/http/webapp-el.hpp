// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// expression language support
//

#pragma once

//#include <boost/operators.hpp>

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

class object;

namespace detail {

class object_impl;
class object_iterator_impl;

class object_impl
{
  public:

	void			reference()						{ ++m_refcount; }
	void			release()						{ if (--m_refcount == 0) delete this; }

	virtual object_iterator_impl*
					create_iterator(bool begin)		{ return nil; }

	virtual bool	empty() const = 0;
	virtual void	print(std::ostream& os) const = 0;
	virtual int		compare(object_impl* rhs) const = 0;

	virtual bool	is_array() const				{ return false; }
	virtual uint32	count() const					{ return 0; }

	virtual int64	to_int() const					{ throw zeep::exception("cannot convert to requested type"); }
	virtual double	to_double() const				{ throw zeep::exception("cannot convert to requested type"); }
	virtual std::string
					to_str() const					{ throw zeep::exception("cannot convert to requested type"); }

  protected:
					object_impl() : m_refcount(1)	{}

	virtual			~object_impl()					{}

  private:
	int				m_refcount;
};

class object_iterator_impl
{
  public:

	void			reference()						{ ++m_refcount; }
	void			release()						{ if (--m_refcount == 0) delete this; }

					object_iterator_impl()
						: m_refcount(1)				{}
	
	virtual void	increment() = 0;
	virtual object&	dereference() = 0;
	virtual bool	equal(const object_iterator_impl* other) = 0;

  protected:
	virtual			~object_iterator_impl()			{}

  private:
	int				m_refcount;
};
	
}

typedef uint32	unicode;

class object
{
  public:
				object();
				object(const object& o);
				
				// create an array object
	explicit	object(const std::vector<object>& v);

				// construct an object directly from some basic types
	explicit	object(int8 v);
	explicit	object(uint8 v);
	explicit	object(int16 v);
	explicit	object(uint16 v);
	explicit	object(int32 v);
	explicit	object(uint32 v);
	explicit	object(int64 v);
//	explicit	object(uint64 v);
	explicit	object(float v);
	explicit	object(double v);
	explicit	object(const char* v);
	explicit	object(const std::string& v);

				~object();

	object&		operator=(const object& o);

				// assign an array object
	object&		operator=(const std::vector<object>& v);

	// and assign some basic types
	object&		operator=(int8 v);
	object&		operator=(uint8 v);
	object&		operator=(int16 v);
	object&		operator=(uint16 v);
	object&		operator=(int32 v);
	object&		operator=(uint32 v);
	object&		operator=(int64 v);
//	object&		operator=(uint64 v);
	object&		operator=(float v);
	object&		operator=(double v);
	object&		operator=(const char* v);
	object&		operator=(const std::string& v);

	bool		undefined() const				{ return m_impl == nil; }
	bool		empty() const					{ return m_impl == nil or m_impl->empty(); }
	bool		is_array() const				{ return m_impl != nil and m_impl->is_array(); }
	uint32		count() const					{ return m_impl->count(); }

	template<typename T>
	T			as() const;

	const object operator[](const std::string& name) const;
	const object operator[](const char* name) const;
	const object operator[](uint32 ix) const;

	object&		operator[](const std::string& name);
	object&		operator[](const char* name);
	object&		operator[](uint32 ix);

	bool		operator<(const object& rhs) const;
	bool		operator==(const object& rhs) const;

	template<class ObjectType>
	class basic_iterator : public std::iterator<std::forward_iterator_tag, ObjectType>
	{
	  public:
		typedef typename std::iterator<std::forward_iterator_tag, ObjectType>	base_type;
		typedef typename base_type::reference									reference;
		typedef typename base_type::pointer										pointer;

						basic_iterator();
						basic_iterator(const basic_iterator& other);
						basic_iterator(detail::object_impl* a);
						basic_iterator(detail::object_impl* a, int);
						basic_iterator(const detail::object_impl* a);
						basic_iterator(const detail::object_impl* a, int);
						~basic_iterator();
		
		basic_iterator&	operator=(const basic_iterator& other);
		
		const reference	operator*() const;
		reference		operator*();
		pointer			operator->() const;

		basic_iterator&	operator++();
		basic_iterator	operator++(int);

		bool			operator==(const basic_iterator& other) const;
		bool			operator!=(const basic_iterator& other) const;

	  private:
		detail::object_iterator_impl*
						m_impl;
	};

	typedef basic_iterator<object>			iterator;
	typedef basic_iterator<const object>	const_iterator;

	iterator			begin()											{ return iterator(m_impl); }
	iterator			end()											{ return iterator(m_impl, -1); }

	const_iterator		begin() const									{ return const_iterator(m_impl); }
	const_iterator		end() const										{ return const_iterator(m_impl, -1); }

	friend iterator			range_begin(object& x)						{ return x.begin(); }
	friend iterator			range_end(object& x)						{ return x.end(); }
	friend const_iterator	range_begin(const object& x)				{ return x.begin(); }
	friend const_iterator	range_end(const object& x)					{ return x.end(); }

	friend std::ostream& operator<<(std::ostream& lhs, const object& rhs);

  private:

	friend object operator+(const object& a, const object& b);
	friend object operator-(const object& a, const object& b);
	friend object operator*(const object& a, const object& b);
	friend object operator%(const object& a, const object& b);
	friend object operator/(const object& a, const object& b);
	friend object operator-(const object& a);

	struct detail::object_impl*	m_impl;
};

bool process_el(const scope& scope, std::string& text);
void evaluate_el(const scope& scope, const std::string& text, object& result);
bool evaluate_el(const scope& scope, const std::string& text);

// --------------------------------------------------------------------

inline object::object()
	: m_impl(nil)
{
}

inline object::object(const object& o)
	: m_impl(o.m_impl)
{
	if (m_impl != nil)
		m_impl->reference();
}

inline object::~object()
{
	if (m_impl != nil)
		m_impl->release();
}

inline object& object::operator=(const object& o)
{
	if (this != &o)
	{
		if (m_impl != nil)
			m_impl->release();
		m_impl = o.m_impl;
		if (m_impl != nil)
			m_impl->reference();
	}
	return *this;
}

// --------------------------------------------------------------------

template<class ObjectType>
object::basic_iterator<ObjectType>::basic_iterator()
	: m_impl(nil)
{
}

template<class ObjectType>
object::basic_iterator<ObjectType>::basic_iterator(const basic_iterator& o)
	: m_impl(o.m_impl)
{
	if (m_impl != nil)
		m_impl->reference();
}
		
template<class ObjectType>
object::basic_iterator<ObjectType>::basic_iterator(detail::object_impl* a)
	: m_impl(a ? a->create_iterator(true) : nil)
{
}
		
template<class ObjectType>
object::basic_iterator<ObjectType>::basic_iterator(detail::object_impl* a, int)
	: m_impl(a ? a->create_iterator(false) : nil)
{
}
		
template<class ObjectType>
object::basic_iterator<ObjectType>::basic_iterator(const detail::object_impl* a)
	: m_impl(a ? a->create_iterator(true) : nil)
{
}
		
template<class ObjectType>
object::basic_iterator<ObjectType>::basic_iterator(const detail::object_impl* a, int)
	: m_impl(a ? a->create_iterator(false) : nil)
{
}
		
template<class ObjectType>
object::basic_iterator<ObjectType>::~basic_iterator()
{
	if (m_impl != nil)
		m_impl->release();
}
		
template<class ObjectType>
object::basic_iterator<ObjectType>& object::basic_iterator<ObjectType>::operator=(const basic_iterator& o)
{
	if (this != &o)
	{
		if (m_impl != nil)
			m_impl->release();
		m_impl = o.m_impl;
		if (m_impl != nil)
			m_impl->reference();
	}
	return *this;
}
		
template<class ObjectType>
const typename object::basic_iterator<ObjectType>::reference object::basic_iterator<ObjectType>::operator*() const
{
	if (m_impl == nil)
		throw exception("dereferencing invalid object iterator");
	return m_impl->dereference();
}

template<class ObjectType>
typename object::basic_iterator<ObjectType>::reference object::basic_iterator<ObjectType>::operator*()
{
	if (m_impl == nil)
		throw exception("dereferencing invalid object iterator");
	return m_impl->dereference();
}

template<class ObjectType>
typename object::basic_iterator<ObjectType>::pointer object::basic_iterator<ObjectType>::operator->() const
{
	if (m_impl == nil)
		throw exception("dereferencing invalid object iterator");
	return &m_impl->dereference();
}

template<class ObjectType>
object::basic_iterator<ObjectType>& object::basic_iterator<ObjectType>::operator++()
{
	if (m_impl == nil)
		throw exception("incrementing invalid object iterator");
	m_impl->increment();
	return *this;
}

template<class ObjectType>
object::basic_iterator<ObjectType> object::basic_iterator<ObjectType>::operator++(int)
{
	if (m_impl == nil)
		throw exception("incrementing invalid object iterator");

	basic_iterator<ObjectType> iter(*this);
	m_impl->increment();
	return iter;
}

template<class ObjectType>
bool object::basic_iterator<ObjectType>::operator==(const basic_iterator& o) const
{
	bool result;
	if (m_impl == nil and o.m_impl == nil)
		result = true;
	else if (m_impl == nil or o.m_impl == nil)
		throw exception("invalid object iterators");
	else
		result = m_impl->equal(o.m_impl);
	return result;
}

template<class ObjectType>
bool object::basic_iterator<ObjectType>::operator!=(const basic_iterator& o) const
{
	bool result;
	if (m_impl == nil and o.m_impl == nil)
		result = false;
	else if (m_impl == nil or o.m_impl == nil)
		throw exception("invalid object iterators");
	else
		result = not m_impl->equal(o.m_impl);
	return result;
}

// --------------------------------------------------------------------

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
        typedef zeep::http::el::object::iterator type;
    };

    template<>
    struct range_const_iterator<zeep::http::el::object>
    {
        typedef zeep::http::el::object::const_iterator type;
    };
}
