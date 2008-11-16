#ifndef XML_SOAP_HANDLER_H
#define XML_SOAP_HANDLER_H

#include <string>
#include "xml/serialize.hpp"
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/include/invoke.hpp>
#include <boost/fusion/include/iterator_range.hpp>
#include <boost/fusion/include/accumulate.hpp>
#include <boost/fusion/container/vector.hpp>

namespace xml
{
namespace soap
{
	
template<typename Iterator>
struct parameter_deserializer
{
	typedef Iterator result_type;
	
	node_ptr	m_node;
	
	parameter_deserializer(
		node_ptr	node)
		: m_node(node) {}
	
	template<typename T>
	Iterator operator()(T& t, Iterator i) const
	{
		deserializer d(m_node);
		d(*i++, t);
		return i;
	}
};

struct handler_base
{
						handler_base(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response)
							: m_action_name(action)
							, m_request_name(request)
							, m_response_name(response) {}

	virtual				~handler_base() {}

	std::string			get_request_name() const			{ return m_request_name; }

	void				set_request_name(
							const std::string&	name)		{ m_request_name = name; }

	std::string			get_response_name() const			{ return m_response_name; }

	void				set_response_name(
							const std::string&	name)		{ m_response_name = name; }

	std::string			get_action_name() const				{ return m_action_name; }

	void				set_action_name(
							const std::string&	name)		{ m_action_name = name; }

	virtual node_ptr	call(
							node_ptr			in) = 0;

  protected:
	std::string		m_action_name, m_request_name, m_response_name;
};

template<class Derived, class Owner, typename F> struct call_handler;

template<class Derived, class Owner, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(R&)> : public handler_base
{
	enum { name_count = 1 };
	
	typedef R			res_type;
	typedef void (Owner::*CallFunc)(R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[1])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}
	
	virtual node_ptr	call(
							node_ptr			in)
						{
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}

	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;

	enum { name_count = 2 };
	
	typedef R			res_type;
	typedef void (Owner::*CallFunc)(T1, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[2])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}
	
	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;

	enum { name_count = 3 };
	
	typedef R			res_type;
	typedef void (Owner::*CallFunc)(T1, T2, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[3])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							t_2 a2; d(m_names[1], a2);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename T3, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, T3, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;
	
