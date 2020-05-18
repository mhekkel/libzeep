// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::http::soap_controller class.
/// Instances of this class take care of mapping member functions to
/// SOAP calls automatically converting in- and output data

#include <zeep/config.hpp>

#include <zeep/http/controller.hpp>
#include <zeep/xml/serialize.hpp>
#include <zeep/xml/document.hpp>
#include <zeep/soap/envelope.hpp>

namespace zeep::soap
{

/// \brief class that helps with handling SOAP requests
///
/// This controller will handle SOAP requests automatically handing the packing
/// and unpacking of XML envelopes.
///
/// To use this, create a subclass and add some methods that should be exposed.
/// Then _map_ these methods on a path that optionally contains parameter values.
///
/// See the chapter on SOAP controllers in the documention for more information.

class controller : public http::controller
{
  public:
	/// \brief constructor
	///
	/// \param prefix_path	This is the leading part of the request URI for each mount point
	/// \param ns			This is the XML Namespace for our SOAP calls
	controller(const std::string& prefix_path, const std::string& ns)
		: http::controller(prefix_path)
		, m_ns(ns)
		, m_service("b")
	{
		while (m_prefix_path.front() == '/')
			m_prefix_path.erase(0, 1);
		m_location = m_prefix_path;
	}

    ~controller()
	{
		for (auto mp: m_mountpoints)
			delete mp;
	}

	/// \brief Set the external address at which this service is visible
	void set_location(const std::string& location)
	{
		m_location = location;
	}

	/// \brief Set the service name
	void set_service(const std::string& service)
	{
		m_service = service;
	}

	/// \brief map a SOAP action to \a callback using \a names for mapping the arguments
	///
	/// The method in \a callback should be a method of the derived class having as many
	/// arguments as the number of specified \a names.
	template<typename Callback, typename... ArgNames>
	void map_action(const char* actionName, Callback callback, ArgNames... names)
	{
		auto mp = m_mountpoints.emplace_back(new mount_point<Callback>(actionName, this, callback, names...));
		mp->collect_types(m_types, m_ns);
	}

	/// \brief Create a WSDL based on the registered actions
	xml::element make_wsdl();

	/// \brief Handle the SOAP request
	virtual bool handle_request(http::request& req, http::reply& reply);

  protected:

	using type_map = std::map<std::string,xml::element>;
	using message_map = std::map<std::string,xml::element>;

	struct mount_point_base
	{
		mount_point_base(const char* action) : m_action(action) {}

        virtual ~mount_point_base() {}

		virtual void call(const xml::element& request, http::reply& reply, const std::string& ns) = 0;
		virtual void collect_types(std::map<std::string,xml::element>& types, const std::string& ns) = 0;

		std::string m_action;
	};

	template<typename Callback, typename...>
	struct mount_point {};

	template<typename ControllerType, typename Result, typename... Args>
	struct mount_point<Result(ControllerType::*)(Args...)> : mount_point_base
	{
		using Sig = Result(ControllerType::*)(Args...);
		using ArgsTuple = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<Args>>...>;
		using Callback = std::function<Result(Args...)>;
 
		static constexpr size_t N = sizeof...(Args);

		mount_point(const char* action, controller* owner, Sig sig)
			: mount_point_base(action)
		{
			ControllerType* controller = dynamic_cast<ControllerType*>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](Args... args) {
				return (controller->*sig)(args...);
			};
		}

		template<typename... Names>
		mount_point(const char* action, controller* owner, Sig sig, Names... names)
			: mount_point(action, owner, sig)
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");
			
			// for (auto name: {...names })
			size_t i = 0;
			for (auto name: { names... })
				m_names[i++] = name;
		}

		virtual void collect_types(std::map<std::string,xml::element>& types, const std::string& ns)
		{
			if constexpr (not std::is_void_v<Result>)
				m_responseType = xml::type_serializer<Result>::schema("Response", ns);
			if constexpr (sizeof...(Args) > 0)
				collect_types(types, ns, std::make_index_sequence<N>());
		}

		template<std::size_t... I>
		void collect_types(std::map<std::string,xml::element>& types, const std::string& ns, std::index_sequence<I...> ix)
		{
			std::make_tuple((m_parameterTypes[I] = collect_type<I>(types, ns))...);
		}

		template<std::size_t I>
		xml::element collect_type(std::map<std::string,xml::element>& types, const std::string& ns)
		{
			using type = typename std::tuple_element_t<I, ArgsTuple>;
			return xml::type_serializer<type>::schema(m_names[I], ns);
		}

		virtual void call(const xml::element& request, http::reply& reply, const std::string& ns)
		{
			reply.set_status(http::ok);

			ArgsTuple args = collect_arguments(request, std::make_index_sequence<N>());
			invoke<Result>(std::move(args), reply, ns);
		}

