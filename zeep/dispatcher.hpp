//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_DISPATCHER_H

#if not defined(BOOST_PP_IS_ITERATING)

#include <boost/version.hpp>
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

#include "zeep/xml/node.hpp"
#include "zeep/exception.hpp"
#include "zeep/xml/serialize.hpp"

namespace zeep {

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
	
	// due to a change in fusion::accumulate we have to define both functors:
	template<typename T>
	Iterator	operator()(Iterator i, T& t) const
				{
					xml::deserializer d(m_node);
					d & boost::serialization::make_nvp(i->c_str(), t);
					return ++i;
				}

	template<typename T>
	Iterator	operator()(T& t, Iterator i) const
				{
					xml::deserializer d(m_node);
					d & boost::serialization::make_nvp(i->c_str(), t);
					return ++i;
				}
};

template<typename Iterator>
struct parameter_types
{
	typedef Iterator		result_type;
	
	type_map&	m_types;
	node_ptr	m_node;
	
				parameter_types(
					type_map&	types,
					node_ptr	node)
					: m_types(types)
					, m_node(node) {}
	
	template<typename T>
	Iterator	operator()(Iterator i, T& t) const
				{
					xml::wsdl_creator d(m_types, m_node);
					d & boost::serialization::make_nvp(i->c_str(), t);
					return ++i;
				}

	template<typename T>
	Iterator	operator()(T& t, Iterator i) const
				{
					xml::wsdl_creator d(m_types, m_node);
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

	static void	invoke(Class* object, Function method,
					argument_type arguments, response_type& response)
				{
					(object->*method)(response);
				}
};

// all the other specializations are specified at the bottom of this file
#define  BOOST_PP_FILENAME_1 "zeep/dispatcher.hpp"
#define  BOOST_PP_ITERATION_LIMITS (1, 9)
#include BOOST_PP_ITERATE()

// messages can be used by more than one action, so we need a way to avoid duplicates
typedef std::map<std::string,node_ptr>	message_map;

struct handler_base
{
						handler_base(const std::string&	action)
							: m_action(action)
							, m_response(action + "Response") {}

	virtual node_ptr	call(node_ptr in) = 0;

	virtual void		collect(type_map& types, message_map& messages,
							node_ptr portType, node_ptr binding) = 0;
	
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

