#ifndef XML_SOAP_DISPATCHER_H
#define XML_SOAP_DISPATCHER_H

#include <boost/static_assert.hpp>
#include "xml/soap/handler.hpp"

#include <typeinfo>

namespace xml
{
namespace soap
{

template<class Derived>
class dispatcher : public boost::noncopyable
{
  public:
					dispatcher() {}

	void			dispatch(
						const char*		action,
						xml::node_ptr	in,
						xml::node_ptr	out);

	template<typename F>
	void			register_soap_call(
						const char*		action,
						typename call<Derived,F>::call_func_type				call_,
						const char*		arg1)
					{
						enum { required_count = call<Derived,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 1);
						
						const char* names[1] = { arg1 };
						
						actions.push_back(
							new call<Derived,F>(action, static_cast<Derived*>(this), call_, names));
					}

	template<typename F>
	void			register_soap_call(
						const char*		action,
						typename call<Derived,F>::call_func_type				call_,
						const char*		arg1,
						const char*		arg2)
					{
						enum { required_count = call<Derived,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 2);
						
						const char* names[2] = { arg1, arg2 };
						
						actions.push_back(
							new call<Derived,F>(action, static_cast<Derived*>(this), call_, names));
					}

  private:
	
	boost::ptr_vector<named_handler_base>
					actions;
};

template<class Derived>
void dispatcher<Derived>::dispatch(
	const char*		action,
	xml::node_ptr	in,
	xml::node_ptr	out)
{
	boost::ptr_vector<named_handler_base>::iterator cb = find_if(actions.begin(), actions.end(),
		boost::bind(&named_handler_base::get_action_name, _1) == action);

	if (cb == actions.end())
		throw xml::exception("Action not specified");
	
	cb->call(in, out);
}

}
}

#endif
