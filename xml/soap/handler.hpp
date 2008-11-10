#ifndef XML_SOAP_HANDLER_H
#define XML_SOAP_HANDLER_H

#include <string>
#include "xml/serialize.hpp"

namespace xml
{
namespace soap
{

struct named_handler_base
{
					named_handler_base(
						const char*		action,
						const char*		request,
						const char*		response)
						: action_name(action)
						, request_name(request)
						, response_name(response) {}

	virtual			~named_handler_base() {}

	std::string		get_request_name() const			{ return request_name; }
	std::string		get_response_name() const			{ return response_name; }
	std::string		get_action_name() const				{ return action_name; }

	virtual void	call(
						xml::node_ptr	in,
						xml::node_ptr	out) = 0;

  protected:
	std::string		action_name, request_name, response_name;
};

template<typename F> struct handler_base;

template<typename R, typename T1>
struct handler_base<void(const T1&, R&)> : public named_handler_base
{
						handler_base(
							const char*		action,
							const char*		request,
							const char*		response,
							const char*		names[1])
							: named_handler_base(action, request, response)
							, d1(names[0]) {}
	
	virtual void		do_call(
							const T1&		a1,
							R&				r) = 0;
	
	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;
							d1(in, a1);
							
							R response;
							
							do_call(a1, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
};

template<typename R, typename T1, typename T2>
struct handler_base<void(const T1&, const T2&, R&)> : public named_handler_base
{
						handler_base(
							const char*		action,
							const char*		request,
							const char*		response,
							const char*		names[2])
							: named_handler_base(action, request, response)
							, d1(names[0])
							, d2(names[1]) {}
	
	virtual void		do_call(
							const T1&		a1,
							const T2&		a2,
							R&				r) = 0;
	
	virtual void		call(
							xml::node_ptr	in,
							xml::node_ptr	out)
						{
							T1 a1;
							d1(in, a1);
							
							T2 a2;
							d2(in, a2);
							
							R response;
							
							do_call(a1, a2, response);
							
							serializer<R> r("response");
							r(response, out);
						}
	
	deserializer<T1>	d1;
	deserializer<T2>	d2;
};

template<class Derived, class Owner, typename F> struct call_handler;

template<class Derived, class Owner, typename T1, typename R>
struct call_handler<Derived,Owner,void(const T1&, R&)> : public handler_base<void(const T1&, R&)>
{
	enum { name_count = 1 };
	
	typedef handler_base<void(const T1&, R&)> base_type;
	typedef void (Owner::*CallFunc)(const T1&, R&);

					call_handler(
						const char*		action,
						const char*		request,
						const char*		response,
						const char*		names[1])
						: base_type(action, request, response, names) {}

	virtual void	do_call(
						const T1&		a1,
						R&				r)
					{
						Derived* self = static_cast<Derived*>(this);
						Owner* owner = self->owner_;
						CallFunc func = self->call_;

						(owner->*func)(a1, r);
					}
};

template<class Derived, class Owner, typename T1, typename T2, typename R>
struct call_handler<Derived,Owner,void(const T1&, const T2&, R&)> : public handler_base<void(const T1&, const T2&, R&)>
{
	enum { name_count = 2 };
	
	typedef handler_base<void(const T1&, const T2&, R&)> base_type;
	typedef void (Owner::*CallFunc)(const T1&, const T2&, R&);

					call_handler(
						const char*		action,
						const char*		request,
						const char*		response,
						const char*		names[2])
						: base_type(action, request, response, names) {}

	virtual void	do_call(
						const T1&		a1,
						const T2&		a2,
						R&				r)
					{
						Derived* self = static_cast<Derived*>(this);
						Owner* owner = self->owner_;
						CallFunc func = self->call_;

						(owner->*func)(a1, a2, r);
					}
};

template<class C, typename F>
struct call : public call_handler<call<C,F>, C, F>
{
  public:
	typedef call_handler<call<C,F>, C, F>		base_type;
	typedef typename base_type::CallFunc		call_func_type;
	typedef C									owner_type;
	
	enum { name_count = base_type::name_count };

					call(
						const char*			action,
						const char*			req_name,
						const char*			res_name,
						owner_type*			owner,
						call_func_type		call,
						const char*			names[base_type::name_count])
						: base_type(action, req_name, res_name, names)
						, owner_(owner)
						, call_(call) {}
	
  	owner_type*			owner_;
	call_func_type		call_;
};

} // soap
} // xml

#endif
