// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

/// Dispatcher is the class that implements the code to dispatch calls
/// based on an incoming SOAP message. This code uses boost::fusion for
/// most of its tricks.
/// 
/// The way dispatches works is as follows. The class dispatches has a
/// member called m_handlers which is a list of available methods bound
/// to a SOAP action. You can add functions to this list by calling
/// register_action. This function takes four arguments:
/// - The name of the action as it is included in the WSDL.
/// - A pointer to the owning object of the function.
/// - The actual function/method.
/// - A list of argument names. The number of names in this list
///   is specified by the signature of the function and an error
///   will be reported if these are not equal.
/// 
/// The dispatcher will create a wrapping handler object for each of
/// the registered actions. Each handler object has two main methods.
/// One to do the actual call to the method as registered. This one
/// uses the zeep::xml::serializer code to collect the values out of
/// the SOAP message and passes these using fusion calls to the 
/// registered method.
/// The second method of the handler class is collect which is used
/// to collect all the information required to create a complete WSDL.

#ifndef SOAP_DISPATCHER_H

#if LIBZEEP_DOXYGEN_INVOKED || ! defined(BOOST_PP_IS_ITERATING)

#include <boost/version.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/accumulate.hpp>

#include <zeep/xml/node.hpp>
#include <zeep/exception.hpp>
#include <zeep/xml/serialize.hpp>

namespace zeep {

namespace detail {

using namespace xml;
namespace f = boost::fusion;

template<typename Iterator>
struct parameter_deserializer
{
	typedef Iterator		result_type;
	
	element*	m_node;
	
				parameter_deserializer(element* node)
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
	element*	m_node;
	
				parameter_types(type_map& types, element* node)
					: m_types(types), m_node(node) {}
	
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

#ifndef LIBZEEP_DOXYGEN_INVOKED
// all the other specializations are specified at the bottom of this file
#define  BOOST_PP_FILENAME_1 <zeep/dispatcher.hpp>
#define  BOOST_PP_ITERATION_LIMITS (1, 9)
#include BOOST_PP_ITERATE()
#endif

// messages can be used by more than one action, so we need a way to avoid duplicates
typedef std::map<std::string,element*>	message_map;

struct handler_base
{
						handler_base(const std::string&	action)
							: m_action(action)
							, m_response(action + "Response") {}
	virtual					~handler_base() {}

	virtual element*	call(element* in) = 0;

	virtual void		collect(type_map& types, message_map& messages,
							element* portType, element* binding) = 0;
	
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
	
	virtual element*	call(element* in)
						{
							// start by collecting all the parameters
							argument_type args;
							boost::fusion::accumulate(args, m_names.begin(),
								parameter_deserializer<std::string*>(in));
			
							// now call the actual server code
							response_type response;
							handler_traits<Function>::invoke(m_object, m_method, args, response);
							
							// and serialize the result back into XML
							element* result(new element(get_response_name()));
							serializer sr(result);
							sr.serialize(m_names[name_count - 1].c_str(), response);

							// that's all, we're done
							return result;
						}

