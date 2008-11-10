#ifndef XML_SOAP_HANDLER_H
#define XML_SOAP_HANDLER_H

#include <string>
#include "xml/serialize.hpp"

namespace xml
{
namespace soap
{

struct handler_base
{
					handler_base(
						const std::string&	action,
						const std::string&	request,
						const std::string&	response)
						: action_name(action)
						, request_name(request)
						, response_name(response) {}

	virtual			~handler_base() {}

	std::string		get_request_name() const			{ return request_name; }
	std::string		get_response_name() const			{ return response_name; }
	std::string		get_action_name() const				{ return action_name; }

	virtual void	call(
						xml::node_ptr	in,
						xml::node_ptr	out) = 0;

  protected:
	std::string		action_name, request_name, response_name;
};

template<class Derived, class Owner, typename F> struct call_handler;

template<class Derived, class Owner, typename T1, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, R&)> : public handler_base
{
	enum { name_count = 1 };
	
	typedef void (Owner::*CallFunc)(const T1&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[1])
							: handler_base(action, request, response)
							, d1(names[0]) {}
	
	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;
							d1(in, a1);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
};

template<class Derived, class Owner, typename T1, typename T2, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, R&)> : public handler_base
{
	enum { name_count = 2 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[2])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
};

template<class Derived, class Owner, typename T1, typename T2, typename T3, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, const T3&, R&)> : public handler_base
{
	enum { name_count = 3 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, const T3&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[3])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1])
							, d3(names[2]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							T3 a3;	d3(in, a3);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
	deserializer<T3>	d3;
};
template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, const T3&, const T4&, R&)> : public handler_base
{
	enum { name_count = 4 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, const T3&, const T4&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[4])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1])
							, d3(names[2])
							, d4(names[3]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							T3 a3;	d3(in, a3);
							T4 a4;	d4(in, a4);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
	deserializer<T3>	d3;
	deserializer<T4>	d4;
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, const T3&, const T4&,
	const T5&, R&)> : public handler_base
{
	enum { name_count = 5 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, const T3&, const T4&, const T5&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[5])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1])
							, d3(names[2])
							, d4(names[3])
							, d5(names[4]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							T3 a3;	d3(in, a3);
							T4 a4;	d4(in, a4);
							T5 a5;	d5(in, a5);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
	deserializer<T3>	d3;
	deserializer<T4>	d4;
	deserializer<T5>	d5;
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, const T3&, const T4&,
	const T5&, const T6&, R&)> : public handler_base
{
	enum { name_count = 6 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, const T3&, const T4&, const T5&,
			const T6&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[6])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1])
							, d3(names[2])
							, d4(names[3])
							, d5(names[4])
							, d6(names[5]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							T3 a3;	d3(in, a3);
							T4 a4;	d4(in, a4);
							T5 a5;	d5(in, a5);
							T6 a6;	d6(in, a6);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
	deserializer<T3>	d3;
	deserializer<T4>	d4;
	deserializer<T5>	d5;
	deserializer<T6>	d6;
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename T7, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, const T3&, const T4&,
	const T5&, const T6&, const T7&, R&)> : public handler_base
{
	enum { name_count = 7 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, const T3&, const T4&, const T5&,
			const T6&, const T7&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[7])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1])
							, d3(names[2])
							, d4(names[3])
							, d5(names[4])
							, d6(names[5])
							, d7(names[6]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							T3 a3;	d3(in, a3);
							T4 a4;	d4(in, a4);
							T5 a5;	d5(in, a5);
							T6 a6;	d6(in, a6);
							T7 a7;	d7(in, a7);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, a7, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
	deserializer<T3>	d3;
	deserializer<T4>	d4;
	deserializer<T5>	d5;
	deserializer<T6>	d6;
	deserializer<T7>	d7;
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename T7, typename T8, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, const T3&, const T4&,
	const T5&, const T6&, const T7&, const T8&, R&)> : public handler_base
{
	enum { name_count = 8 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, const T3&, const T4&, const T5&,
			const T6&, const T7&, const T8&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[8])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1])
							, d3(names[2])
							, d4(names[3])
							, d5(names[4])
							, d6(names[5])
							, d7(names[6])
							, d8(names[7]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							T3 a3;	d3(in, a3);
							T4 a4;	d4(in, a4);
							T5 a5;	d5(in, a5);
							T6 a6;	d6(in, a6);
							T7 a7;	d7(in, a7);
							T8 a8;	d8(in, a8);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, a7, a8, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
	deserializer<T3>	d3;
	deserializer<T4>	d4;
	deserializer<T5>	d5;
	deserializer<T6>	d6;
	deserializer<T7>	d7;
	deserializer<T8>	d8;
};

template<class Derived, class Owner, typename T1, typename T2, typename T3,
	typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename R>
struct call_handler<Derived,Owner,void(Owner::*)(const T1&, const T2&, const T3&, const T4&,
	const T5&, const T6&, const T7&, const T8&, const T9&, R&)> : public handler_base
{
	enum { name_count = 9 };
	
	typedef R												res_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, const T3&, const T4&, const T5&,
			const T6&, const T7&, const T8&, const T9&, R&);

						call_handler(
							const std::string&	action,
							const std::string&	request,
							const std::string&	response,
							const char*		names[9])
							: handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1])
							, d3(names[2])
							, d4(names[3])
							, d5(names[4])
							, d6(names[5])
							, d7(names[6])
							, d8(names[7])
							, d9(names[8]) {}

	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;	d1(in, a1);
							T2 a2;	d2(in, a2);
							T3 a3;	d3(in, a3);
							T4 a4;	d4(in, a4);
							T5 a5;	d5(in, a5);
							T6 a6;	d6(in, a6);
							T7 a7;	d7(in, a7);
							T8 a8;	d8(in, a8);
							T9 a9;	d9(in, a9);
							
							Derived* self = static_cast<Derived*>(this);
							Owner* owner = self->owner_;
							CallFunc func = self->call_;
	
							R response;
							
							(owner->*func)(a1, a2, a3, a4, a5, a6, a7, a8, a9, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
	deserializer<T3>	d3;
	deserializer<T4>	d4;
	deserializer<T5>	d5;
	deserializer<T6>	d6;
	deserializer<T7>	d7;
	deserializer<T8>	d8;
	deserializer<T9>	d9;
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
						call_func_type			call,
						const char*				names[base_type::name_count])
						: base_type(action, action, typeid(res_type).name(), names)
						, owner_(owner)
						, call_(call) {}
	
  	owner_type*			owner_;
	call_func_type		call_;
};

} // soap
} // xml

#endif
