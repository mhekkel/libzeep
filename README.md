[![DOI](https://zenodo.org/badge/44161414.svg)](https://zenodo.org/badge/latestdoi/44161414)
[![Documentation Status](https://readthedocs.org/projects/libzeep/badge/?version=latest)](https://libzeep.readthedocs.io/en/latest/?badge=latest)

[![github CI](https://github.com/mhekkel/libzeep/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/mhekkel/libzeep/actions)

libzeep
=======

TL;DR
-----

Libzeep is a web application framework written in C++. To see a starter project
visit the [libzeep-webapp-starter](https://github.com/mhekkel/libzeep-webapp-starter.git)
page.

About
-----

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

The current incarnation of libzeep, version 6.0, is a completely refactored
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

You also need to have installed [Howard Hinnants date library](https://github.com/HowardHinnant/date).

And, unless you are using macOS, it is recommended to install
[mrc](https://github.com/mhekkel/mrc) in order to have resources support in libzeep.

The commands to build libzeep from the command line are e.g.:

```bash
    git clone https://github.com/mhekkel/libzeep
    cd libzeep
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ctest --test-dir build
    cmake --install build
```

Creating a simple web application
---------------------------------

Create a template in xhtml first, store this as `hello.xhtml` in a directory called `docroot`:

```xml
<!DOCTYPE html SYSTEM "about:legacy-compat">
<html xmlns="http://www.w3.org/1999/xhtml"
  xmlns:z="http://www.hekkelman.com/libzeep/m2">
  <head>
    <title>Hello</title>
  </head>
  <p>Hello, <span z:text="${name ?: 'world'}"/>!</p>
</html>
```

Then create a source file called `http-server.cpp` with the following content:

```c++
#define WEBAPP_USES_RESOURCES 0

#include <zeep/http/server.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/template-processor.hpp>

class hello_controller : public zeep::http::html_controller
{
  public:
    hello_controller()
    {
        map_get("", &hello_controller::handle_index, "name");
        map_get("index.html", &hello_controller::handle_index, "name");
        map_get("hello/{name}", &hello_controller::handle_index, "name");
    }

    zeep::http::reply handle_index(const zeep::http::scope& scope,
        std::optional<std::string> user)
    {
        zeep::http::scope sub(scope);
        sub.put("name", user.value_or("world"));

        return get_template_processor().create_reply_from_template("hello.xhtml", sub);
    }
};

int main()
{
    zeep::http::server srv(std::filesystem::canonical("docroot"));

    srv.add_controller(new hello_controller());

    srv.bind("::", 8080);
    srv.run(2);

    return 0;
}
```

Create a `CMakeLists.txt` file:

```cmake
cmake_minimum_required(VERSION 3.16)

project(http-server LANGUAGES CXX)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(zeep REQUIRED)

add_executable(http-server http-server.cpp)
target_link_libraries(http-server zeep::zeep)
```

And configure and build the app:

```bash
cmake .
cmake --build .
```

And then run it:

```bash
./http-server
```

Now you can access the result using the following URL's:

* <http://localhost:8080/>
* <http://localhost:8080/index.html?name=maarten>
* <http://localhost:8080/hello/maarten>