	virtual void		collect(type_map& types, message_map& messages,
							element* portType, element* binding)
						{
							// the request type
							element* requestType(new element("xsd:element"));
							requestType->set_attribute("name", get_action_name());
							types[get_action_name()] = requestType;
							
							element* complexType(new element("xsd:complexType"));
							requestType->append(complexType);
							
							element* sequence(new element("xsd:sequence"));
							complexType->append(sequence);
							
							argument_type args;
							boost::fusion::accumulate(args, m_names.begin(),
								parameter_types<std::string*>(types, sequence));

							// and the response type
							element* responseType(new element("xsd:element"));
							responseType->set_attribute("name", get_response_name());
							types[get_response_name()] = responseType;

							complexType = new element("xsd:complexType");
							responseType->append(complexType);
							
							sequence = new element("xsd:sequence");
							complexType->append(sequence);
							
							wsdl_creator wc(types, sequence, false);

							response_type response;
							wc & boost::serialization::make_nvp(m_names[name_count - 1].c_str(), response);
							
							// now the wsdl operations
							element* message = new element("wsdl:message");
							message->set_attribute("name", get_action_name() + "RequestMessage");
							element* n = new element("wsdl:part");
							n->set_attribute("name", "parameters");
							n->set_attribute("element", kPrefix + ':' + get_action_name());
							message->append(n);
							messages[message->get_attribute("name")] = message;

							message = new element("wsdl:message");
							message->set_attribute("name", get_response_name() + "Message");
							n = new element("wsdl:part");
							n->set_attribute("name", "parameters");
							n->set_attribute("element", kPrefix + ':' + get_response_name());
							message->append(n);
							messages[message->get_attribute("name")] = message;
							
							// port type
							element* operation(new element("wsdl:operation"));
							operation->set_attribute("name", get_action_name());
							
							element* input(new element("wsdl:input"));
							input->set_attribute("message", kPrefix + ':' + get_action_name() + "RequestMessage");
							operation->append(input);

							element* output(new element("wsdl:output"));
							output->set_attribute("message", kPrefix + ':' + get_response_name() + "Message");
							operation->append(output);
							
							portType->append(operation);
							
							// and the soap operations
							operation = new element("wsdl:operation");
							operation->set_attribute("name", get_action_name());
							binding->append(operation);
							
							input = new element("wsdl:input");
							operation->append(input);
							
							output = new element("wsdl:output");
							operation->append(output);
							
							element* body(new element("soap:body"));
							body->set_attribute("use", "literal");
							input->append(body);

							body = new element("soap:body");
							body->set_attribute("use", "literal");
							output->append(body);
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
	typedef std::vector<detail::handler_base*>	handler_list;

						dispatcher(const std::string& ns, const std::string& service)
							: m_ns(ns)
							, m_service(service) {}
		
	virtual 			~dispatcher()
						{
							for (handler_list::iterator cb = m_handlers.begin(); cb != m_handlers.end(); ++cb)
								delete *cb;
						}
		
	template<
		class Class,
		typename Function
	>
	void				register_action(const char* action, Class* server, Function call,
							typename detail::handler<Class,Function>::names_type& arg)
						{
							m_handlers.push_back(new detail::handler<Class,Function>(action, server, call, arg));
						}

	/// \brief Dispatch a SOAP message and return the result
	xml::element*		dispatch(xml::element* in)
						{
							return dispatch(in->name(), in);
						}

	/// \brief Dispatch a SOAP message and return the result
	xml::element*		dispatch(const std::string& action, xml::element* in)
						{
//							if (in->ns() != m_ns)
//								throw exception("Invalid request, no match for namespace");
							
							handler_list::iterator cb = std::find_if(
								m_handlers.begin(), m_handlers.end(),
								boost::bind(&detail::handler_base::get_action_name, _1) == action);

							if (cb == m_handlers.end())
								throw exception("Action %s is not defined", action.c_str());

							xml::element* result = (*cb)->call(in);
							result->set_name_space("", m_ns);
							return result;
						}

	/// \brief Create a WSDL based on the registered actions
	xml::element*		make_wsdl(const std::string& address)
						{
							// start by making the root node: wsdl:definitions
							xml::element* wsdl(new xml::element("wsdl:definitions"));
							wsdl->set_attribute("targetNamespace", m_ns);
							wsdl->set_name_space("wsdl", "http://schemas.xmlsoap.org/wsdl/");
							wsdl->set_name_space(xml::kPrefix, m_ns);
							wsdl->set_name_space("soap", "http://schemas.xmlsoap.org/wsdl/soap/");
							
							// add wsdl:types
							xml::element* types(new xml::element("wsdl:types"));
							wsdl->append(types);

							// add xsd:schema
							xml::element* schema(new xml::element("xsd:schema"));
							schema->set_attribute("targetNamespace", m_ns);
							schema->set_name_space("xsd", "http://www.w3.org/2001/XMLSchema");
							schema->set_attribute("elementFormDefault", "qualified");
							schema->set_attribute("attributeFormDefault", "unqualified");
							types->append(schema);

							// add wsdl:binding
							xml::element* binding(new xml::element("wsdl:binding"));
							binding->set_attribute("name", m_service);
							binding->set_attribute("type", xml::kPrefix + ':' + m_service + "PortType");
							
							// add soap:binding
							xml::element* soapBinding(new xml::element("soap:binding"));
							soapBinding->set_attribute("style", "document");
							soapBinding->set_attribute("transport", "http://schemas.xmlsoap.org/soap/http");
							binding->append(soapBinding);
							
							// add wsdl:portType
							xml::element* portType(new xml::element("wsdl:portType"));
							portType->set_attribute("name", m_service + "PortType");
							
							// and the types
							xml::type_map typeMap;
							detail::message_map messageMap;
							
							for (handler_list::iterator cb = m_handlers.begin(); cb != m_handlers.end(); ++cb)
								(*cb)->collect(typeMap, messageMap, portType, binding);
							
							for (detail::message_map::iterator m = messageMap.begin(); m != messageMap.end(); ++m)
								wsdl->append(m->second);
							
							for (xml::type_map::iterator t = typeMap.begin(); t != typeMap.end(); ++t)
								schema->append(t->second);
							
							wsdl->append(portType);
							wsdl->append(binding);
							
							// finish with the wsdl:service
							xml::element* service(new xml::element("wsdl:service"));
							service->set_attribute("name", m_service);
							wsdl->append(service);
							
							xml::element* port(new xml::element("wsdl:port"));
							port->set_attribute("name", m_service);
							port->set_attribute("binding", xml::kPrefix + ':' + m_service);
							service->append(port);
							
							xml::element* soapAddress(new xml::element("soap:address"));
							soapAddress->set_attribute("location", address);
							port->append(soapAddress);
							
							return wsdl;
						}

	void				set_response_name(const std::string& action, const std::string& name)
						{
							handler_list::iterator cb = std::find_if(
								m_handlers.begin(), m_handlers.end(),
								boost::bind(&detail::handler_base::get_action_name, _1) == action);

							if (cb == m_handlers.end())
								throw exception("Action %s is not defined", action.c_str());
							
							(*cb)->set_response_name(name);
						}

	std::string			m_ns;
	std::string			m_service;
	handler_list		m_handlers;
};

}

#ifndef LIBZEEP_DOXYGEN_INVOKED
#define SOAP_DISPATCHER_H
#endif

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