	enum { name_count = 4 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(T1, T2, T3, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[4])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							boost::fusion::vector<t_1,t_2,t_3> a;
							boost::fusion::accumulate(a, &m_names[0], parameter_deserializer<std::string*>(in));
//							std::string* n = m_names;
//							boost::fusion::for_each(a, boost::bind(&deserializer::des, d, _1, n));
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(boost::fusion::at_c<0>(a), boost::fusion::at_c<1>(a), boost::fusion::at_c<2>(a), response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, T3, T4, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;
	typedef typename boost::remove_const<typename boost::remove_reference<T4>::type>::type	t_4;

	enum { name_count = 5 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(T1, T2, T3, T4, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[5])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							t_2 a2; d(m_names[1], a2);
							t_3 a3; d(m_names[2], a3);
							t_4 a4; d(m_names[3], a4);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, T3, T4, 	T5, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;
	typedef typename boost::remove_const<typename boost::remove_reference<T4>::type>::type	t_4;
	typedef typename boost::remove_const<typename boost::remove_reference<T5>::type>::type	t_5;

	enum { name_count = 6 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(T1, T2, T3, T4, T5, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[6])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							t_2 a2; d(m_names[1], a2);
							t_3 a3; d(m_names[2], a3);
							t_4 a4; d(m_names[3], a4);
							t_5 a5; d(m_names[4], a5);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, T3, T4, 	T5, T6, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;
	typedef typename boost::remove_const<typename boost::remove_reference<T4>::type>::type	t_4;
	typedef typename boost::remove_const<typename boost::remove_reference<T5>::type>::type	t_5;
	typedef typename boost::remove_const<typename boost::remove_reference<T6>::type>::type	t_6;

	enum { name_count = 7 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(T1, T2, T3, T4, T5, 			T6, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[7])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							t_2 a2; d(m_names[1], a2);
							t_3 a3; d(m_names[2], a3);
							t_4 a4; d(m_names[3], a4);
							t_5 a5; d(m_names[4], a5);
							t_6 a6; d(m_names[5], a6);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename T7, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, T3, T4, 	T5, T6, T7, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;
	typedef typename boost::remove_const<typename boost::remove_reference<T4>::type>::type	t_4;
	typedef typename boost::remove_const<typename boost::remove_reference<T5>::type>::type	t_5;
	typedef typename boost::remove_const<typename boost::remove_reference<T6>::type>::type	t_6;
	typedef typename boost::remove_const<typename boost::remove_reference<T7>::type>::type	t_7;

	enum { name_count = 8 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(T1, T2, T3, T4, T5, 			T6, T7, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[8])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							t_2 a2; d(m_names[1], a2);
							t_3 a3; d(m_names[2], a3);
							t_4 a4; d(m_names[3], a4);
							t_5 a5; d(m_names[4], a5);
							t_6 a6; d(m_names[5], a6);
							t_7 a7; d(m_names[6], a7);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, a7, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename T7, typename T8, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, T3, T4, 	T5, T6, T7, T8, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;
	typedef typename boost::remove_const<typename boost::remove_reference<T4>::type>::type	t_4;
	typedef typename boost::remove_const<typename boost::remove_reference<T5>::type>::type	t_5;
	typedef typename boost::remove_const<typename boost::remove_reference<T6>::type>::type	t_6;
	typedef typename boost::remove_const<typename boost::remove_reference<T7>::type>::type	t_7;
	typedef typename boost::remove_const<typename boost::remove_reference<T8>::type>::type	t_8;

	enum { name_count = 9 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(T1, T2, T3, T4, T5, 			T6, T7, T8, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[9])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							t_2 a2; d(m_names[1], a2);
							t_3 a3; d(m_names[2], a3);
							t_4 a4; d(m_names[3], a4);
							t_5 a5; d(m_names[4], a5);
							t_6 a6; d(m_names[5], a6);
							t_7 a7; d(m_names[6], a7);
							t_8 a8; d(m_names[7], a8);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, a7, a8, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, R&)> : public handler_base
{
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;
	typedef typename boost::remove_const<typename boost::remove_reference<T4>::type>::type	t_4;
	typedef typename boost::remove_const<typename boost::remove_reference<T5>::type>::type	t_5;
	typedef typename boost::remove_const<typename boost::remove_reference<T6>::type>::type	t_6;
	typedef typename boost::remove_const<typename boost::remove_reference<T7>::type>::type	t_7;
	typedef typename boost::remove_const<typename boost::remove_reference<T8>::type>::type	t_8;
	typedef typename boost::remove_const<typename boost::remove_reference<T9>::type>::type	t_9;

	enum { name_count = 10 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(T1, T2, T3, T4, T5, T6, T7, T8, T9, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*			names[10])
							: handler_base(action, request, response)
							{
								copy(names, names + name_count, m_names);
							}

	virtual node_ptr	call(
							node_ptr			in)
						{
							deserializer d(in);
							
							t_1 a1; d(m_names[0], a1);
							t_2 a2; d(m_names[1], a2);
							t_3 a3; d(m_names[2], a3);
							t_4 a4; d(m_names[3], a4);
							t_5 a5; d(m_names[4], a5);
							t_6 a6; d(m_names[5], a6);
							t_7 a7; d(m_names[6], a7);
							t_8 a8; d(m_names[7], a8);
							t_9 a9; d(m_names[8], a9);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, a7, a8, a9, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}
	
	std::string			m_names[name_count];
};

template<class C, typename F>
struct call : public call_handler<call<C,F>, C, F>
{
  public:
	typedef call_handler<call<C,F>, C, F>		base_type;
	typedef typename base_type::res_type		res_type;
	typedef typename base_type::CallFunc		call_func_type;
	typedef C									owner_type;
	
	enum { name_count = base_type::name_count };

					call(
						const std::string&		action,
						owner_type*				owner,
						call_func_type			call)
						: base_type(action, action, action + "Response")
						, owner_(owner)
						, call_(call) {}

					call(
						const std::string&		action,
						owner_type*				owner,
						call_func_type			call,
						const char*				names[base_type::name_count])
						: base_type(action, action, action + "Response", names)
						, owner_(owner)
						, call_(call) {}
	
  	owner_type*			owner_;
	call_func_type		call_;
};

} // soap
} // xml

#endif
