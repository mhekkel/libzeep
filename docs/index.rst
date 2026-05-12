Introduction
============

Libzeep started as a spin-off of `MRS <https://mrs.cmbi.umcn.nl>`_ which is a tool to index and search large text-based bioinformatics databanks. The code that generates a SOAP server in compile time in MRS was needed in another project and this is how libzeep started. BTW, zeep is the dutch word for soap.

One of the major parts of libzeep used to be the XML library. It contains a full validating parser with support for XML 1.0 and 1.1 as well as a DOM API for manipulating XML based data structures in memory.

The XML part of libzeep has been split off in version 7 and libzeep now uses `libzeem <https://forge.hekkelman.net/maarten/zeem>` for the manipulation of XML.

You can use libzeep for building web applications in C++ including a web server implementation, SOAP and REST controller support and a templating engine looking suspisciously like `Thymeleaf <https://www.thymeleaf.org/>`_. Lots of the concepts used in libzeep are inspired by the Java based `Spring framework <https://spring.io/>`_.

This library contains a web server implementation. There's also code to create daemon processes and run a preforked webserver. The design follows a bit the one from Spring and so there's a HTTP server class that delegates requests to controllers. A security context class helps in limiting access to authorized users only.

Three specialized controller classes provide HTML templates, REST and SOAP services. The template language implementation attempts to be source code compatible with Thymeleaf.

The base controller class is a REST controller and maps member function calls to the HTTP URI space and translates HTTP parameters and HTTP form content into function variables and it provides transparent and automatic translation of result types into JSON.

The SOAP controller is like the REST controller, but now digests requests wrapped in SOAP envelopes, delegates them to handler functions and returns the result back wrapped in SOAP envelopes.

.. toctree::
   :maxdepth: 2
   :caption: Contents
   
   self
   lib-http
   lib-generic
   api/library_root.rst
   genindex

