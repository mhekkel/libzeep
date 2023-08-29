JSON
====

Introduction
------------

The web application code for libzeep contained an implementation of `expression language <https://en.wikipedia.org/wiki/Unified_Expression_Language>`_, a simple language used in the HTML templates. The variables in this language look and act a bit like objects in JavaScript. And then REST support was added and more support for `JSON <https://www.json.org/>`_ was needed.

For C++ there is already a very good JSON library available from `Niels Lohmann <https://github.com/nlohmann/json>`_. I tried to use this initially, but it didn't fit well into the rest of libzeep. Still I liked the way it worked and so I tried to create something that is source code compatible, however fancy stuff like JSON Pointer and JSON Patch are not available (yet).

This library just provides code to parse and write JSON. It stores the data in an efficient way, allows easy manipulation and also can use the same mechanism of serialization as the XML library.

The JSON element
----------------

The main data structure in this library is :cpp:class:`zeep::json::element`_, this is the representation of a JSON object and thus can contain various types of data. See this synopsis on how to use it.

.. literalinclude:: ../examples/synopsis-json.cpp
	:language: c++
	:lines: 57-94

There is also support for *enum*s, see the following example. The idea is, you call the :cpp:func:`zeep::value_serializer<Enum>::init` once to initialize the global mapping of enum values to strings. The name parameter is optional, but required if you use this serializer also in a SOAP controller.

.. literalinclude:: ../examples/synopsis-json.cpp
	:language: c++
	:lines: 41-50

STL-like interface
------------------

The __json_element__ class acts as an STL container, see the class reference for more information. But to give you an idea:

.. literalinclude:: ../examples/synopsis-json.cpp
	:language: c++
	:lines: 18-31

