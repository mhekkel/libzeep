XML
===

Introduction
------------

The core of this library is a validating XML parser with DTD processing and all. On top of this are implemented an API for manipulating XML data in a DOM like fashion and a serialization API. As a bonus there's also an XPath implementation, albeit this is limited to XPath 1.0.

The DOM API
-----------

libzeep uses a modern C++ way of accessing and manipulating data. To give an idea have a look at the following code.

.. literalinclude:: ../examples/synopsis-xml.cpp
	:language: c++
	:start-after: //[ synopsis_xml_main
	:end-before: //]

XML nodes
^^^^^^^^^

The class :cpp:class:`zeep::xml::node` is the base class for all classes in the DOM API. The class is not copy constructable and subclasses use move semantics to offer a simple API while still being memory and performance efficient. Nodes can have siblings and a parent but no children.

The class :cpp:class:`zeep::xml::element` is the main class, it implements a full XML node with child nodes and attributes. The children are stored as a linked list and same goes for the attributes.

The class :cpp:class:`zeep::xml::text` contains the text between XML elements. A :cpp:class:`zeep::xml::cdata` class is derived from :cpp:class:`zeep::xml::text` and other possible child nodes for an XML element are :cpp:class:`zeep::xml::processing_instruction` and :cpp:class:`zeep::xml::comment`.

XML elements also contain attributes, stored in the :cpp:class:`zeep::xml::attribute` class. Namespace information is stored in these attributes as well. Attributes support structured binding, so the following works:

.. code-block:: cpp

	zeep::xml::attribute a("x", "1");
	auto& [name, value] = a; // name == "x", value == "1"

Input and output
--------------------------------------

The class :cpp:class:`zeep::xml::document` derives from :cpp:class:`zeep::xml::element` can load from and write to files.

streaming I/O
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use std::iostream to read and write :cpp:class:`zeep::xml::document` objects. Reading is as simple as:

.. code-block:: cpp

	zeep::xml::document doc;
	std::cin >> doc;

Writing is just as simple. A warning though, round trip fidelity is not guaranteed. There are a few issues with that. First of all, the default is to replace CDATA sections in a file with their content. If this is not the desired behaviour you can call :cpp::func:`zeep::xml::document::set_preserve_cdata` with argument `true`.

Another issue is that text nodes containing only white space are present in documents read from disk while these are absent by default in documents created on the fly. When writing out XML using `iostream` you can specify to wrap and indent a document. But if the document was read in, the result will have extraneous spacing.

Specifying indentation is BTW done like this:

.. code-block:: cpp

	std::cout << std::setw(2) << doc;

That will indent with two spaces for each level.

validation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This will not validate the XML using the DTD by default. If you do want to validate and process the DTD, you have to specify where to find this DTD and other external entities. You can either use :cpp::func:`zeep::xml::document::set_base_dir` or you can specify an entity_loader using :cpp::func:`zeep::xml::document::set_entity_loader`

As an example, take the following DTD file

.. code-block:: xml

	<!ELEMENT foo (bar)>
	<!ELEMENT bar (#PCDATA)>
	<!ENTITY hello "Hello, world!">

And an XML document containing

.. code-block:: xml

	<?xml version="1.0" encoding="UTF-8" ?>
	<!DOCTYPE foo SYSTEM "sample.dtd">
	<foo>
	<bar>&hello;</bar>
	</foo>

When we want to see the `&hello;` entity replaced with `'Hello, world!'` as specified in the DTD, we need to provide a way to load this DTD. To do this, look at the following code. Of course, in this example a simple call to :cpp::func:`zeep::xml::document::set_base_dir` would have been sufficient.

.. literalinclude:: ../examples/validating-xml-sample.cpp
	:language: c++
	:start-after: //[ xml_validation_sample
	:end-before: //]

Serialization
--------------------------------------

An alternative way to read/write XML files is using serialization. To do this, we first construct a structure called Person. We add a templated function to this struct just like in `boost::serialize` and then we can read the file.

.. literalinclude:: ../examples/serialize-xml.cpp
	:language: c++
	:start-after: //[ serialisation
	:end-before: //]

attributes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Suppose you want to serialize a value into a XML attribute, you would have to replace `zeep::make_nvp` with `zeep::make_attribute_nvp`.

custom types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

What happens during serialization is deconstruction of structured data types into parts that can be converted into text strings. For this final conversion there are __value_serializer__ helper classes. __value_serializer__ is a template and specializations for the default types are given in `<zeep/value_serializer.hpp>`. You can create your own specializations for this class for custom data types, look at the one for `std::chrono::system_clock::time_point` for inspiration.

enums
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For conversion of enum's you can use the __value_serializer__ specialization for enums:

.. code-block:: cpp

	enum class MyEnum { FOO, BAR };
	__value_serializer__<MyEnum>::instance()
	("foo", MyEnum::FOO)
	("bar", MyEnum::BAR);

There's also a new interface, somewhat more intuitive from a modern C++ viewpoint:

.. literalinclude:: ../examples/synopsis-json.cpp
	:language: c++
	:start-after: //[ enum_support
	:end-before: //]

XPath 1.0
--------------------------------------

Libzeep comes with a [XPath 1.0](http://www.w3.org/TR/xpath/) implementation. You can use this to locate elements in a DOM tree easily. For a complete description of the XPath specification you should read the documentation at e.g. http://www.w3.org/TR/xpath/ or https://www.w3schools.com/xml/xpath_intro.asp.

The way it works in libzeep is that you can call `find()` on an :cpp:class:`zeep::xml::element` object and it will return a `zeep::xml::element_set` object which is actually a `std::list` of :cpp:class:`zeep::xml::element` pointers of the elements that conform to the specification in XPath passed as parameter to `find()`. An alternative method `find_first()` can be used to return only the first element.

An example where we look for the first person in our test file with the lastname Jones:

.. code-block:: cpp

	zeep::xml::element* jones = doc.child()->find_first("//person[lastname='Jones']");

variables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

XPath constructs can reference variables. As an example, suppose you need to find nodes in a special XML Namespace but you do not want to find out what the prefix of this Namespace is, you could do something like this:

.. literalinclude:: ../examples/xpath-sample.cpp
	:language: c++
	:start-after: //[ xpath_sample
	:end-before: //]

.. note::

	Please note that the evaluation of an XPath returns pointers to XML nodes. Of course these are only valid as long as you do not modify the the document in which they are contained.
