Libzeep was developed to make it easy to create SOAP servers. And since working with SOAP means working with XML and no decent C++ XML library existed on my radar I've created a full XML library as well.

The XML part of libzeep consists of a validating parser, a DOM(-like) node implementation, an XPath search engine and a XML writer/formatter. The validation works based on DOCTYPE definitions, XML schema support will be added in a later release.

The performance of the parser is not optimal yet although it performs very decently. If speed is critical and you really need that few percent saving you can choose to use expat as a parser instead.

Please note that libzeep aims to provide a fully compliant XML processor as specified by the W3 organisation (see: http://www.w3.org/TR/xml ). This means it is as strict as the standard requires and it stops processing a file when a validation of the well-formedness is encountered, or when a document appears to be invalid when it is in validating mode. Error reporting is done in this case, although I admit that error reporting should be improved.

The SOAP server part of libzeep makes it very easy to create a SOAP server software in C++. You use it to export a C++ object's methods as SOAP actions. The library generates a WSDL on-the-fly for the exported actions and it also has a REST style interface.

libzeep requires the Boost libraries and currently requires at least version 1.36 of Boost since it uses the new asio library for network I/O. The current version of libzeep has been tested with boost 1.39 and newer only.

To use libzeep, you have to edit the makefile and make sure the paths to your installation of boost libraries are correct. After this you simply type 'make zeep-test' and a 'zeep-test' executable is build. You can also cd into the tests directory and build the two test applications called xpath-test and parser-test. For Windows users there's a VC solution file in the msvc directory.

XML Library -- usage

Using the XML library of libzeep is fairly trivial. The first class you use is the zeep::xml::document class. You can use this class to read XML files and write them out again. Reading and writing is strictly done using stl iostreams. Make sure you open these streams in binary mode, random parsing errors will occur if you don't when running in Windows.

	#include <fstream>
	#include "zeep/xml/document.hpp"
	
	using namespace std;
	using namespace zeep;
	
	...
	
	xml::document doc;
	doc.set_validating(true);			// validation is off by default
	ifstream file("/...", ios::binary); // avoid CRLF translation
	file >> doc;
	
Now that you have a document, you can walk its content which is organised in nodes. There are several nodes classes, the most interesting for most is xml::element. These elements can have children, some of which are also elements.

Internally the nodes are stored as linked lists. However, to conform to STL coding practices, xml::element can used like a container. The iterator of xml::element (which it inherits from its base class xml::container) only returns child xml::element objects skipping over comments and processing instructions.

So, to iterate over all elements directly under the first element of a document, we do something like this:

	xml::element& first = *doc.child();
	for (xml::document::iterator e = first.begin(); e != first.end(); ++e)
		cout << e.name() << endl;

Likewise you can iterate over the attributes of an xml::element, like this:

	for (xml::element::attribute_iterator a = e.attr_begin(); a != e.attr_end(); ++a)
		cout << a->name() << endl;

More often you're interested in a specific element among many others. Now you can recursively iterate the tree until you've found what you're looking for, but it is way easier to use xpaths in that case. Let say you need the element 'book' having an attribute 'title' with value 'Du côté de chez Swann', you could do this:

	xml::element* book = doc.find("//book[@title='Du côté de chez Swann']");

You can access the attributes by name:

	assert(book->get_attribute("author") == "Proust");

And the content, contained in the text nodes of an element:
	
	cout << book->content() << endl;

And writing out an XML file again can be done by writing an xml::document:

	cout << doc;
	
Or by using xml::writer directly.

libzeep has XML Namespace support. The qname method of the nodes returns a qualified name, that is the namespace prefix, a colon and the localname contatenated. (Something like 'ns:book'). The method name() returns the qname() with its prefix stripped off.

SOAP Server -- usage

Have a look at the zeep-test.cpp file to see how to create a server. This
example server is not entirely trivial since it has three exported methods
that each take another set of parameters.

When you run this sample server, it listens to port 10333 on your localhost.
You can access the wsdl by pointing your browser at:

http://localhost:10333/wsdl

and to access e.g. the Count method of this server from the REST interface
you can browse to:

http://localhost:10333/rest/Count/db/sprot/booleanquery/os:human

As you can see, parameters and values are passed in the URL, order is not
important and multiple id/value pairs can be specified for input parameters
that allow more than one value.

The steps to create a server are:

Create a new server object that derives from soap::server. The constructor
of this object should call the inherited constructor passing it the
namespace and the service name for this new SOAP server as well as the
internet address and port to listen to.

Inside the constructor of your new server object you have to register the
methods of the server you want to export. These methods can take any number
of input arguments and only one output parameter which is the last parameter
of the method. The result of these methods should be void.

Please note that if the method's last (output) parameter is a struct, then
the fields of this struct will be moved up in the Response message of the
action in the WSDL. To the outside world this method will look like it has
multiple output parameters. This was done to be compatible with another
popular SOAP tool but the result may be a bit confusing at times.

To register the methods you have to call the inherited 'register_action'
method which takes four parameters:

- the name of the action as it is published
- the pointer for your server object, usually it is 'this'.
- a pointer to the method of your server object you want to export
- an array of pointers to the exported names for each of the parameters
	of the exported method/action. The size of this array should be exactly
	as long as the arity of your method. You will get a compilation error
	if it isn't.

If you export enum parameters, you add the names for all possible values of 
your enumerated type by using the SOAP_XML_ADD_ENUM macro. The first parameter
should be the name of the enum type, the second the value identifier.

If you have structured types you want to export, you have to do two things.
First of all the structure needs to be able to serialize itself. You do this
by adding the method serialize to each struct. For a struct consisting of two 
fields, db and id, you specify:

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(db)
		   & BOOST_SERIALIZATION_NVP(id);
	}

The next thing you need for each struct is to set its exported name using the
SOAP_XML_SET_STRUCT_NAME macro.

And that's it. The moment the constructor is done, your server is ready to
run. You can start it by calling the 'run' method, normally you do this from
a new thread. The servers will start listening to the address and port you
specified. Beware though that the server is multithreaded and so your exported
methods should be reentrant. The number of threads the server will use can be
specified in the constructor of the soap::server base class.

If your server is behind a reverse proxy, you set the actual location in the
WSDL from which it is accessible by calling the server's set_location method.

Inside your server method you have access to the ostream object used to write
out log entries by using the inherited log() member function.

That's it.

This is a first release, please send all the problems and/or bugs you encounter
to: m.hekkelman@cmbi.ru.nl

-maarten hekkelman
