// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_CONFIG_H
#define SOAP_CONFIG_H

/// Libzeep comes with its own XML parser implementation. If you prefer
/// you can use expat instead. To do so you have to define the 
/// SOAP_XML_HAS_EXPAT_SUPPORT flag and then you can call the
/// zeep::xml::document::set_parser_type function to specify expat.

#ifndef SOAP_XML_HAS_EXPAT_SUPPORT
#define SOAP_XML_HAS_EXPAT_SUPPORT 0
#endif

// The http server implementation in libzeep can use a
// preforked mode. That means the main process listens to
// a network port and passes the socket to a client process
// for doing the actual handling. The advantages for a setup
// like this is that if the client fails, the server can detect
// this and restart the client thereby guaranteeing a better
// uptime.

#ifndef SOAP_SERVER_HAS_PREFORK
#if defined(_MSC_VER)
#define SOAP_SERVER_HAS_PREFORK 0
#else
#define SOAP_SERVER_HAS_PREFORK 1
#endif
#endif

/// see if we're using Visual C++, if so we have to include
/// some VC specific include files to make the standard C++
/// keywords work.

#if defined(_MSC_VER) && !defined(__MWERKS__)
#	if defined(_MSC_EXTENSIONS)		// why is it an extension to leave out something?
#		define and		&&
#		define and_eq	&=
#		define bitand	&
#		define bitor	|
#		define compl	~
#		define not		!
#		define not_eq	!=
#		define or		||
#		define or_eq	|=
#		define xor		^
#		define xor_eq	^=
#	endif // _MSC_EXTENSIONS

#	pragma warning (disable : 4355)	// this is used in Base Initializer list
#	pragma warning (disable : 4996)	// unsafe function or variable
#	pragma warning (disable : 4068)	// unknown pragma
#	pragma warning (disable : 4996)	// stl copy()
#	pragma warning (disable : 4800)	// BOOL conversion
#endif

// GCC 4.4 and before do not know nullptr
#if defined (__GNUC__) && ((__GNUC__ < 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ <= 5)))
const                        // this is a const object...
class {
public:
  template<class T>          // convertible to any type
    operator T*() const      // of null non-member
    { return 0; }            // pointer...
  template<class C, class T> // or any type of null
    operator T C::*() const  // member pointer...
    { return 0; }
private:
  void operator&() const;    // whose address can't be taken
} nullptr = {};              // and whose name is nullptr
#endif

/// fixes for cygwin/boost-1.43 combo
/// source:
///  https://svn.boost.org/trac/boost/attachment/ticket/4816/boost_asio_bug_cygwin_with_fix.cpp
#ifdef __CYGWIN__
//////////////// FIX STARTS HERE
/// 1st issue
#include <boost/asio/detail/pipe_select_interrupter.hpp>

/// 2nd issue
#include <termios.h>
#ifdef cfgetospeed
#define __cfgetospeed__impl(tp) cfgetospeed(tp)
#undef cfgetospeed
inline speed_t cfgetospeed(const struct termios *tp)
{
        //return ((tp)->c_ospeed);
        return __cfgetospeed__impl(tp);
}
#undef __cfgetospeed__impl
#endif /// cfgetospeed is a macro

/// 3rd issue
#undef __CYGWIN__
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#define __CYGWIN__
//////////////// FIX ENDS HERE
#endif


#endif
