// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of the zeep::http::soap_controller class.
/// Instances of this class take care of mapping member functions to
/// SOAP calls automatically converting in- and output data

#ifndef ZEEP_CXX_MODULE
# include "zeep/export.hpp"
# include "zeep/config.hpp"

# include "zeep/http/controller.hpp"

# include <zeem/zeem.hpp>
#endif

namespace zeep::http
{

/// \brief Wrapper around a SOAP envelope for input and output of correctly formatted SOAP messages

ZEEP_EXPORT class soap_envelope
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
	// envelope(zeem::document& data);

	/// \brief The request element as contained in the original SOAP message
	/// \return The request \a zeem::element
	zeem::element &request() { return *m_request; }

  private:
	zeem::document m_payload;
	zeem::element *m_request;
};

// --------------------------------------------------------------------

/// \brief Wrap data into a SOAP envelope
///
/// \param    data  The \a zeem::element object to wrap into the envelope
/// \return   A new \a zeem::element object containing the envelope.
ZEEP_EXPORT zeem::element make_envelope(zeem::element &&data);
// zeem::element make_envelope(const zeem::element& data);

/// \brief Create a standard SOAP Fault message for the string parameter
///
/// \param    message The string object containing a descriptive error message.
/// \return   A new \a zeem::element object containing the fault envelope.
ZEEP_EXPORT zeem::element make_fault(std::string message);
/// \brief Create a standard SOAP Fault message for the exception object
///
/// \param    ex The exception object that was caught.
/// \return   A new \a zeem::element object containing the fault envelope.
ZEEP_EXPORT zeem::element make_fault(const std::exception &ex);

// --------------------------------------------------------------------

/// \brief Controller class that helps with handling SOAP requests
///
/// This controller will handle SOAP requests automatically handling the packing
/// and unpacking of XML envelopes.
///
/// To use this, create a subclass and add some methods that should be exposed.
/// Then _map_ these methods on a path that optionally contains parameter values.
///
/// See the chapter on SOAP controllers in the documentation for more information.

