// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

/// 

#ifndef SOAP_ENVELOPE_H
#define SOAP_ENVELOPE_H

#include <zeep/xml/document.hpp>
#include <zeep/exception.hpp>

namespace zeep
{

class envelope : public boost::noncopyable
{
  public:
					envelope();
	
					envelope(xml::document& data);

	xml::element*	request()						{ return m_request; }

  private:
	xml::element*	m_request;
};

xml::element* make_envelope(xml::element* data);
xml::element* make_fault(const std::string& message);
xml::element* make_fault(const std::exception& ex);

}

#endif
