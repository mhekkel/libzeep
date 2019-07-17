// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
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
/// - The name of the action as it is included in the schema.
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
/// to collect all the information required to create a complete schema.

#pragma once

#include <type_traits>

#include <boost/serialization/nvp.hpp>

#include <zeep/xml/node.hpp>
#include <zeep/exception.hpp>
#include <zeep/xml/serialize.hpp>

namespace zeep
{

namespace detail
{

using namespace xml;

template<typename T1, typename... T>
struct result_type
{
	typedef typename result_type<T...>::type type;
};

template<typename T>
struct result_type<T>
{
	typedef typename std::remove_reference<T>::type type;
};

template<typename F>
struct invoker
{
	template<typename Function, typename Response, typename... Args>
	static void invoke(Function f, ::zeep::xml::deserializer& d, const char** name, Response& response, Args... args)
	{
		f(args..., response);
	}

	static void collect_types(schema_creator& c, const char** name)
	{
	}
};

template<typename A0, typename A1, typename... A>
struct invoker<void(A0, A1, A...)>
{
	typedef invoker<void(A1, A...)>	next_invoker;

	template<typename Function, typename Response, typename... Args>
	static void invoke(Function f, ::zeep::xml::deserializer& d, const char** name, Response& response, Args... args)
	{
		typedef typename std::remove_const<typename std::remove_reference<A0>::type>::type argument_type;

		argument_type arg = {};
 		d.deserialize_element(*name, arg);
		next_invoker::invoke(f, d, ++name, response, arg, args...);
	}

	static void collect_types(schema_creator& c, const char** name)
	{
		typedef typename std::remove_const<typename std::remove_reference<A0>::type>::type argument_type;

		argument_type arg = {};

		c.add_element(*name, arg);
		next_invoker::collect_types(c, ++name);
	}
};

template <typename Function>
struct handler_traits;

template<typename... Args>
struct handler_traits<void(Args...)>
{
	typedef typename result_type<Args...>::type			result_type;

	static_assert(not std::is_reference<result_type>::value, "result type (last argument) should be a reference");

	template<typename Function>
	static void invoke(Function func, element* in, const char* names[], result_type& response)
	{
		::zeep::xml::deserializer d(in);
		invoker<void(Args...)>::invoke(func, d, names, response);
	}

	static void collect_types(schema_creator sc, const char* names[])
	{
		invoker<void(Args...)>::collect_types(sc, names);
	}
};

// messages can be used by more than one action, so we need a way to avoid duplicates
typedef std::map<std::string, element *> message_map;

struct handler_base
{
	handler_base(const std::string& action)
		: m_action(action), m_response(action + "Response") {}
	virtual ~handler_base() {}

	virtual element* call(element* in) = 0;

	virtual void collect(type_map& types, message_map& messages,
						 element* portType, element* binding) = 0;

	const std::string& get_action_name() const { return m_action; }

	std::string get_response_name() const { return m_response; }
	void set_response_name(const std::string& name) { m_response = name; }

	std::string m_action, m_response;
};

template <typename Function> struct handler;

template<typename... Args>
struct handler<void(Args...)> : public handler_base
{
	typedef handler_traits<void(Args...)> handler_traits_type;
	typedef typename handler_traits_type::result_type response_type;

	static constexpr std::size_t name_count = sizeof...(Args);
	typedef const char* names_type[name_count];

	handler(const char* action, std::function<void(Args...)>&& callback, std::initializer_list<const char*> names)
		: handler_base(action), m_method(callback)
	{
		std::copy(names.begin(), names.end(), m_names.begin());
	}

	virtual element* call(element* in)
	{
		// call the actual server code
		response_type response;
		handler_traits_type::invoke(m_method, in, m_names.data(), response);

		// and serialize the result back into XML
		element* result(new element(get_response_name()));
		::zeep::xml::serializer sr(result);
		sr.serialize_element(m_names[name_count - 1], response);

		// that's all, we're done
		return result;
	}

	virtual void collect(type_map& types, message_map& messages,
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

		schema_creator sc(types, sequence);
		handler_traits_type::collect_types(sc, m_names.data());

		// and the response type
		element* responseType(new element("xsd:element"));
		responseType->set_attribute("name", get_response_name());
		types[get_response_name()] = responseType;

		complexType = new element("xsd:complexType");
		responseType->append(complexType);

		sequence = new element("xsd:sequence");
		complexType->append(sequence);

		schema_creator wc(types, sequence);

		response_type response;
		wc.add_element(m_names[name_count - 1], response);

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
		element* soapOperation(new element("soap:operation"));
		soapOperation->set_attribute("soapAction", "");
		soapOperation->set_attribute("style", "document");
		operation->append(soapOperation);

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

	std::function<void(Args...)> m_method;
	std::array<const char*, name_count> m_names;
};

template<typename Class, typename Method> struct handler_factory;

template<typename Class, typename... Arguments>
struct handler_factory<Class, void(Class::*)(Arguments...)>
{
	typedef void(Class::*callback_method_type)(Arguments...);
	typedef handler<void(Arguments...)>			handler_type;
	typedef std::function<void(Arguments...)>	callback_type;

	static callback_type create_callback(Class* server, callback_method_type method)
	{
		return [server, method](Arguments... args)
		{
			(server->*method)(args...);
		};
	}
};

} // namespace detail

class dispatcher
{
public:
	typedef std::vector<detail::handler_base *> handler_list;

	dispatcher(const std::string& ns, const std::string& service)
		: m_ns(ns), m_service(service) {}

	virtual ~dispatcher()
	{
		for (handler_list::iterator cb = m_handlers.begin(); cb != m_handlers.end(); ++cb)
			delete* cb;
	}

	template <typename Class, typename Method>
	void register_action(const char* action, Class* server, Method method, std::initializer_list<const char*> names)
	{
		typedef typename detail::handler_factory<Class, Method> handler_factory;
		typedef typename handler_factory::handler_type handler_type;

		m_handlers.push_back(new handler_type(action, handler_factory::create_callback(server, method), names));
	}

	/// \brief Dispatch a SOAP message and return the result
	xml::element* dispatch(xml::element* in)
	{
		return dispatch(in->name(), in);
	}

	/// \brief Dispatch a SOAP message and return the result
	xml::element* dispatch(const std::string& action, xml::element* in)
	{
		if (in->ns() != m_ns)
			throw exception("Invalid request, no match for namespace");

		handler_list::iterator cb = std::find_if(
			m_handlers.begin(), m_handlers.end(),
			[action](auto h) { return h->get_action_name() == action; });

		if (cb == m_handlers.end())
			throw exception("Action %s is not defined", action.c_str());

		xml::element* result = (*cb)->call(in);
		result->set_name_space("", m_ns);
		return result;
	}

	/// \brief Create a WSDL based on the registered actions
	xml::element* make_wsdl(const std::string& address)
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

	void set_response_name(const std::string& action, const std::string& name)
	{
		handler_list::iterator cb = std::find_if(
			m_handlers.begin(), m_handlers.end(),
			[action](auto h) { return h->get_action_name() == action; });

		if (cb == m_handlers.end())
			throw exception("Action %s is not defined", action.c_str());

		(*cb)->set_response_name(name);
	}

	std::string m_ns;
	std::string m_service;
	handler_list m_handlers;
};

} // namespace zeep

