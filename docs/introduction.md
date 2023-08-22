Introduction
============

Libzeep started as a spin-off of [MRS](https://mrs.cmbi.umcn.nl) which is a tool to index and search large text-based bioinformatics databanks. The code that generates a SOAP server in compile time in MRS was needed in another project and this is how libzeep started. BTW, zeep is the dutch word for soap.

One of the major parts of libzeep used to be the XML library. It contains a full validating parser with support for XML 1.0 and 1.1 as well as a DOM API for manipulating XML based data structures in memory.

The current implementation of libzeep goes much further. It is by now a swissarmy knife for building web applications in C++ including a web server implementation, a JSON library, SOAP and REST controller support and a templating engine looking suspisciously like [Thymeleaf](https://www.thymeleaf.org/). Lots of the concepts used in libzeep are inspired by the Java based [Spring framework](https://spring.io/).

In version 5.0 the library has been split up in thre sub libraries, each targeted at different tasks.

xml
---

A feature complete XML library containing a validating parser as well as a modern C++ API for the data structures. It also supports serializing data structures using a boost-like serialization interface.

json
----

This is an implementation of a JSON library. I've attempted to make it source code compatible with the very good [JSON library by Niels Lohmann](https://github.com/nlohmann/json). There are some major differences though, my library has a very different parser as well as support for serializing using the same technique as libzeep-xml. On the other hand mine lacks many of the advanced cool features found in Niels library.

http
----

This library contains a web server implementation. There's also code to create daemon processes and run a preforked webserver. The design follows a bit the one from Spring and so there's a HTTP server class that delegates requests to controllers. A security context class helps in limiting access to authorized users only.

Three specialized controller classes provide HTML templates, REST and SOAP services. The template language implementation attempts to be source code compatible with Thymeleaf.

The REST controller maps member function calls to the HTTP URI space and translates HTTP parameters and HTTP form content into function variables and it provides transparent and automatic translation of result types into JSON.

The SOAP controller is like the REST controller, but now digests requests wrapped in SOAP envelopes, delegates them to handler functions and returns the result back wrapped in SOAP envelopes.
