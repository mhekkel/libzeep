// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

///

#ifndef SOAP_ENVELOPE_H
#define SOAP_ENVELOPE_H

#include <zeep/xml/document.hpp>
#include <zeep/exception.hpp>

/// Envelope is a wrapper around a SOAP envelope. Use it for
/// input and output of correctly formatted SOAP messages.

namespace zeep
{

class envelope : public boost::noncopyable
{
public:
	/// \brief Create an empty envelope
	envelope();

	/// \brief Parse a SOAP message received from a client,
	/// throws an exception if the envelope is empty or invalid.
	envelope(xml::document& data);

	/// \brief The request element as contained in the original SOAP message
	xml::element* request() { return m_request; }

private:
	xml::element* m_request;
};

/// Wrap data into a SOAP envelope
///
/// \param    data  The xml::element object to wrap into the envelope
/// \return   A new xml::element object containing the envelope.
xml::element* make_envelope(xml::element* data);
/// Create a standard SOAP Fault message for the string parameter
///
/// \param    message The string object containing a descriptive error message.
/// \return   A new xml::element object containing the fault envelope.
xml::element* make_fault(const std::string& message);
/// Create a standard SOAP Fault message for the exception object
///
/// \param    ex The exception object that was catched.
/// \return   A new xml::element object containing the fault envelope.
xml::element* make_fault(const std::exception& ex);

} // namespace zeep

#endif
