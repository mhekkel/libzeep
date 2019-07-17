// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <cstdarg>
#include <cstdio>

#include <zeep/exception.hpp>

namespace zeep {

exception::exception(
	const char*		message,
	...)
{
	char msg_buffer[1024];
	
	va_list vl;
	va_start(vl, message);
#if defined(_MSC_VER)
	vsnprintf_s(msg_buffer, sizeof(msg_buffer), _TRUNCATE, message, vl);
#else
	vsnprintf(msg_buffer, sizeof(msg_buffer), message, vl);
#endif
	va_end(vl);

	m_message = msg_buffer;
}

}