		template<typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_void_v<ResultType>, int> = 0>
		void invoke(ArgsTuple&& args, http::reply& reply, const std::string& ns)
		{
			std::apply(m_callback, std::forward<ArgsTuple>(args));

			xml::element response(m_action + "Response");
			response.move_to_name_space("m", ns, false, false);
			reply.set_content(soap::make_envelope(std::move(response)));
		}

		template<typename ResultType, typename ArgsTuple, std::enable_if_t<not std::is_void_v<ResultType>, int> = 0>
		void invoke(ArgsTuple&& args, http::reply& reply, const std::string& ns)
		{
			auto result = std::apply(m_callback, std::forward<ArgsTuple>(args));

			// and serialize the result back into XML
			xml::element response(m_action + "Response");
			
			xml::serializer sr(response);
			sr.serialize_element(result);
			response.move_to_name_space("m", ns, true, true);
			
			auto envelope = soap::make_envelope(std::move(response));

			reply.set_content(std::move(envelope));
		}

		template<std::size_t... I>
		ArgsTuple collect_arguments(const xml::element& request, std::index_sequence<I...>)
		{
			xml::deserializer ds(request);

			return std::make_tuple(get_parameter<typename std::tuple_element_t<I, ArgsTuple>>(ds, m_names[I])...);
		}

		template<typename T>
		T get_parameter(xml::deserializer& ds, const char* name)
		{
			T v = {};
			ds.deserialize_element(name, v);
			return v;
		}

		virtual void describe(type_map& types, message_map& messages,
			xml::element& portType, xml::element* binding)
		{
			// the request type
			xml::element requestType("xsd:element", { { "name", m_action }});
			
			auto& complexType = requestType.emplace_back("xsd:complexType");
			auto& sequence = complexType.emplace_back("xsd:sequence");
			
			// // for (size_t)
			// boost::fusion::accumulate(args, m_names.begin(),
			// 	parameter_types<std::string*>(types, sequence));

			// // and the response type
			// element* responseType(new element("xsd:element"));
			// responseType->set_attribute("name", get_response_name());
			// types[get_response_name()] = responseType;

			// complexType = new element("xsd:complexType");
			// responseType->append(complexType);
			
			// sequence = new element("xsd:sequence");
			// complexType->append(sequence);

			// types[m_action] = requestType;

			// schema_creator wc(types, sequence);

			// response_type response;
			// wc.add_element(m_names[name_count - 1].c_str(), response);
			
			// // now the wsdl operations
			// element* message = new element("wsdl:message");
			// message->set_attribute("name", get_action_name() + "RequestMessage");
			// element* n = new element("wsdl:part");
			// n->set_attribute("name", "parameters");
			// n->set_attribute("element", kPrefix + ':' + get_action_name());
			// message->append(n);
			// messages[message->get_attribute("name")] = message;

			// message = new element("wsdl:message");
			// message->set_attribute("name", get_response_name() + "Message");
			// n = new element("wsdl:part");
			// n->set_attribute("name", "parameters");
			// n->set_attribute("element", kPrefix + ':' + get_response_name());
			// message->append(n);
			// messages[message->get_attribute("name")] = message;
			
			// // port type
			// element* operation(new element("wsdl:operation"));
			// operation->set_attribute("name", get_action_name());
			
			// element* input(new element("wsdl:input"));
			// input->set_attribute("message", kPrefix + ':' + get_action_name() + "RequestMessage");
			// operation->append(input);

			// element* output(new element("wsdl:output"));
			// output->set_attribute("message", kPrefix + ':' + get_response_name() + "Message");
			// operation->append(output);
			
			// portType->append(operation);
			
			// // and the soap operations
			// operation = new element("wsdl:operation");
			// operation->set_attribute("name", get_action_name());
			// binding->append(operation);
			// element* soapOperation(new element("soap:operation"));
			// soapOperation->set_attribute("soapAction", "");
			// soapOperation->set_attribute("style", "document");
			// operation->append(soapOperation);
			
			// input = new element("wsdl:input");
			// operation->append(input);

			// output = new element("wsdl:output");
			// operation->append(output);

			// element* body(new element("soap:body"));
			// body->set_attribute("use", "literal");
			// input->append(body);

			// body = new element("soap:body");
			// body->set_attribute("use", "literal");
			// output->append(body);
		}

		Callback m_callback;
		std::array<const char*, N>	m_names;
		xml::element m_responseType;
		std::array<xml::element, N> m_parameterTypes;
	};

	std::list<mount_point_base*> m_mountpoints;
	std::string m_ns, m_location, m_service;
	std::map<std::string,xml::element> m_types;
};

} // namespace zeep::http
