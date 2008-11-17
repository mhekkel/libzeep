#ifndef XML_SOAP_DISPATCHER_H
#define XML_SOAP_DISPATCHER_H

#include <boost/serialization/nvp.hpp>
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/accumulate.hpp>

#include "xml/node.hpp"
#include "xml/exception.hpp"
#include "xml/serialize.hpp"

namespace xml { namespace soap {

namespace detail {

using namespace xml;
namespace f = boost::fusion;

template<typename Iterator>
struct parameter_deserializer
{
	typedef Iterator		result_type;
	
	node_ptr	m_node;
	
				parameter_deserializer(
					node_ptr	node)
					: m_node(node) {}
	
	template<typename T>
	Iterator	operator()(T& t, Iterator i) const
				{
					xml::deserializer d(m_node);
					d(*i++, t);
					return i;
				}
};

template<typename Function>
struct handler_traits;

template<class Class, typename R>
struct handler_traits<void(Class::*)(R&)>
{
	typedef void(Class::*Function)(R&);

	typedef typename f::vector<>															argument_type;
	typedef R																				response_type;

	static void	invoke(Class* object, Function method, argument_type arguments, response_type& response)
				{
					(object->*method)(response);
				}
};

template<class Class, typename T1, typename R>
struct handler_traits<void(Class::*)(T1, R&)>
{
	typedef void(Class::*Function)(T1, R&);
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;

	typedef typename f::vector<t_1>															argument_type;
	typedef R																				response_type;

	static void	invoke(Class* object, Function method, argument_type arguments, response_type& response)
				{
					(object->*method)(f::at_c<0>(arguments), response);
				}
};

template<class Class, typename T1, typename T2, typename R>
struct handler_traits<void(Class::*)(T1, T2, R&)>
{
	typedef void(Class::*Function)(T1, T2, R&);
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;

	typedef typename f::vector<t_1,t_2>														argument_type;
	typedef R																				response_type;

	static void	invoke(Class* object, Function method, argument_type arguments, response_type& response)
				{
					(object->*method)(f::at_c<0>(arguments), f::at_c<1>(arguments), response);
				}
};

template<class Class, typename T1, typename T2, typename T3, typename R>
struct handler_traits<void(Class::*)(T1, T2, T3, R&)>
{
	typedef void(Class::*Function)(T1, T2, T3, R&);
	typedef typename boost::remove_const<typename boost::remove_reference<T1>::type>::type	t_1;
	typedef typename boost::remove_const<typename boost::remove_reference<T2>::type>::type	t_2;
	typedef typename boost::remove_const<typename boost::remove_reference<T3>::type>::type	t_3;

	typedef typename f::vector<t_1,t_2,t_3>													argument_type;
	typedef R																				response_type;

	static void	invoke(Class* object, Function method, argument_type arguments, response_type& response)
				{
					(object->*method)(f::at_c<0>(arguments), f::at_c<1>(arguments), f::at_c<2>(arguments), response);
				}
};

struct handler_base
{
			handler_base(const std::string&	action)
				: m_action(action) {}

	virtual node_ptr	call(node_ptr in) = 0;
	
	const std::string&	get_action_name() const			{ return m_action; }

	std::string			get_response_name() const		{ return m_action + "Response"; }
	
	std::string	m_action;	
};

template
<
	class Class,
	typename Function
>
struct handler : public handler_base
{
	typedef typename handler_traits<Function>::argument_type	argument_type;
	typedef typename handler_traits<Function>::response_type	response_type;
	enum { name_count = argument_type::size::value + 1 };
	typedef const char*											names_type[name_count];
	
						handler(const char* action, Class* object, Function method, names_type& names)
							: handler_base(action)
							, m_object(object)
							, m_method(method)
							, m_names(names) {}
	
	virtual node_ptr	call(node_ptr in)
						{
							argument_type args;
							boost::fusion::accumulate(args, &m_names[0], parameter_deserializer<const char**>(in));
			
							response_type response;
							handler_traits<Function>::invoke(m_object, m_method, args, response);
							
							node_ptr result(new node(get_response_name()));
							serializer::serialize(result, m_names[name_count - 1], response);
							return result;
						}

	Class*				m_object;
	Function			m_method;
	names_type&			m_names;
};
	
}

class dispatcher
{
  public:
	typedef boost::ptr_vector<detail::handler_base>	handler_list;


						dispatcher(const std::string& ns)
							: m_ns(ns) {}
		
	template<
		class Class,
		typename Function
	>
	void				register_action(const char* action, Class* server, Function call,
							typename detail::handler<Class,Function>::names_type& arg)
						{
							m_handlers.push_back(new detail::handler<Class,Function>(action, server, call, arg));
						}

	xml::node_ptr		dispatch(const char* action, xml::node_ptr in)
						{
							handler_list::iterator cb = std::find_if(
								m_handlers.begin(), m_handlers.end(),
								boost::bind(&detail::handler_base::get_action_name, _1) == action);
							if (cb == m_handlers.end())
								throw xml::exception("Action %s is not defined", action);
							return cb->call(in);
						}

	std::string			m_ns;		
	handler_list		m_handlers;
};

}
}

#endif
