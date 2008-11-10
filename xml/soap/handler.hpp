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
struct call_handler<Derived,Owner,void(const T1&, R&)> : public handler_base
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
struct call_handler<Derived,Owner,void(const T1&, const T2&, R&)> : public handler_base
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
							T1 a1;
							d1(in, a1);
							
							T2 a2;
							d2(in, a2);
							
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
