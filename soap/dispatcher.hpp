#ifndef XML_SOAP_DISPATCHER_H

#if not defined(BOOST_PP_IS_ITERATING)

#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/noncopyable.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/accumulate.hpp>

#include "soap/xml/node.hpp"
#include "soap/exception.hpp"
#include "soap/xml/serialize.hpp"

namespace soap {

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
					d & boost::serialization::make_nvp(i->c_str(), t);
					return ++i;
				}
};

template<typename Function>
struct handler_traits;

// first specialization, no input arguments specified
template<class Class, typename R>
struct handler_traits<void(Class::*)(R&)>
{
	typedef void(Class::*Function)(R&);

	typedef typename f::vector<>	argument_type;
	typedef R						response_type;

	static void	invoke(Class* object, Function method, argument_type arguments, response_type& response)
				{
					(object->*method)(response);
				}
};

// all the other specializations are specified at the bottom of this file
#define  BOOST_PP_FILENAME_1 "soap/dispatcher.hpp"
#define  BOOST_PP_ITERATION_LIMITS (1, 9)
#include BOOST_PP_ITERATE()

struct handler_base
{
						handler_base(const std::string&	action)
							: m_action(action)
							, m_response(action + "Response") {}

	virtual node_ptr	call(node_ptr in) = 0;
	
	const std::string&	get_action_name() const						{ return m_action; }

	std::string			get_response_name() const					{ return m_response; }
	void				set_response_name(const std::string& name)	{ m_response = name; }
	
	std::string			m_action, m_response;	
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
						{
							copy(names, names + name_count, m_names.begin());
						}
	
	virtual node_ptr	call(node_ptr in)
						{
							// start by collecting all the parameters
							argument_type args;
							boost::fusion::accumulate(args, m_names.begin(),
								parameter_deserializer<std::string*>(in));
			
							// now call the actual server code
							response_type response;
							handler_traits<Function>::invoke(m_object, m_method, args, response);
							
							// and serialize the result back into XML
							node_ptr result(new node(get_response_name()));
							serializer sr(result, false);
							sr & boost::serialization::make_nvp(m_names[name_count - 1].c_str(), response);

							// that's all, we're done
							return result;
						}

	Class*				m_object;
	Function			m_method;
	boost::array<std::string,name_count>
						m_names;
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

	xml::node_ptr		dispatch(const std::string& action, xml::node_ptr in)
						{
//							if (in->ns() != m_ns)
//								throw exception("Invalid request, no match for namespace");
							
							handler_list::iterator cb = std::find_if(
								m_handlers.begin(), m_handlers.end(),
								boost::bind(&detail::handler_base::get_action_name, _1) == action);

							if (cb == m_handlers.end())
								throw exception("Action %s is not defined", action.c_str());

							xml::node_ptr result = cb->call(in);
							result->add_attribute("xmlns", m_ns);
							return result;
						}

	void				set_response_name(const std::string& action, const std::string& name)
						{
							handler_list::iterator cb = std::find_if(
								m_handlers.begin(), m_handlers.end(),
								boost::bind(&detail::handler_base::get_action_name, _1) == action);

							if (cb == m_handlers.end())
								throw exception("Action %s is not defined", action.c_str());
							
							cb->set_response_name(name);
						}

	std::string			m_ns;		
	handler_list		m_handlers;
};

}

#define XML_SOAP_DISPATCHER_H

#else // BOOST_PP_IS_ITERATING
//
//	Specializations for the handler_traits for a range of parameters
//
#define N BOOST_PP_ITERATION()

template<class Class, BOOST_PP_ENUM_PARAMS(N,typename T), typename R>
struct handler_traits<void(Class::*)(BOOST_PP_ENUM_PARAMS(N,T), R&)>
{
	typedef void(Class::*Function)(BOOST_PP_ENUM_PARAMS(N,T), R&);

#define M(z,j,data) typedef typename boost::remove_const<typename boost::remove_reference<T ## j>::type>::type	t_ ## j;
	BOOST_PP_REPEAT(N,M,~)
#undef M

	typedef typename f::vector<BOOST_PP_ENUM_PARAMS(N,t_)>	argument_type;
	typedef R												response_type;

#define M(z,j,data) f::at_c<j>(arguments)
	static void	invoke(Class* object, Function method, argument_type arguments, response_type& response)
				{
					(object->*method)(BOOST_PP_ENUM(N,M,~), response);
				}
#undef M
};

#endif
#endif
