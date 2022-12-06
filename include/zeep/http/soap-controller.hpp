// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2022
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::soap_controller class.
/// Instances of this class take care of mapping member functions to
/// SOAP calls automatically converting in- and output data

#include <zeep/config.hpp>

#include <zeep/http/controller.hpp>
#include <zeep/xml/document.hpp>
#include <zeep/xml/serialize.hpp>

namespace zeep::http
{

/// soap_envelope is a wrapper around a SOAP envelope. Use it for
/// input and output of correctly formatted SOAP messages.

class soap_envelope
{
  public:
	soap_envelope(const soap_envelope &) = delete;
	soap_envelope &operator=(const soap_envelope &) = delete;

	/// \brief Create an empty envelope
	soap_envelope();

	/// \brief Parse a SOAP message from the payload received from a client,
	/// throws an exception if the envelope is empty or invalid.
	soap_envelope(std::string &payload);

	// /// \brief Parse a SOAP message received from a client,
	// /// throws an exception if the envelope is empty or invalid.
	// envelope(xml::document& data);

	/// \brief The request element as contained in the original SOAP message
	xml::element &request() { return *m_request; }

  private:
	xml::document m_payload;
	xml::element *m_request;
};

// --------------------------------------------------------------------

/// Wrap data into a SOAP envelope
///
/// \param    data  The xml::element object to wrap into the envelope
/// \return   A new xml::element object containing the envelope.
xml::element make_envelope(xml::element &&data);
// xml::element make_envelope(const xml::element& data);

/// Create a standard SOAP Fault message for the string parameter
///
/// \param    message The string object containing a descriptive error message.
/// \return   A new xml::element object containing the fault envelope.
xml::element make_fault(const std::string &message);
/// Create a standard SOAP Fault message for the exception object
///
/// \param    ex The exception object that was catched.
/// \return   A new xml::element object containing the fault envelope.
xml::element make_fault(const std::exception &ex);

// --------------------------------------------------------------------

/// \brief class that helps with handling SOAP requests
///
/// This controller will handle SOAP requests automatically handing the packing
/// and unpacking of XML envelopes.
///
/// To use this, create a subclass and add some methods that should be exposed.
/// Then _map_ these methods on a path that optionally contains parameter values.
///
/// See the chapter on SOAP controllers in the documention for more information.

class soap_controller : public controller
{
  public:
	/// \brief constructor
	///
	/// \param prefix_path	This is the leading part of the request URI for each mount point
	/// \param ns			This is the XML Namespace for our SOAP calls
	soap_controller(const std::string &prefix_path, const std::string &ns)
		: controller(prefix_path)
		, m_ns(ns)
		, m_service("b")
	{
		while (m_prefix_path.front() == '/')
			m_prefix_path.erase(0, 1);
		m_location = m_prefix_path;
	}

	~soap_controller()
	{
		for (auto mp : m_mountpoints)
			delete mp;
	}

	/// \brief Set the external address at which this service is visible
	void set_location(const std::string &location)
	{
		m_location = location;
	}

	/// \brief Set the service name
	void set_service(const std::string &service)
	{
		m_service = service;
	}

	/// \brief map a SOAP action to \a callback using \a names for mapping the arguments
	///
	/// The method in \a callback should be a method of the derived class having as many
	/// arguments as the number of specified \a names.
	template <typename Callback, typename... ArgNames>
	void map_action(const char *actionName, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(actionName, this, callback, names...));
	}

	/// \brief Create a WSDL based on the registered actions
	xml::element make_wsdl();

	/// \brief Handle the SOAP request
	virtual bool handle_request(request &req, reply &reply);

  protected:
	using type_map = std::map<std::string, xml::element>;
	using message_map = std::map<std::string, xml::element>;

	struct mount_point_base
	{
		mount_point_base(const char *action)
			: m_action(action)
		{
		}

		virtual ~mount_point_base() {}

		virtual void call(const xml::element &request, reply &reply, const std::string &ns) = 0;
		virtual void describe(type_map &types, message_map &messages, xml::element &portType, xml::element &binding) = 0;

		std::string m_action;
	};

	template <typename Callback, typename...>
	struct mount_point
	{
	};

	template <typename ControllerType, typename Result, typename... Args>
	struct mount_point<Result (ControllerType::*)(Args...)> : mount_point_base
	{
		using Sig = Result (ControllerType::*)(Args...);
		using ArgsTuple = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<Args>>...>;
		using Callback = std::function<Result(Args...)>;

		static constexpr size_t N = sizeof...(Args);

		mount_point(const char *action, soap_controller *owner, Sig sig)
			: mount_point_base(action)
		{
			ControllerType *controller = dynamic_cast<ControllerType *>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](Args... args)
			{
				return (controller->*sig)(args...);
			};
		}

		template <typename... Names>
		mount_point(const char *action, soap_controller *owner, Sig sig, Names... names)
			: mount_point(action, owner, sig)
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");

