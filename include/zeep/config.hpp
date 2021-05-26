// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

/// \file
/// Generic configuration file, contains defines and (probably obsolete) stuff for msvc

#pragma once

#include <valarray>
#include <filesystem>
#include <type_traits>

/// The http server implementation in libzeep can use a
/// preforked mode. That means the main process listens to
/// a network port and passes the socket to a client process
/// for doing the actual handling. The advantages for a setup
/// like this is that if the client fails, the server can detect
/// this and restart the client thereby guaranteeing a better
/// uptime.

#ifndef HTTP_SERVER_HAS_PREFORK
#if defined(_MSC_VER)
#define HTTP_SERVER_HAS_PREFORK 0
#else
#define HTTP_SERVER_HAS_PREFORK 1
#endif
#endif

/// The webapp class in libzeep can use resources to load files.
/// In this case you need the resource compiler 'mrc' obtainable
/// from https://github.com/mhekkel/mrc

#ifndef WEBAPP_USES_RESOURCES
#define WEBAPP_USES_RESOURCES 0
#endif

/// The current version of libzeep

#define LIBZEEP_VERSION		  "5.1.0"
#define LIBZEEP_VERSION_MAJOR 5
#define LIBZEEP_VERSION_MINOR 1
#define LIBZEEP_VERSION_PATCH 0

// see if we're using Visual C++, if so we have to include
// some VC specific include files to make the standard C++
// keywords work.

#if defined(_MSC_VER)
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

// configure defined definitions
#include <zeep/config-defs.hpp>
