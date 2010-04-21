//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_CONFIG_H
#define SOAP_CONFIG_H

/// Libzeep comes with its own XML parser implementation. If you prefer
/// you can use expat instead. To do so you have to define the 
/// SOAP_XML_HAS_EXPAT_SUPPORT flag and then you can call the
/// zeep::xml::document::set_parser_type function to specify expat.

#ifndef SOAP_XML_HAS_EXPAT_SUPPORT
#define SOAP_XML_HAS_EXPAT_SUPPORT 0
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
#endif

/// My code uses nil as NULL, it just looks better on my eyes.
/// Eventually we'll have to change this to the new C++x0 keyword
/// nullptr.

#ifndef nil
#define nil NULL
#endif

// more defines may follow here

#endif
