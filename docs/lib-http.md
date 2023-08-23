# HTTP

<!-- [def __http_server__ [classref zeep::http::server `zeep::http::server`]]
[def __request__ [classref zeep::http::request `request`]]
[def __reply__ [classref zeep::http::reply `reply`]]
[def __scope__ [classref zeep::http::scope `scope`]]
[def __header__ [classref zeep::http::header `header`]]
[def __html_controller__ [classref zeep::http::html_controller `html_controller`]]
[def __rest_controller__ [classref zeep::http::rest_controller `rest_controller`]]
[def __soap_controller__ [classref zeep::http::soap_controller `soap_controller`]] -->

## Introduction

The goal of libzeep is to provide a library that makes creating web applications as easy as possible. A lot of frameworks already exist to help building these interactive web apps written in languages ranging from Java to Python to more exotic stuff like Ruby and Laravel. The [Spring](https://spring.io) version stands out between these since it is well designed and offers tons of features and still is fairly easy to work with. But all of these have one flaw in common, they're not written in C++ and thus lack the raw performance.

Libzeep tries to implement some of the design patterns found in Spring. There is a very basic HTTP server class with some additional classes that help in creating daemon processes when running a UNIX or lookalike. This HTTP server class delegates requests to controller classes that each process requests in their own corner of the URI space occupied by your server.

There are three main controller classes, each targeted at a different task. The first, __html_controller__ maps requests to functions that take a __request__ and __scope__ and return a __reply__. Various routines are available to help constructing XHTML based replies based on XHTML template files. These files can contain special tags that will be processed and the values can be expressed in _expression language_.

The second controller class is the __rest_controller__. This one also maps requests to functions, but this time the parameters in the request are automatically translated into function parameters and the result of the function is converted back into JSON automatically. Named parameters can be passed in the payload of a POST request, as query parameters in a GET request or as parts of the URI path, as in ``GET /book/1234/title`` where you request the title of book number 1234.

The third controller is the __soap_controller__. Similar to the REST controller, but this time the translation is between SOAP XML messages and parameters and vice versa.

## HTTP server

Starting an HTTP server is easy. Simply create the __http_server__ object, [memberref zeep::http::server::bind `bind`] it to an address and call run specifying how many threads to run simultaneously. Something like this:

```{literalinclude} ../examples/http-server-0.cpp
:start-after: //[ most_simple_http_server
:end-before: //]
:language: c++
```

Running this code will create a server that listens to port 8080 on localhost and will return ``NOT FOUND`` for all requests. Not particularly useful of course. It is possible to derive a class from __http_server__ and implement [memberref zeep::http::server::handle_request `handle_request`]. But there is a better alternative: use controllers.

## Controllers

A __http_server__ can have one or more controllers. Each controller handles requests that have URI's that start with a specified prefix. In this example we have a controller that handles any request, since it has a prefix that is effectively the same as the root. It returns a simple reply.

```{literalinclude} ../examples/http-server-1.cpp
:start-after: //[ simple_http_server
:end-before: //]
:language: c++
```

Still a not so useful example. Fortunately there are several implementations of [classref zeep::http::controller `controller`] that we can use.

### HTML Controller

The __html_controller__ class allows you to *mount* a request handler on a URI path, the result is that this request handler, which is a method of your controller class, will be called whenever a HTTP request with a matching URI comes in.

The handler method has next to the __request__ and __reply__ parameter an additional __scope__ parameter. This scope is a kind of nested map of variable names and values. The scope is `const`, if you want to add data to the scope you should create your own sub scope and pass the original in the constructor.

A handler can of course create simple replies, just as in the previous example. But you can also use templates. Note that the constructor of __html_controller__ takes a second parameter that is called docroot. This should contain the path to the directory containing the templates.

:::{note}
The docroot parameter is ignored when you create a html controller based on resources, see section on resources further in this documentation.
:::

Our __html_controller__ indirectly inherits [classref zeep::http::template_processor `template_processor`] and this is the class that uses the `docroot` parameter. This class takes care of processing template files. It loads them and uses the registered tag processors and the `scope` to fill in the blanks and process the constructs found in the template.

```{literalinclude} ../examples/http-server-2.cpp
:language: c++
:start-after: //[ simple_http_server_2
:end-before: //]
```

This example uses the file docroot/hello.xhtml which should contain:

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

Now build and run this code, and you can access your welcome page at [If you want to see another name, use e.g. <http://localhost:8080/?name=maarten> instead.

Several remarks here.

The server object is created with a ``docroot`` parameter. That parameter tells the server to create a default [classref zeep::http::template_processor `template_processor`] for use by the __html_controller__ objects.

As you can see in the handler code, a check is made for a parameter called ``name``. When present, its value is stored in the newly created sub-scope object. The template file contains a construct in the ``<span>`` element that tests for the availability of this variable and uses the default ``'world'`` otherwise. For more information on templates see the section on [link libzeep.http.xhtml-template xhtml templates].

The path specified in `mount` is `{,index,index.html}` which is a glob pattern, this pattern can accept the following constructs:

:::{table} The accepted glob pattern constructs
:widths: auto

| path | matches |
|------|---------|
| `**/*.js` | matches `x.js`, `a/b/c.js`, etc |
| `{css,scripts}/` | matches e.g. `css/1/first.css` and `scripts/index.js` |
| `a;b;c` | matches either `a`, `b` or `c` |
:::

The way _mount_ works in the __html_controller__ class was a bit oldfashioned and inflexible. Especially compared to the __rest_controller__ class.

So version 6 of libzeep brings a new way of *mounting*, to avoid conflicts now called *mapping*. The signature of handlers is now changed to take a couple of arguments, using std::optional if they are not required. Conversion of types is done automatically. The handler also takes a __scope__ as first parameter and returns the __reply__ object.

How this works in practice is what you can see here:

```{literalinclude} ../examples/http-server-3.cpp
:start-after: //[ simple_http_server_3
:end-before: //]
:language: c++
```

## Request and Reply

An implementation of the HTTP standard will need various data types. There are [classref zeep::http::request HTTP `requests`] and [classref zeep::http::reply HTTP `replies`]. And these contain [classref zeep::http::header HTTP `headers`] but the `method` specifier (which was changed to a `std::string` in a recent update to libzeep).

The HTTP specification for __request__ and __reply__ are sufficiently similar to allow for a common [classref zeep::http::parser message parsing class]. The parser for requests supports [chunked transfer encoding](https://en.wikipedia.org/wiki/Chunked_transfer_encoding).

### request

The __request__ encapsulates what was received. The standard HTTP request contains a method, like ``GET`` or ``POST``. In this version of libzeep only a limited subset of methods are supported.

The next part is the ``uri`` that was requested.

Then we have the version, usually 1.0 or 1.1. Libzeep does not currently support anything else. When 1.1 was used, libzeep will honour the keep-alive flag.

Headers are stored in an array and can be accessed using [memberref zeep::http::request::get_header `get_header`].

Cookies stored in the headers can be accessed using [memberref zeep::http::request::get_cookie `get_cookie`].

A __request__ may also contain a payload, usually only in case of a ``POST`` or ``PUT``.

Requests can have parameters. These can be passed url-encoded in the uri, or they can be encoded in the payload using ``application/x-www-form-urlencoded`` or ``multipart/form-data`` encoding. The various [memberref zeep::http::reply::get_parameter `get_parameter`] members allow retrieving these parameters by name, optinally passing in a default value in case the parameter was not part of the request.

A special case are file parameters, these are retrieved using [memberref zeep::http::reply::get_file_parameter `get_file_parameter`]. This returns a [classref zeep::http::file_param `file_param`] struct that contains information about the uploaded file. Using the [classref zeep::char_streambuf `char_streambuf`] class you can efficiently read the contents of such a file:

```c++
zeep::file_param f = req.get_file_parameter("upoad");
zeep::char_streambuf sb(f.data, f.length);
std::istream is(&sb);
```

Many other convenience accessors are available but data is also directly accessible since this is a `struct`.

There are some functions to set data. Those are probably only useful if you write your own code to send out HTTP requests to other servers.

### reply

The __reply__ object is the object you need to fill in. Replies contain a status line, headers and optionally a payload.

There is a static member called [memberref zeep::http::reply::stock_reply `stock_reply`] that allows you to create a complete
reply from a status code and an optional message.

The [memberref zeep::http::reply::set_header `set_header`] and [memberref zeep::http::reply::set_cookie `set_cookie`] member functions take care of setting headers and cookies respectively.

The content of the payload can be set using the various [memberref zeep::http::reply::set_content `set_content`] methods. They will set the content type header according to the data passed in. If you specify a `std::istream*` as content, and the version is set to ``1.1`` then the data stream will be sent in chunked transfer-encoding.



## XHTML Template Processing

[def __template_processor__ [classref zeep::http::template_processor `template_processor`]]

Many web application frameworks provide a way of templating, write some boilerplate HTML and fill in the details at the moment a page is requested. Apart from that, a page may contains lots of external scripts, stylesheets, images and fonts. For these two tasks libzeep comes with a __template_processor__ class.

### loading

Starting with the second task just mentioned, the __template_processor__ takes a ``docroot`` parameter in its constructor. This docroot is the location on disk where files are located. But it is also possible to build libzeep with in-memory resources by using [``mrc``](https://github.com/mhekkel/mrc). Have a look at the example code for usage.

The [memberref zeep::http::basic_template_processor::load_file `load_file`] member of __template_processor__ loads a file from disk (or compiled resources), the [memberref zeep::http::basic_template_processor::file_time `file_time`] member can be used to get the file time of a file. This can be used to generate ``304 not modified`` replies instead.

The [memberref zeep::http::basic_template_processor::load_template `load_template`] member loads a template file from docroot and parses the XML contained in this file into a __document__.

### templates

Since we're using a XML parser/library to load template, they should be strict XMTML. It is possible to make these files somewhat HTML 5 like by adding the doctype

```xml
<!DOCTYPE html SYSTEM "about:legacy-compat">
```

The tags inside a template can be processed using a tag_processor. Tag processors are linked to element and attributes in the template using XML namespaces. 

The method [memberref zeep::http::basic_template_processor::create_reply_from_template `create_reply_from_template`] can be used to convert a template into a reply using the data store in a [classref zeep::http::scope `scope`].

### tag processing

Libzeep comes with two tag_processor implementations, the first [classref zeep::http::tag_processor_v1 `tag_processor_v1`] is a legacy one and should probably not be used in new code. The second, [classref zeep::http::tag_processor_v2 `tag_processor_v2`], is inspired by [@https://www.thymeleaf.org].

## Using ``el`` script

[def __json_object__ [classref zeep::json::element `json::element`]]

``el`` is the abbreviation for _Expression Language_. It is a script language that tries to be like
[Unified Expression Language](http://en.wikipedia.org/wiki/Unified_Expression_Language). libzeep comes with code to evaluate ``el`` expressions.
The language has the concept of variables and these can be created in the C++ code using the __json_object__ class.
Variables created this way are then stored in a __scope__ object and passed along to the processing code.

To give an example:

```{literalinclude} ../examples/synopsis-el-1.cpp
:language: c++
:start-after: //[ fill_scope
:end-before: //]
```

And then you can process some ``expression language`` construct like this:

```{literalinclude} ../examples/synopsis-el-1.cpp
:language: c++
:start-after: //[ evaluate_el
:end-before: //]
```

And if you then print out the result it should give you something like:

  "1: 1, 2: 2"

### ``el`` syntax

Most often you will use simple expressions:

:::{table} Simple expressions

| expression | evaluates to |
|--|--|
| `${ ... }` | variable |
| `*{ ... }` | selection variable (lookup is done in the scope of variables that were selected with ``z:select``, v2 processor only ) |
| `@{ ... }` | link URL |
| `~{ ... }` | fragment |
| `#{ ... }` | message (not supported yet in libzeep) |
:::

The language has literals:

:::{table} Literals

| expression | evaluates to |
| -- | -- |
| `'my text string'` | Text literal |
| `42` | Numeric literal |
| `3.14` | Numeric literal, note that scientific notation is not supported |
| `true` | Boolean literal |
| `null` | Null literal |
| `user greeting` | token |
:::

Text operations supported are:

:::{table} Text operations

| construct | description |
| -- | -- |
|  `'a' + ' b'` | concatenation, result is 'a b' |
| `\|hello ${user}\|` | literal substitution, if variable user contains 'scott', result is 'hello scott' |
:::

Operators:

:::{table} operators

| operators | type |
|--|--|
| `+ - * / %` | binary operators for standard arithmetic operations |
| `-` | unary operator, minus sign |
| `and or` | binary operators for standard boolean operations |
| `! not` | unary operators, negate |
| `> < >= <= gt lt ge le` | operators to compare values |
| `== != eq ne` | operators to check for equality |
| a `?` b | conditional operator: if a then return b else null |
| a `?` b `:` c | conditional operator: if a then return b else return c |
| a `?:` b | conditional operator: if a then return a else return b |
:::

When using variables, accessing their content follows the same rules as in Javascript. Arrays have a member function `length`.

A few predefined utility objects are predefined. These are `#dates`, `#numbers`, `#request` and `#security`.

```{eval-rst}

+------------------------------------------------------+------------------------------------------------------------------------------------------+
| object.method                                        | Description                                                                              |
+======================================================+==========================================================================================+
| ``#dates.format(date,format)``                       | This method takes two parameters, a preferrably ISO formatted date and a format string.  |
|                                                      | The result will be the output of `std::put_time`_.                                       |
+------------------------------------------------------+------------------------------------------------------------------------------------------+
|``#numbers.formatDecimal(number,int_digits,decimals)``| This method takes up to three parameters, a number that needs to be formatted, an        |
|                                                      | int_digits parameter that specifies the minimum number of integral digits to use and     |
|                                                      | a decimals parameter that specifies how many decimals to use.                            |
|                                                      |                                                                                          |
|                                                      | The number is formatted using the locale matching the language specified in the Accept   |
|                                                      | HTTP request header. However, if that locale is not available the default locale is used.|
|                                                      |                                                                                          |
|                                                      | Defaults for int_digits is 1 and decimals is 0.                                          |
|                                                      |                                                                                          |
|                                                      | Example output: ``${#numbers.formatDecimal(pi,2,4)}`` is **03.1415** when the locale to  |
|                                                      | use is en_US.                                                                            |
+------------------------------------------------------+------------------------------------------------------------------------------------------+
| ``#numbers.formatDiskSize(number,decimals)``         | This method takes up to two parameters, a number that needs to be formatted,             |
|                                                      | and a decimals parameter that specifies how many decimals to use.                        |
|                                                      |                                                                                          |
|                                                      | The number is divided by 1024 until it fits three int digits and the suffix is           |
|                                                      | adjusted accordingly.                                                                    |
|                                                      | Default for decimals is 0.                                                               |
|                                                      |                                                                                          |
|                                                      | Example output: ``${#numbers.formatDiskSize(20480,2)}`` is **2.00K** when the locale to  |
|                                                      | use is en_US.                                                                            |
+------------------------------------------------------+------------------------------------------------------------------------------------------+
| ``#request.getRequestURI()``                         | Returns the original URI in the HTTP request.                                            |
+------------------------------------------------------+------------------------------------------------------------------------------------------+
| ``#security.authorized()``                           | Returns whether the uses has successfully logged in.                                     |
+------------------------------------------------------+------------------------------------------------------------------------------------------+
| ``#security.username()``                             | Returns the username for the current user.                                               |
+------------------------------------------------------+------------------------------------------------------------------------------------------+
| ``#security.hasRole(role)``                          | Returns whether the uses has the role as specified by the parameter.                     |
+------------------------------------------------------+------------------------------------------------------------------------------------------+

.. _std::put_time: https://en.cppreference.com/w/cpp/io/manip/put_time
```

## tag_processor_v1

This tag_processor works on tags, mostly. As opposed to tag_processor_v2 which works on attributes mainly.
The tags are in a separate XML namespace. You can change this name space
using the `ns` parameter in the constructor, the default is ``http://www.hekkelman.com/libzeep/m1``.

There are several predefined processing tags which are summarized below. There used to be also
a way to add your own processing tags using an `add_processor` method but that has been dropped.

```{eval-rst}
+------------------+-----------------------------------------------------------------------------------------------+
| tag name         | Description and Examples                                                                      |
| (without prefix) |                                                                                               |
+==================+===============================================================================================+
| include          | Takes one parameter named `file` and replaces the tag with the processed content of this file |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |    <zeep:include file="menu.xhtml"/>                                                          |
+------------------+-----------------------------------------------------------------------------------------------+
| if               | Takes one parameter named `test` containing an `el` script. This script is evaluated and      |
|                  | if the result is not empty, zero or false, the content of the `if` tags is inserted in        |
|                  | the output. Otherwise, the content is discarded.                                              |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |    <zeep:if test="${not empty name}">                                                         |
|                  |      Hello ${name}                                                                            |
|                  |    </zeep:if>                                                                                 |
+------------------+-----------------------------------------------------------------------------------------------+
| iterate          | Takes two parameters, `collection` which contains an `el` script that evaluates to an array   |
|                  | `el::object` and a name in `var`. The content of the `iterate` tag is included for each value |
|                  | of `collection` and `var` will contain the current value.                                     |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |   <ul>                                                                                        |
|                  |     <zeep:iterate collection="${names}" var="name">                                           |
|                  |       <li>${name}</li>                                                                        |
|                  |     </zeep:iterate>                                                                           |
|                  |   </ul>                                                                                       |
+------------------+-----------------------------------------------------------------------------------------------+
| for              | Takes three parameters. The parameters `begin` and `end` should evaluate to a number. The     |
|                  | parameter `var` contains a name that will be used to hold the current value when inserting    |
|                  | the content of the `for` tag in each iteration of the for loop between `begin` and `end`.     |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |   <zeep:for begin="1" end="3" var="i">                                                        |
|                  |     ${i},                                                                                     |
|                  |   </zeep:for>                                                                                 |
+------------------+-----------------------------------------------------------------------------------------------+
| number           | Format the number in the `n` parameter using the `f` format. This is limited to the formats   |
|                  | ``'#.##0'`` and ``'#.##0B'`` for now. The first formats an integer value using thousand       |
|                  | separators, the second tries to format the integer value in a power of two multiplier         |
|                  | (kibi, mebi, etc.) with a suffix of `B`, `M`, `G`, etc.                                       |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |    <zeep:number n="1024" f="0.00#B"/>                                                         |
|                  |                                                                                               |
|                  | Will output `1K`                                                                              |
+------------------+-----------------------------------------------------------------------------------------------+
| options          | This tag will insert multiple =<option>= tags for each element in the `collection` parameter. |
|                  | This `collection` paramter can contain an array of strings or it can contain an array of      |
|                  | `el::object`. In the latter case, you can specify a `value` and                               |
|                  | `label` parameter to name the value and label fields of these objects. A `selected`           |
|                  | parameter can be used to select the current value of the options.                             |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |    <zeep:options collection="${names}" value="id" label="fullName" selected="1" />            |
+------------------+-----------------------------------------------------------------------------------------------+
| option           | Generate a single =<option>= tag with a value as specified in the `value` parameter. If       |
|                  | `selected` evaluates to the same value as `value`, the option is selected.                    |
|                  | The content of the =<option>= tag is inserted in the final tag.                               |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |    <zeep:option value="1" selected="${user}">                                                 |
|                  |      John Doe                                                                                 |
|                  |    </zeep:option>                                                                             |
+------------------+-----------------------------------------------------------------------------------------------+
| checkbox         | Create an ``<input>`` tag with type `checkbox`. The parameter `name` is used as name          |
|                  | attribute and the parameter `checked` is evaluated to see if the checkbox should be in        |
|                  | checked mode.                                                                                 |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |    <zeep:checkbox name='cb1' checked='${true}'>                                               |
|                  |      Check me                                                                                 |
|                  |    </zeep:checkbox>                                                                           |
+------------------+-----------------------------------------------------------------------------------------------+
| url              | The url processing tag creates a new variable in the current scope with the name as specified |
|                  | in the `var` parameter. It then creates a list of all original HTTP parameters for the        |
|                  | current page. You can override these parameter, and add new ones, by adding ``<param>`` tags  |
|                  | in the ``<url>`` tag.                                                                         |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |    <zeep:url var="next">                                                                      |
|                  |      <zeep:param name='page' value='${page + 1}'/>                                            |
|                  |    <zeep:url>                                                                                 |
|                  |    <a href="${next}">Next page</a>                                                            |
+------------------+-----------------------------------------------------------------------------------------------+
| embed            | This tag takes the content of the `var` parameter which should contain valid XML and puts the |
|                  | processed value in the document.                                                              |
|                  |                                                                                               |
|                  | .. code-block:: xml                                                                           |
|                  |                                                                                               |
|                  |     <zeep:embed var="&lt;em&gt;Hello, world!&lt;/em&gt;"/>                                    |
+------------------+-----------------------------------------------------------------------------------------------+
```

## tag_processor_v2

Tag processor version 2 is an implementation of the documentation for [Thymeleaf](https://www.thymeleaf.org/doc/tutorials/3.0/usingthymeleaf.html).
This documententation is a bit sparse, for now you might be better off reading the one at the Thymeleaf site.

There are some notable differences between Thymeleaf and libzeep though, libzeep does not support the concept of ``messages`` yet, so using this for localization is not going to work. Furthermore, the Thymeleaf library is written in Java and assumes all data constructs are Java object. Of course that is different in libzeep.

### tags

There is only one tag this tag processor processes, which is ``<z:block>``, for the rest this processor only processes attributes.

### attributes

Some attributes are treated special, these are listed below. For the other tags the general rule is that if the tag has the prefix for the ``v2`` namespace, the value of the attribute will be evaluated and the result will be placed in an attribute without the prefix. Possibly overwriting an already existing attribute with that name.

So, e.g. if you have

```xml
<span id="one" z:id="${id}"/>
```

and the variable ``id`` contains ``'two'`` the result will be

```xml
<span id="two"/>
```

### special attributes

```{eval-rst}

.. table:: processed attributes

   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | attribute      | remarks                                                                                                                                         +
   +================+=================================================================================================================================================+
   | ``assert``     | If the value of this attribute evaluates to `true`, an exception will be thrown.                                                                |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``attr``       | The value of this attribute is an expression consisting of one or more comma separated statements that assign a value to an attribute. e.g.     |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <img z:attr="width=${width},height=${height}"/>                                                                                              |
   |                |                                                                                                                                                 |
   |                | will result in the following when the value of the variables ``width`` and ``height`` is `100`.                                                 |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <img width="100" height="100/>                                                                                                               |
   |                |                                                                                                                                                 |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   |``classappend``,| The value is evaluated and the result is appended to the ``class`` or ``style`` attribute respectively.                                         |
   |``styleappend`` |                                                                                                                                                 |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``each``       | The expression in the value should have a name for a variable, optinally followed by a comma and the name for an iterator-info variable. Then a |
   |                | colon followed by an expression whose result after evaluation is an array.                                                                      |
   |                |                                                                                                                                                 |
   |                | The tag containing this attribute will be repeated as many times as there are elements in the array. Each copy will then be evaluated with the  |
   |                | name value set to the current value in the array.                                                                                               |
   |                |                                                                                                                                                 |
   |                | Example, consider this snippet                                                                                                                  |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <tr z:each="a, i: ${ { 'aap', 'noot', 'mies' } }">                                                                                           |
   |                |      <td z:text="${i.count}"/>                                                                                                                  |
   |                |      <td z:text="${a}"/>                                                                                                                        |
   |                |    </tr>                                                                                                                                        |
   |                |                                                                                                                                                 |
   |                | will result in                                                                                                                                  |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <tr>                                                                                                                                         |
   |                |      <td>1</td>                                                                                                                                 |
   |                |      <td>aap</td>                                                                                                                               |
   |                |    </tr>                                                                                                                                        |
   |                |    <tr>                                                                                                                                         |
   |                |      <td>2</td>                                                                                                                                 |
   |                |      <td>noot</td>                                                                                                                              |
   |                |    </tr>                                                                                                                                        |
   |                |    <tr>                                                                                                                                         |
   |                |      <td>3</td>                                                                                                                                 |
   |                |      <td>mies</td>                                                                                                                              |
   |                |    </tr>                                                                                                                                        |
   |                |                                                                                                                                                 |
   |                | The iterator-info variable can be used to get info about the current value.                                                                     |
   |                |                                                                                                                                                 |
   |                | .. table:: iterator members                                                                                                                     |
   |                |                                                                                                                                                 |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | name        | description                                        |                                                                          |
   |                |   +=============+====================================================+                                                                          |
   |                |   | ``count``   | counting number starting at one.                   |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | ``index``   | counting number starting at zero.                  |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | ``even``    | boolean indicating whether this is an even element |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | ``odd``     | boolean indicating whether this is an odd element  |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | ``size``    | size of the total array/collection                 |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | ``first``   | boolean indicating the first element               |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | ``last``    | boolean indicating the last element                |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   |                |   | ``current`` | the value of the current element                   |                                                                          |
   |                |   +-------------+----------------------------------------------------+                                                                          |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``if``,        | The attribute value is evaluated and if the result is `true` respectively `false` the containing tag is preserved, otherwise it is deleted.     |
   | ``unless``     |                                                                                                                                                 |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``include``,   | These three statements are used to pull in fragments of markup. The value of the attribute is evaluated and should contain a fragment           |
   | ``insert``,    | specification. The contents of this fragment are then copied to the destination. The three attributes differ in the following way:              |
   | ``replace``    |                                                                                                                                                 |
   |                | .. table:: variable list                                                                                                                        |
   |                |                                                                                                                                                 |
   |                |    +---------+-------------------------------------------------------------------------------+                                                  |
   |                |    | insert  | The complete fragment is inserted inside the body of the containing tag       |                                                  |
   |                |    +---------+-------------------------------------------------------------------------------+                                                  |
   |                |    | replace | The complete fragment is replaces the containing tag                          |                                                  |
   |                |    +---------+-------------------------------------------------------------------------------+                                                  |
   |                |    | include | The content of the fragment is inserted inside the body of the containing tag |                                                  |
   |                |    +---------+-------------------------------------------------------------------------------+                                                  |
   |                |                                                                                                                                                 |
   |                | Example, when the fragment is                                                                                                                   |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <span z:fragment="f">hello</span>                                                                                                            |
   |                |                                                                                                                                                 |
   |                | the following markup:                                                                                                                           |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <div z:insert="~{::f}"/>                                                                                                                     |
   |                |    <div z:replace="~{::f}"/>                                                                                                                    |
   |                |    <div z:include="~{::f}"/>                                                                                                                    |
   |                |                                                                                                                                                 |
   |                | will result in:                                                                                                                                 |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <div><span>hello</span></div>                                                                                                                |
   |                |    <span>hello</span>                                                                                                                           |
   |                |    <div>hello</div>                                                                                                                             |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``inline``     | The processor processes occurrences of \[\[ ... \]\] or \[\( ... \)\] by evaluating what is in between those brackets.                          |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <div>Hello, [[${name ?: 'world'}]]</div>                                                                                                     |
   |                |                                                                                                                                                 |
   |                | will result in (when name = 'scott'):                                                                                                           |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <div>Hello, scott</div>                                                                                                                      |
   |                |                                                                                                                                                 |
   |                | Using this attribute, you can do even more fancy things. If the value of this attribute is ``javascript``, the replacement will be valid in a   |
   |                | javascript context by properly quoting double quotes e.g. And it will process commented values properly, as in:                                 |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <script z:inline='javascript'>let x = /*[[${var}]]*/ null;</script>                                                                          |
   |                |                                                                                                                                                 |
   |                | Might result in:                                                                                                                                |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |
   |                |    <script>let x = "He said \"bla bla bla\"";</script>                                                                                          |
   |                | If the inline attribute has value ``text``, the whole body of the tag will be evaluated as ``el``.                                              |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``switch``,    | Example:                                                                                                                                        |
   | ``case``       |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |            
   |                |    <div z:switch="${user.role}">                                                                                                                | 
   |                |      <span z:case="'admin'">Admin</span>                                                                                                        |
   |                |      <span z:case="*">Some other user</span>                                                                                                    |
   |                |    </div>                                                                                                                                       |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``text``,      | The simplest, replace the body of the tag with the result of the evaluation of the content of this attribute.                                   |
   | ``utext``      |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |            
   |                |    <span z:text="${name}"/>                                                                                                                     |
   |                |                                                                                                                                                 |            
   |                | Will result in:                                                                                                                                 |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |            
   |                |    <span>scott</span>                                                                                                                           |
   |                |                                                                                                                                                 |            
   |                | The ``text`` variant will quote special characters like <, > and &. The ``utext`` variant will not, but beware, if the result is not valid XML  |
   |                | an exception is thrown.                                                                                                                         |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
   | ``with``       | Assign a value to a variable. Example:                                                                                                          |
   |                |                                                                                                                                                 |
   |                | .. code-block:: xml                                                                                                                             |
   |                |                                                                                                                                                 |            
   |                |    <div z:with="n=${name}">                                                                                                                     |
   |                |      <span z:text="${n}"/>                                                                                                                      |
   |                |    </div>                                                                                                                                       |
   +----------------+-------------------------------------------------------------------------------------------------------------------------------------------------+
```

## REST Controller

The __rest_controller__ class is similar to the __html_controller__ in that it allows you to map a request to a member function. The difference however is that the REST controller translates parameters from HTTP requests into arguments for your method and translates the result of the method back into JSON to be returned to the client. Lets walk through an example again to show how this works.

We begin our example by declaring some shopping cart objects. These are plain structs that also define a `serialize` method for use with serialization.

```{literalinclude} ../examples/rest-sample.cpp
:start-after: //[ cart_items
:end-before: //]
:language: c++
```

Now we create a REST controller that will handle the creation of a new cart and adding and deleting items from this cart. We use standard CRUD REST syntax for this, so e.g. the cart ID is part of the path in the URI for adding and deleting items.

```{literalinclude} ../examples/rest-sample.cpp
:start-after: //[ shop_rest_controller
:end-before: //]
:language: c++
```

The calls to this rest controller are in the ``scripts/shop.js`` file. Have a look at that file to see how it works. To give you an idea, this is the snippet that is called after clicking the _add_ link for an item.

```javascript
  addToCart(item) {
    const fd = new FormData();
    fd.append("name", item);
    fetch(`/cart/${this.cartID}/item`, { method: "POST", body: fd})
      .then(r => r.json())
      .then(order => this.updateOrder(order))
      .catch(err => {
        console.log(err);
        alert(`Failed to add ${item} to cart`);
      });
  }
```

The page, script and stylesheet are served by a __html_controller__.

```{literalinclude} ../examples/rest-sample.cpp
:start-after: //[ shop_html_controller
:end-before: //]
:language: c++

And tie everything together.

```{literalinclude} ../examples/rest-sample.cpp
:start-after: //[ shop_main
:end-before: //]
:language: c++
```

## REST Controller (CRUD)

The previous example is a rough example on how to use the __rest_controller__, it assumes you pass in the parameters using form data or query parameters. There's another approach, that is more elegant and easier for the developer: create a [CRUD](https://en.wikipedia.org/wiki/Create,_read,_update_and_delete) interface and pass the data encoded in JSON format.

To make this work, we rework the previous example. The data items ``Cart`` and ``Item`` remain the same, they already have a `serialize` method. The real difference is in the JavaScript code, in the previous example all work was done by the server, the JavaScript only called separate methods to fill the items list and the server responded with the new Cart content. In this example however, management of the cart content is done by the JavaScript and the updated cart is then sent using a ``PUT`` to the server.

The server therefore looks like this:

```{literalinclude} ../examples/rest-sample-2.cpp
:language: c++
:start-after: //[ shop_rest_controller_2
:end-before: //]
```

Some ceveats: this works probably only well if you have a single ``JSON`` (or compatible) data type as main parameter and optionally one or more path parameters. The request should also have ``content-type`` equal to ``application/json`` to work properly.

## SOAP Controller

Creating SOAP controllers is also easy. But that will have to wait a bit.

## Error handling

[def __error_handler__ [classref zeep::http::error_handler `error_handler`]]

During the processing of a request, an error may occur, often by throwing an std::exception. The default __error_handler__ class takes care of catching exceptions and turn these in sensible error replies. A default error_handler is created in the __http_server__ constructor and this one uses the default ``error.xhtml`` template in ``docroot``, or a built in template if that file does not exist.

You can derive your own error handler from __error_handler__ and implement a ``create_error_reply`` member to handle some errors differently. The error handlers will be called in the reverse order of being added allowing you to override default behaviour.

## Security

<!-- [def __security_context__ [classref zeep::http::security_context `security_context`]]
[def __user_service__ [classref zeep::http::user_service `user_service`]]
[def __user_details__ [classref zeep::http::user_details `user_details`]]
[def __simple_user_service__ [classref zeep::http::simple_user_service `simple_user_service`]] -->

In a web application it is often required to limit access to certain URI's to authorized users. To facilitate this in libzeep, the __http_server__ object can use a __security_context__. The __security_context__ itself uses a __user_service__ to provide __user_details__ structs containing the actual data for a user.

The __user_details__ structure contains the ``username``, encrypted ``password`` and a list of ``roles`` this user can play. The roles are simple text strings and should preferrably be very short, like ``'USER'`` or ``'ADMIN'``. 

The __user_service__ class returns __user_details__ based on a ``username`` via the pure virtual method [memberref zeep::http::user_service::load_user `load_user`]. A __simple_user_service__ class is available to create very simple user services based on static data. In a real world application you should implement your own user_service and store user information in e.g. a database.

### example

Let us walk through an example of an application using security. This web application will have two pages, a landing page at the URI =/= (but also at ``/index`` and ``/index.html``) and an admin page at ``/admin``. The latter of course will only be accessible by our admin who is called _scott_ and he uses the password _tiger_.

[note The code for all example code used in this documentation can be found in the doc/examples subdirectory of the libzeep distribution. ]

First start by writing some template code. For this example we will have a common menu template and two templates for the two pages respectively. The interesting part of the menu template is this: 

```xml
<div z:fragment="menu" class="w3-bar w3-border w3-light-grey">
  <a href="/" class="w3-bar-item w3-button">Home</a>
  <a href="/admin" class="w3-bar-item w3-button"
      z:classappend="${#security.hasRole('ADMIN') ? '' : 'w3-text-grey w3-hover-none w3-hover-text-grey'}">Admin</a>
  <a z:if="${not #security.authorized()}" href="/login" class="w3-bar-item w3-button w3-green w3-right">Login</a>
  <a z:if="${#security.authorized()}" href="/logout" class="w3-bar-item w3-button w3-green w3-right">Logout</a>
</div> 
```

We're using [W3.CSS](https://www.w3schools.com/w3css/default.asp) as CSS library, albeit we have stored a copy in our own docroot. The two last links in this navigation bar have the ``z:if``"..."= argument checking whether the current user is authorized. These attributes help in select which of the two will be visible, login or logout, based on the current authentication state. The ``#security`` class in our ``el`` library has two more methods called ``username`` and ``hasRole``. The last one returns true when a user has the role asked for.

Next we define a simple __html_controller__ that handles the two pages and also serves stylesheets and scripts.

```{literalinclude} ../examples/security-sample.cpp
:language: c++
:start-after: //[ sample_security_controller
:end-before: //]
```

Nothing fancy here, just a simple controller returning pages based on templates. In the template we add a salutation:

```xml
<p>Hello, <span z:text="${#security.username() ?: 'world'}"></span>!</p>
```

And here we see the call to ``#security.username()``. Note also the use of the elvis operator, if username is not set, 'world' is used instead.

Now, in the `main` of our application we first create a __user_service__.

```{literalinclude} ../examples/security-sample.cpp
:language: c++
:start-after: //[ create_user_service
:end-before: //]
```

We use the __simple_user_service__ class and provide a static list of users. The __user_service__ should return __user_details__ with an encrypted password and therefore we encrypt the plain text password here. Normally you would store this password encrypted of course. For encrypting password we use the [classref zeep::http::pbkdf2_sha256_password_encoder `pbkdf2_sha256_password_encoder`] class. You can add other password encoders based on other algorithms like bcrypt but you then have to register these yourself using [memberref zeep::http::security_context::register_password_encoder `security_context::register_password_encoder`];

Now we can create the security context. This context will be passed to the __http_server__ class as a pointer and the __http_server__ will take ownership.

```{literalinclude} ../examples/security-sample.cpp
:language: c++
:start-after: //[ create_security_context
:end-before: //]
```

The secret passed to the security_context is used in creating signatures for the [JWT token](https://en.wikipedia.org/wiki/JSON_Web_Token). If you store this secret, the sessions of your users will persist reboots of the server. But in this case we create a new secret after each launch and thus the tokens will expire.

Now we add access rules.

```{literalinclude} ../examples/security-sample.cpp
:language: c++
:start-after: //[ add_access_rules
:end-before: //]
```

A rule specifies for a glob pattern which users can access it based on the roles these users have. If the list of roles is empty, this means all users should be able to access this URI. When a request is received, the rules are checked in order of which they were added. The first match will be used to check the roles.

In this example ``/admin`` is only accessible by users having role _ADMIN_. All other URI's are allowed by everyone. Note that we could have also created the __security_context__ with the parameter defaultAccessAllowed as `true`, we then would not have needed that last rule.

And now we can create the __http_server__, add some controller and start it.

```{literalinclude} ../examples/security-sample.cpp
:language: c++
:start-after: //[ start_server
:end-before: //]
```

Note that we add the default [classref zeep::http::login_controller `login_controller`]. This controller takes care of handling ``/login`` and ``/logout`` requests. It will also add the required rule for ``/login`` to the __security_context__ using `add_rule("/login", {});` since otherwise the login page would not be reachable. Make sure you do not add your own rules that prevent access to this page.

And that's all. You can now start this server and see that you can access ``/`` and ``/login`` without any problem but ``/admin`` will give you an authentication error. When you login using the credentials ``scott/tiger`` you can access the ``/admin`` page and you can now also click the Logout button.

## CSRF protection

The __security_context__ class contains some rudimentary support for protecting against [CSRF attacks](https://en.wikipedia.org/wiki/Cross-site_request_forgery). The way it works is that the server class add a special `csrf-token` cookie to a session. This cookie is stored in the browser with the flags `SameSite=Lax` and `HttpOnly` which makes it unavailable to malicious scripts that might have been injected in your pages. If a value has been set to this cookie and the __security_context__ class has the `set_validate_csrf` flag set, each `POST` or `SUBMIT` will be checked if there is a `_csrf` parameter and this should contain the same value as the `csrf-token` cookie.

So, to use this functionality, call the [memberref zeep::http::security_context::set_validate_csrf `set_validate_csrf`] method on a newly created __security_context__ instance. Next you should make sure each form or `POST` call should contain a `_csrf` parameter with the value stored in the session cookie `csrf-token`. This value can be obtained by calling [memberref zeep::http::context::get_csrf_token `zeep::http::context::get_csrf_token`].

## Cryptographic routines

A limited number of cryptographic routines are available in `<zeep/crypto.hpp>`. These can be divided in the following categories:

### Encoding

The functions encode and decode functions take a std::string and return the encoded or decoded content. There are three encoding schemes, `hex`, `base64` and `base64url`. The latter is simply `base64` but with a different characterset and without the trailing `=` characters allowing their use in a URL.

### Hashing

There are three hash algorithm implementations. These are `md5`, `sha1` and `sha256`. All of them take a std::string and return a std::string with the resulting hash. Note that these strings are not human readable and may contain null characters. Therefore you need the encoding routines to convert a hash into something you can print to the screen e.g.

### HMac

Hashed message authentication codes can be calculated using the available hash functions. Again, these functions take std::string parameters for the message and the key. The result is again a binary std::string.

### Key derivation

Two key derivation routines are on offer, both of them PBKDF2, one using HMAC SHA1 and the other HMAC SHA256.
