Generic
=======

Introduction
------------

Originally libzeep came as a single library. But if you only need the XML functionality it might not be very useful to include all the networking code for the HTTP server. And so the libraries were split into modules that can be used independently from each other. There are some files that are shared by all libraries though

The configuration file
^^^^^^^^^^^^^^^^^^^^^^

In :ref:`file_zeep_config.hpp` you can find a couple of flags that influence what parts of libzeep should be left out. The first is to enable building a :cpp:class:`zeep::http::preforked_server` class, probably only useful in a UNIX context.

The other flag allows the compilation of code that uses resources. Resources in a libzeep context are a bit different from their counterparts in MacOS and Windows. Libzeep uses *mrc* to bundle resources in an executable. Especially for small web applications this makes installation very easy at the cost of configurability. See `github pages for mrc <https://github.com/mhekkel/mrc>`_ for more information.

character array streambuf
^^^^^^^^^^^^^^^^^^^^^^^^^

Sometimes it is very convenient to have a `std::istream` reading from a `const char*` buffer, the class
:cpp:class:`zeep::char_streambuf` allows you to do just that.

.. code-block:: cpp

	auto sb = zeep::char_streambuf("Hello, world!");
	auto is = std::istream(&sb);

	std::string line;
	std::getline(is, line);

Unicode/text support
^^^^^^^^^^^^^^^^^^^^

Various simple routines used when working with UTF-8 encoded Unicode text. Routines that are so common, you really ask yourself why these are not part of the standard yet.

Serialization support
^^^^^^^^^^^^^^^^^^^^^

Originally, libzeep could only serialize XML. Later, with the addition of a JSON library with its own serialization ideas, the two were merged somewhat and the overlapping code ended up here.