ZEEP_EXPORT class soap_controller : public controller
{
  public:
	/// \brief constructor
	///
	/// \param prefix_path	This is the leading part of the request URI for each mount point
	/// \param service      The name of the service
	/// \param ns			This is the XML Namespace for our SOAP calls
	soap_controller(const std::string &prefix_path, std::string service, std::string ns)
		: controller(prefix_path)
		, m_ns(std::move(ns))
		, m_service(std::move(service))
	{
		// while (m_prefix_path.front() == '/')
		// 	m_prefix_path.erase(0, 1);
		m_location = m_prefix_path.string();
	}

	/// \brief Set the external address at which this service is visible
	/// \param location   The external URL of this service
	void set_location(std::string location) noexcept
	{
		m_location = std::move(location);
	}

	/// \brief Set the service name
	/// \param service   The name of the service
	void set_service(std::string service) noexcept
	{
		m_service = std::move(service);
	}

	/// \brief Map a SOAP action to \a callback using \a names for mapping the arguments
	///
	/// The method in \a callback should be a method of the derived class having as many
	/// arguments as the number of specified \a names.
	/// \tparam Callback    The callback function type
	/// \tparam ArgNames    The parameter name types
	/// \param actionName   The SOAP action name
	/// \param callback     The member function pointer to invoke
	/// \param names        The parameter names for mapping XML elements to arguments
	template <typename Callback, typename... ArgNames>
	void map_action(const char *actionName, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(actionName, this, callback, names...));
	}

	/// \brief Create a WSDL based on the registered actions
	/// \return A \a zeem::element containing the WSDL description
	zeem::element make_wsdl();

	/// \brief Handle the SOAP request
	/// \param req      The incoming SOAP request
	/// \param reply_   The reply to populate with the SOAP response
	/// \return True if the request was handled
	bool handle_request(request &req, reply &reply_) override;

  protected:
	/// @cond

	using type_map = std::map<std::string, zeem::element>;
	using message_map = std::map<std::string, zeem::element>;

	/// \brief Base class for SOAP mount points
	struct mount_point_base
	{
		/// \brief Construct a SOAP mount point base
		/// \param action   The SOAP action name
		mount_point_base(const char *action)
			: m_action(action)
		{
		}

		virtual ~mount_point_base() = default;

		/// \brief Invoke the mount point handler
		/// \param request   The SOAP request element
		/// \param reply_    The reply to populate
		/// \param ns        The XML namespace
		virtual void call(const zeem::element &request, reply &reply_, std::string_view ns) = 0;
		/// \brief Describe this action for WSDL generation
		virtual void describe(type_map &types, message_map &messages, zeem::element &portType, zeem::element &binding) = 0;

		std::string m_action; ///< The SOAP action name
	};

	template <typename Callback, typename...>
	struct mount_point
	{
	};

	/// \brief Templated abstract base class for SOAP mount points
	/// \tparam ControllerType   The type of the controller
	/// \tparam Result           The return type of the callback
	/// \tparam Args             The argument types of the callback
	template <typename ControllerType, typename Result, typename... Args>
	struct mount_point<Result (ControllerType::*)(Args...)> : mount_point_base
	{
		using Sig = Result (ControllerType::*)(Args...);
		using ArgsTuple = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<Args>>...>;
		using Callback = std::function<Result(Args...)>;

		static constexpr size_t N = sizeof...(Args);

		/// \brief Construct a mount point
		/// \param action   The SOAP action name
		/// \param owner    The owning controller
		/// \param sig      The member function pointer
		mount_point(const char *action, soap_controller *owner, Sig sig)
			: mount_point_base(action)
		{
			auto *controller = dynamic_cast<ControllerType *>(owner);
			if (controller == nullptr)
				throw exception("Invalid controller for callback");

			m_callback = [controller, sig](Args... args)
			{
				return (controller->*sig)(args...);
			};
		}

		/// \brief Construct a mount point with named parameters
		/// \tparam Names   The parameter name types
		/// \param action   The SOAP action name
		/// \param owner    The owning controller
		/// \param sig      The member function pointer
		/// \param names    The parameter names for XML element mapping
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

		/// \brief Invoke the mount point handler
		/// \param request   The SOAP request element
		/// \param rep       The reply to populate
		/// \param ns        The XML namespace
		void call(const zeem::element &request, reply &rep, std::string_view ns) override
		{
			rep.set_status(status_type::ok);

			ArgsTuple args = collect_arguments(request, std::make_index_sequence<N>());
			invoke<Result>(std::move(args), rep, ns);
		}

		/// \brief Invoke for void-returning callbacks
		template <typename ResultType, typename ArgsTuple>
		void invoke(ArgsTuple &&args, reply &rep, std::string_view ns)
			requires(std::is_void_v<ResultType>)
		{
			std::apply(m_callback, std::forward<ArgsTuple>(args));

			zeem::element response(m_action + "Response");
			response.move_to_name_space("m", ns, false, false);
			rep.set_content(make_envelope(std::move(response)));
		}

		/// \brief Invoke for non-void-returning callbacks
		template <typename ResultType, typename ArgsTuple>
		void invoke(ArgsTuple &&args, reply &rep, std::string_view ns)
			requires(not std::is_void_v<ResultType>)
		{
			auto result = std::apply(m_callback, std::forward<ArgsTuple>(args));

			// and serialize the result back into XML
			zeem::element response(m_action + "Response");

			zeem::serializer sr(response);
			sr.serialize_element(result);
			response.move_to_name_space("m", ns, true, true);

			auto envelope = make_envelope(std::move(response));

			rep.set_content(envelope);
		}

		/// \brief Collect arguments from the SOAP request XML
		template <std::size_t... I>
		ArgsTuple collect_arguments(const zeem::element &request, std::index_sequence<I...> /* unused */)
		{
			zeem::deserializer ds(request);

			return std::make_tuple(get_parameter<typename std::tuple_element_t<I, ArgsTuple>>(ds, m_names[I])...);
		}

		/// \brief Extract a single parameter from the deserializer
		template <typename T>
		T get_parameter(zeem::deserializer &ds, const char *name)
		{
			T v = {};
			ds.deserialize_element(name, v);
			return v;
		}

		/// \brief Collect types for WSDL schema generation
		virtual void collect_types(type_map &types, zeem::element &seq, std::string_view ns)
		{
			if constexpr (sizeof...(Args) > 0)
				collect_types(types, seq, ns, std::make_index_sequence<N>());
		}

		template <std::size_t... I>
		void collect_types(type_map &types, zeem::element &seq, std::string_view ns, std::index_sequence<I...> /*ix*/)
		{
			(collect_type<I>(types, seq, ns), ...);
		}

		template <std::size_t I>
		void collect_type(type_map &types, zeem::element &seq, std::string_view /*ns*/)
		{
			using type = typename std::tuple_element_t<I, ArgsTuple>;

			zeem::schema_creator sc(types, seq);
			sc.set_ns_prefix("ns");

			sc.add_element(m_names[I], type{});
		}

		/// \brief Describe this action for WSDL generation
		void describe(type_map &types, message_map &messages,
			zeem::element &portType, zeem::element &binding) override
		{
			// the request type
			zeem::element requestType("xsd:element", { { "name", m_action } });
			auto complexType = requestType.emplace_back("xsd:complexType");

			collect_types(types, *complexType->emplace_back("xsd:sequence"), "ns");

			types[m_action + "Request"] = requestType;

			// and the response type
			zeem::element responseType("xsd:element", { { "name", m_action + "Response" } });

			if constexpr (not std::is_void_v<Result>)
			{
				auto complexType2 = responseType.emplace_back("xsd:complexType");
				auto sequence = complexType2->emplace_back("xsd:sequence");

				zeem::schema_creator sc(types, *sequence);
				sc.set_ns_prefix("ns");
				sc.add_element("Response", Result{});
			}

			types[m_action + "Response"] = responseType;

			// now the wsdl operations
			zeem::element message("wsdl:message", { { "name", m_action + "RequestMessage" } });
			message.emplace_back(zeem::element{ "wsdl:part", { { "name", "parameters" }, { "element", "ns:" + m_action } } });
			messages[m_action + "RequestMessage"] = message;

			message = zeem::element("wsdl:message", { { "name", m_action + "Message" } });
			message.emplace_back(zeem::element{ "wsdl:part", { { "name", "parameters" }, { "element", "ns:" + m_action } } });
			messages[m_action + "Message"] = message;

			// port type
			zeem::element operation("wsdl:operation", { { "name", m_action } });

			operation.emplace_back(zeem::element{ "wsdl:input", { { "message", "ns:" + m_action + "RequestMessage" } } });
			operation.emplace_back(zeem::element{ "wsdl:output", { { "message", "ns:" + m_action + "Message" } } });

			portType.emplace_back(std::move(operation));

			// and the soap operations
			operation = zeem::element{ "wsdl:operation", { { "name", m_action } } };
			operation.emplace_back(zeem::element{ "soap:operation", { { "soapAction", "" }, { "style", "document" } } });

			zeem::element body("soap:body");
			body.set_attribute("use", "literal");

			zeem::element input("wsdl:input");
			input.push_back(body);
			operation.emplace_back(std::move(input));

			zeem::element output("wsdl:output");
			output.emplace_back(std::move(body));
			operation.emplace_back(std::move(output));

			binding.emplace_back(std::move(operation));
		}

		Callback m_callback;                 ///< The stored callback function
		std::array<const char *, N> m_names; ///< The parameter names for XML element mapping
	};

	std::list<std::unique_ptr<mount_point_base>> m_mountpoints; ///< The list of registered SOAP mount points
	std::string m_ns;                                           ///< The XML namespace
	std::string m_location;                                     ///< The external address of this service
	std::string m_service;                                      ///< The service name

	/// @endcond
};

} // namespace zeep::http
