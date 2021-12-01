[![Build Status](https://travis-ci.org/mhekkel/libzeep.svg?branch=master)](https://travis-ci.org/mhekkel/libzeep)
[![DOI](https://zenodo.org/badge/44161414.svg)](https://zenodo.org/badge/latestdoi/44161414)

About libzeep
=============

Libzeep was originally developed to make it easy to create SOAP servers. And since
working with SOAP means working with XML and no decent C++ XML library
existed on my radar I created a full XML library as well.

Unfortunately (well, considering the work I did), REST proved to be more
popular than SOAP, and so I added a better JSON implementation to version
4 of libzeep as well as a way to create REST servers more easily.

But then I had to use Spring for some time and was impressed by the simplicity
of building interactive web applications and thought I should bring that
simplicity to the C++ world. After all, my applications need raw speed and
no, Java is not fast.

The current incarnation of libzeep, version 5.0, is a completely refactored
set of libraries. One for manipulating XML, one for handling JSON and one for
building web applications.

The XML part of libzeep consists of a validating parser, a DOM(-like) node
implementation, an XPath search engine and a XML writer/formatter. The
validation works based on DOCTYPE definitions.

Please note that libzeep aims to provide a fully compliant XML processor as
specified by the W3 organisation (see: [www.w3.org/TR/xml](https://www.w3.org/TR/xml) ).
This means
it is as strict as the standard requires and it stops processing a file when
a validation of the well-formedness is encountered, or when a document
appears to be invalid when it is in validating mode. Error reporting is done
in this case.

The JSON library in itself is fairly simple. There are much better alternatives
if you're looking for just JSON. But this implementation is required by the
web application part. 

And then we have a web application library. This one makes it very easy to build
a HTTP server that serves HTML but also speaks REST and SOAP. The current
implementation consists of a HTTP server class to which you can add controllers.
Each controller has a path prefix and handles requests for some entries in this
uri path. The base class controller is simple and in fact is just a base class.

The HTML controller can be used as a base class so you can add methods that
will be called for certain URI paths. In combination with the available tag
processors you can then create and return dynamic XHTML pages.

The REST and SOAP controllers likewise can be used as base classes to export
methods that take simple or complex named parameters and return JSON and SOAP
enveloped data structures respectively.

Full documentation can be found at:

[www.hekkelman.com/libzeep-doc/](https://www.hekkelman.com/libzeep-doc/)

Building libzeep
----------------

To build libzeep you have to have [cmake](https://cmake.org/) installed.

It is also recommended to install [mrc](https://github.com/mhekkel/mrc) in
order to have resources support in libzeep.

The commands to build libzeep from the command line are e.g.:

```
	git clone https://github.com/mhekkel/libzeep
	cd libzeep
	mkdir build
	cd build
	cmake .. -DZEEP_BUILD_TESTS=ON
	cmake --build .
	ctest
	cmake --install .

```

On Windows, assuming you have [boost](https://boost.org) installed in C:\Boost, 
the steps would probably look something like (using powershell):

```
	git clone https://github.com/mhekkel/libzeep
	cd libzeep
	mkdir build
	cd build
	cmake .. -DZEEP_BUILD_TESTS=ON -DBOOST_ROOT=C:\Boost
	cmake --build . --config Release
	ctest -C Release
	cmake --install . --config Release

```

The windows version will by default install in your local AppData folder.
Use the --prefix option to specify another location.