			// for (auto name: {...names })
			size_t i = 0;
			for (auto name : { names... })
				m_names[i++] = name;
		}

		virtual void call(const xml::element &request, reply &reply, const std::string &ns)
		{
			reply.set_status(ok);

			ArgsTuple args = collect_arguments(request, std::make_index_sequence<N>());
			invoke<Result>(std::move(args), reply, ns);
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<std::is_void_v<ResultType>, int> = 0>
		void invoke(ArgsTuple &&args, reply &reply, const std::string &ns)
		{
			std::apply(m_callback, std::forward<ArgsTuple>(args));

			xml::element response(m_action + "Response");
			response.move_to_name_space("m", ns, false, false);
			reply.set_content(make_envelope(std::move(response)));
		}

		template <typename ResultType, typename ArgsTuple, std::enable_if_t<not std::is_void_v<ResultType>, int> = 0>
		void invoke(ArgsTuple &&args, reply &reply, const std::string &ns)
		{
			auto result = std::apply(m_callback, std::forward<ArgsTuple>(args));

			// and serialize the result back into XML
			xml::element response(m_action + "Response");

			xml::serializer sr(response);
			sr.serialize_element(result);
			response.move_to_name_space("m", ns, true, true);

			auto envelope = make_envelope(std::move(response));

			reply.set_content(std::move(envelope));
		}

		template <std::size_t... I>
		ArgsTuple collect_arguments(const xml::element &request, std::index_sequence<I...>)
		{
			xml::deserializer ds(request);

			return std::make_tuple(get_parameter<typename std::tuple_element_t<I, ArgsTuple>>(ds, m_names[I])...);
		}

		template <typename T>
		T get_parameter(xml::deserializer &ds, const char *name)
		{
			T v = {};
			ds.deserialize_element(name, v);
			return v;
		}

		virtual void collect_types(xml::type_map &types, xml::element &seq, const std::string &ns)
		{
			if constexpr (sizeof...(Args) > 0)
				collect_types(types, seq, ns, std::make_index_sequence<N>());
		}

		template <std::size_t... I>
		void collect_types(xml::type_map &types, xml::element &seq, const std::string &ns, [[maybe_unused]] std::index_sequence<I...> ix)
		{
			(collect_type<I>(types, seq, ns), ...);
		}

		template <std::size_t I>
		void collect_type(xml::type_map &types, xml::element &seq, [[maybe_unused]] const std::string &ns)
		{
			using type = typename std::tuple_element_t<I, ArgsTuple>;

			xml::schema_creator sc(types, seq);

			sc.add_element(m_names[I], type{});
		}

		virtual void describe(type_map &types, message_map &messages,
			xml::element &portType, xml::element &binding)
		{
			// the request type
			xml::element requestType("xsd:element", { { "name", m_action } });
			auto &complexType = requestType.emplace_back("xsd:complexType");

			collect_types(types, complexType.emplace_back("xsd:sequence"), "ns");

			types[m_action + "Request"] = requestType;

			// and the response type
			xml::element responseType("xsd:element", { { "name", m_action + "Response" } });

			if constexpr (not std::is_void_v<Result>)
			{
				auto &complexType = responseType.emplace_back("xsd:complexType");
				auto &sequence = complexType.emplace_back("xsd:sequence");

				xml::schema_creator sc(types, sequence);
				sc.add_element("Response", Result{});
			}

			types[m_action + "Response"] = responseType;

			// now the wsdl operations
			xml::element message("wsdl:message", {{ "name", m_action + "RequestMessage"}});
			message.emplace_back("wsdl:part", { {"name", "parameters"}, { "element", "ns:" + m_action }});
			messages[m_action + "RequestMessage"] = message;

			message = xml::element("wsdl:message", {{ "name", m_action + "Message" }});
			message.emplace_back("wsdl:part", {{ "name", "parameters"}, {"element", "ns:" + m_action }});
			messages[m_action + "Message"] = message;

			// port type
			xml::element operation("wsdl:operation", { { "name", m_action } });

			operation.emplace_back("wsdl:input", { { "message", "ns:" + m_action + "RequestMessage" } });
			operation.emplace_back("wsdl:output", { { "message", "ns:" + m_action + "Message" } });

			portType.emplace_back(std::move(operation));

			// and the soap operations
			operation = { "wsdl:operation", { { "name", m_action } } };
			operation.emplace_back("soap:operation", { { "soapAction", "" }, { "style", "document" } });

			xml::element body("soap:body");
			body.set_attribute("use", "literal");

			xml::element input("wsdl:input");
			input.push_back(body);
			operation.emplace_back(std::move(input));

			xml::element output("wsdl:output");
			output.emplace_back(std::move(body));
			operation.emplace_back(std::move(output));

			binding.emplace_back(std::move(operation));
		}

		Callback m_callback;
		std::array<const char *, N> m_names;
	};

	std::list<mount_point_base *> m_mountpoints;
	std::string m_ns, m_location, m_service;
};

} // namespace zeep::http
