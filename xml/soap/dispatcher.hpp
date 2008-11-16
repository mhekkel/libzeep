#ifndef XML_SOAP_DISPATCHER_H
#define XML_SOAP_DISPATCHER_H

#include <boost/static_assert.hpp>
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "xml/soap/handler.hpp"

#include <typeinfo>

namespace xml
{
namespace soap
{

class dispatcher : public boost::noncopyable
{
  public:
					dispatcher(
						const std::string&	ns)
						: m_ns(ns) {}

	node_ptr		dispatch(
						const std::string&	action,
						node_ptr			in)
					{
						boost::ptr_vector<handler_base>::iterator cb;
						
						cb = find_if(m_actions.begin(), m_actions.end(),
							boost::bind(&handler_base::get_action_name, _1) == action);
					
						if (cb == m_actions.end())
							throw xml::exception("Action not specified");
						
						node_ptr result = cb->call(in);
						result->add_attribute("xmlns", m_ns);
						
						return result;
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 1);
						
						const char* names[1] = { arg1 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 2);
						
						const char* names[2] = { arg1, arg2 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 3);
						
						const char* names[3] = { arg1, arg2, arg3 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3,
						const char*		arg4)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 4);
						
						const char* names[4] = { arg1, arg2, arg3, arg4 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3,
						const char*		arg4,
						const char*		arg5)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 5);
						
						const char* names[5] = { arg1, arg2, arg3, arg4, arg5 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3,
						const char*		arg4,
						const char*		arg5,
						const char*		arg6)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 6);
						
						const char* names[6] = { arg1, arg2, arg3, arg4, arg5, arg6 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3,
						const char*		arg4,
						const char*		arg5,
						const char*		arg6,
						const char*		arg7)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 7);
						
						const char* names[7] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
						
						m_actions.push_back(
							new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3,
						const char*		arg4,
						const char*		arg5,
						const char*		arg6,
						const char*		arg7,
						const char*		arg8)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 8);
						
						const char* names[8] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3,
						const char*		arg4,
						const char*		arg5,
						const char*		arg6,
						const char*		arg7,
						const char*		arg8,
						const char*		arg9)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 9);
						
						const char* names[9] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	template<class C, typename F>
	void			register_soap_call(
						const char*		action,
						C*				server,
						F				call_,
						const char*		arg1,
						const char*		arg2,
						const char*		arg3,
						const char*		arg4,
						const char*		arg5,
						const char*		arg6,
						const char*		arg7,
						const char*		arg8,
						const char*		arg9,
						const char*		arg10)
					{
						enum { required_count = call<C,F>::name_count };
						BOOST_STATIC_ASSERT(required_count == 10);
						
						const char* names[9] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 };
						
						m_actions.push_back(new call<C,F>(action, server, call_, names));
					}

	void			set_response_name(
						const std::string&	action,
						const std::string&	response_name)
					{
						boost::ptr_vector<handler_base>::iterator cb;
						
						cb = find_if(m_actions.begin(), m_actions.end(),
							boost::bind(&handler_base::get_action_name, _1) == action);
					
						if (cb == m_actions.end())
							throw xml::exception("Action not specified");
						
						cb->set_response_name(response_name);
					}

  private:
	std::string		m_ns;	// SOAP namespace
	boost::ptr_vector<handler_base>
					m_actions;
};

}
}

#endif