	virtual void		collect(type_map& types, message_map& messages,
							node_ptr portType, node_ptr binding)
						{
							// the request type
							node_ptr requestType(new node("element", "xsd"));
							requestType->add_attribute("name", get_action_name());
							types[get_action_name()] = requestType;
							
							node_ptr complexType(new node("complexType", "xsd"));
							requestType->add_child(complexType);
							
							node_ptr sequence(new node("sequence", "xsd"));
							complexType->add_child(sequence);
							
							argument_type args;
							boost::fusion::accumulate(args, m_names.begin(),
								parameter_types<std::string*>(types, sequence));

							// and the response type
							node_ptr responseType(new node("element", "xsd"));
							responseType->add_attribute("name", get_response_name());
							types[get_response_name()] = responseType;

							complexType.reset(new node("complexType", "xsd"));
							responseType->add_child(complexType);
							
							sequence.reset(new node("sequence", "xsd"));
							complexType->add_child(sequence);
							
							wsdl_creator wc(types, sequence, false);

							response_type response;
							wc & boost::serialization::make_nvp(m_names[name_count - 1].c_str(), response);
							
							// now the wsdl operations
							node_ptr message =
								make_node("wsdl:message",
									make_attribute("name", get_action_name() + "RequestMessage"));
							message->add_child(
								make_node("wsdl:part",
										make_attribute("name", "parameters"),
										make_attribute("element", kPrefix + ':' + get_action_name()))
								);
							messages[message->get_attribute("name")] = message;

							message =
								make_node("wsdl:message",
									make_attribute("name", get_response_name() + "Message"));
							message->add_child(
								make_node("wsdl:part",
										make_attribute("name", "parameters"),
										make_attribute("element", kPrefix + ':' + get_response_name()))
								);
							messages[message->get_attribute("name")] = message;
							
							// port type
							
							node_ptr operation(new node("operation", "wsdl"));
							operation->add_attribute("name", get_action_name());
							
							node_ptr input(new node("input", "wsdl"));
							input->add_attribute("message", kPrefix + ':' + get_action_name() + "RequestMessage");
							operation->add_child(input);

							node_ptr output(new node("output", "wsdl"));
							output->add_attribute("message", kPrefix + ':' + get_response_name() + "Message");
							operation->add_child(output);
							
							portType->add_child(operation);
							
							// and the soap operations
							operation.reset(new node("operation", "wsdl"));
							operation->add_attribute("name", get_action_name());
							binding->add_child(operation);
							
							input.reset(new node("input", "wsdl"));
							operation->add_child(input);
							
							output.reset(new node("output", "wsdl"));
							operation->add_child(output);
							
							node_ptr body(new node("body", "soap"));
							body->add_attribute("use", "literal");
							input->add_child(body);
							output->add_child(body);
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

						dispatcher(const std::string& ns, const std::string& service)
							: m_ns(ns)
							, m_service(service) {}
		
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

	xml::node_ptr		make_wsdl(const std::string& address)
						{
							// start by making the root node: wsdl:definitions
							xml::node_ptr wsdl(new xml::node("definitions", "wsdl"));
							wsdl->add_attribute("targetNamespace", m_ns);
							wsdl->add_attribute("xmlns:wsdl", "http://schemas.xmlsoap.org/wsdl/");
							wsdl->add_attribute("xmlns:" + xml::kPrefix, m_ns);
							wsdl->add_attribute("xmlns:soap", "http://schemas.xmlsoap.org/wsdl/soap/");
							
							// add wsdl:types
							xml::node_ptr types(new xml::node("types", "wsdl"));
							wsdl->add_child(types);

							// add xsd:schema
							xml::node_ptr schema(new xml::node("schema", "xsd"));
							schema->add_attribute("targetNamespace", m_ns);
							schema->add_attribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
							schema->add_attribute("elementFormDefault", "qualified");
							schema->add_attribute("attributeFormDefault", "unqualified");
							types->add_child(schema);

							// add wsdl:binding
							xml::node_ptr binding(new xml::node("binding", "wsdl"));
							binding->add_attribute("name", m_service);
							binding->add_attribute("type", xml::kPrefix + ':' + m_service + "PortType");
							
							// add soap:binding
							xml::node_ptr soapBinding(new xml::node("binding", "soap"));
							soapBinding->add_attribute("style", "document");
							soapBinding->add_attribute("transport", "http://schemas.xmlsoap.org/soap/http");
							binding->add_child(soapBinding);
							
							// add wsdl:portType
							xml::node_ptr portType(new xml::node("portType", "wsdl"));
							portType->add_attribute("name", m_service + "PortType");
							
							// and the types
							xml::type_map typeMap;
							detail::message_map messageMap;
							
							for (handler_list::iterator cb = m_handlers.begin(); cb != m_handlers.end(); ++cb)
								cb->collect(typeMap, messageMap, portType, binding);
							
							for (detail::message_map::iterator m = messageMap.begin(); m != messageMap.end(); ++m)
								wsdl->add_child(m->second);
							
							for (xml::type_map::iterator t = typeMap.begin(); t != typeMap.end(); ++t)
								schema->add_child(t->second);
							
							wsdl->add_child(portType);
							wsdl->add_child(binding);
							
							// finish with the wsdl:service
							xml::node_ptr service(new xml::node("service", "wsdl"));
							service->add_attribute("name", m_service);
							wsdl->add_child(service);
							
							xml::node_ptr port(new xml::node("port", "wsdl"));
							port->add_attribute("name", m_service);
							port->add_attribute("binding", xml::kPrefix + ':' + m_service);
							service->add_child(port);
							
							xml::node_ptr soapAddress(new xml::node("address", "soap"));
							soapAddress->add_attribute("location", address);
							port->add_child(soapAddress);
							
							return wsdl;
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
	std::string			m_service;
	handler_list		m_handlers;
};

}

#define SOAP_DISPATCHER_H

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
