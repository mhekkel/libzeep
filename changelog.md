<!--
SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
SPDX-License-Identifier: BSL-1.0
-->

Version 8.4.0
- Security: added constant-time string comparison to prevent timing attacks
  in CSRF and cookie validation
- Security: fixed data race on m_new_connection in server (close acceptor
  before resetting)
- Security: fixed TOCTOU race condition in daemon when probing binding
  to privileged ports
- Security: prevented log injection via unescaped user-supplied strings
- Security: prevented negative array index in request header parsing
- Security: path traversal fix in request handler
- Fixed chunked transfer encoding parser (inverted chunk size check)
- Fixed signed integer overflow (undefined behavior) in el::object
  arithmetic operators (+, -, *, /, %)
- Exception info leaked from SOAP handler via ex.what() into reply body
- Added IPv6 support in TLS server configuration
- Daemon directory creation failures now throw std::system_error instead
  of logging and continuing
- Conditionally use std::flat_map with fallback to std::map when
  __cpp_lib_flat_map is not available
- Added fuzz tests for multipart form data, JSON overflow, and EL
  expression language overflow edge cases
- Added chunked transfer encoding tests (28 cases) to message parser
- Many minor correctness and robustness fixes across the codebase

Version 8.3.0
- Fixed erasing elements from an el::object: iterator-based erase now
  compiles (missing typename on dependent iterator types) and returns a
  consistent iterator when a scalar value is reset to null
- Added tests for all el::object::erase overloads

Version 8.2.0
- Refactored internal storage of el::object to use std::variant
- Accessing a missing key or an out-of-range array index on a const
  el::object now returns a null object instead of throwing
- Modernized el::object constructors
- Proper implementation of status_code
- Fix for building with Boost.asio
- Increased required version of libzeem
- Added more el::object tests

Version 8.1.0
- Added fuzz tests for HTTP parser, EL parser, and JSON deserializer
- Updated to 100% Doxygen coverage across all public headers
- Added noexcept to numerous getter, setter, and query functions
- Moved internal headers (format, glob, signals) into src/detail/
- Fixed infinite loop in JSON lexer when encountering EOF in exponent states
- Added exponent overflow guard in JSON parser (capped at double::max_exponent10)

Version 8.0.1
- Fix generating documentation

Version 8.0.0
- Changed status_type to an enum class (breaking API change)
- Removed preforked_server (breaking API change)
- Removed std::regex from JWT parsing, replaced with string splitting
- Replaced std::localtime with std::chrono::zoned_time
- Replaced exit() calls in daemon with throwing exceptions
- Revived SOAP support and tests
- Security: constant-time string comparison to prevent timing attacks
- Security: Secure cookie flag is now unconditional (no longer gated behind NDEBUG)
- Security: added Secure attribute to CSRF cookie
- Changed to_buffers() from thread_local static to mutable member
- Replaced raw pointers with std::unique_ptr throughout
- General code modernization

Version 7.4.0
- Added HTTP client code, simple fetch uri 
- Requires OpenSSL from now on

Version 7.3.2
- Revive daemon-test example application

Version 7.3.1
- Fix regression in controller::set_reply for files.
- Fix login controller behaviour on invalid password

Version 7.3.0
- Using zeem instead of mxml.
- Fixing many warnings and a couple of issues found with clang-tidy
- Fixed the writing of log files.

Version 7.2.0
- zeep::http::controller callbacks (for REST calls) can now
  have a zeep::http::scope as first parameter.
- Added methods to el::object (nullptr constructor and back/front)

Version 7.1.0
- Fix huge memory leak

Version 7.0.5
- Removed dependency on howard hinnants date library

Version 7.0.4
- Dependabot updates
- Using date with system time zone db
- Fix for building with cmake 4
- Remove forced dependency on nlohmann
- Updated documentation

Version 7.0.3
- Do not catch exception in controller to allow error handlers to
  properly handle the exception.
- Fix the reload option of daemon

Version 7.0.2
- Fix various deamon and log file related problems
- Fix serialization of enums

Version 7.0.1
- Removed double move of string

Version 7.0.0
- Complete rewrite
- Removed xml code, now depends on libzeem
- Refactored json code into el::object (el = expression language)
- Moved legacy html_controller code into a new sub class html_controller_v1
- Removed rest_controller, code is now in the base class controller.
- Many small API changes

Version 6.1.1
- Fixed copyright on named characters files

Version 6.1.0
- Better login handling
- Do not specify BOOST_ASIO_STANDALONE by default

Version 6.0.16
- Fix the reload of daemons without preforking

Version 6.0.15
- Changed the way parsers convert a string to a float,
  now use std::from_chars for a final conversion to
  avoid rounding errors.
- Fix test on macOS (no /proc filesystem there)
- Removed using codecvt_utf8 since it is deprecated
- Removed asio::ip::tcp::resolver::query usage
- Refactored daemon to no longer expose code that
  requires POSIX calls like fork/exec on Windows

Version 6.0.14
- Fix URI parser, some paths were not absolute
- Added variant of http::daemon with forking a daemon but
  no preforked children. Simply a single instance with 
  multiple threads.

Version 6.0.13
- Flush access log after each request. (replacing all
  instances of std::endl was a bit too drastic).
- Better handling of not_found
- Don't test when included in other project
- Fix redirects after login/logout (this time for real, I hope)

Version 6.0.12
- Catch URI parse error in connection

Version 6.0.11
- Builds on macOS again, I hope

Version 6.0.10
- Fix html_controller and rest_controller to pass
  path parameters decoded.
- No longer use the date library to write out localised date/time formats
  since the installed date library might contain ONLY_C_LOCALE defined.
- Do not read PID file when running the foreground
- Renamed the cmake config files for libzeep from CamelCase to kebab-case.
  The install rules should remove older config files.

Version 6.0.9
- Fix writing encoded path segments for URI's

Version 6.0.8
- Security fix: redirect to relative URI's only on login
- Added a new HTTP status code: 422 Unprocessable Entity

Version 6.0.7
- various cmake related fixes
- new version string module

Version 6.0.6
- Dropped support for GNU autotools, pkgconfig

Version 6.0.5
- Fix SONAME (should have been updated to 6 of course)
- Changed code in format to no longer use std::codecvt_utf8
- support for building with stand alone ASIO

Version 6.0.4.1
- Do not try to build examples, that only works after installing

Version 6.0.4
- Fix message parser to accept HTTP messages without a
  content-length but with a content-type header.
- Include <cstdint> at more locations
- Include version string code (https://forge.hekkelman.net/maarten/version-string.cmake)

Version 6.0.3
- Fixes in login controller logic. Again.

Version 6.0.2
- When processing tags in a HTML5 environment, replace CDATA
  sections with plain text. CDATA is not supported in HTML5.
- Better resource linking

Version 6.0.1
- Fixed some issues in serializing and detection templates
  to enable serializing std::optional<time_point>.
- Added option to timeout JWT access tokens
- handle_file of template-processor now uses chunked transfer encoding
- Avoid crash when loading a non-regular file
- complete rewrite of uri class
- Fix formatDecimal for negative numbers

Version 6.0.0
- Dropped boost::date_time and other boost libraries
- Fix daemon::reload
- New html_controller routines that mimic the rest_controller mapping
- Added access control object, for CORS handling
- Changed serialising of std::chrono time_point values.
- Redesigned login_controller

Version 5.1.8
- Fix bug in parsing binary multipart/form-data parameters

Version 5.1.7
- Fix dependency on std::filesystem library

Version 5.1.6
- Fix the visibility of types in zeep::json::detail::iterator_impl
- Reintroduced resolving of bind addresses, using "localhost" is
  easier than only numerical addresses.
- Return correct status code in case of catching an exception
  in rest controllers.
- Fix dependency in .cmake config file for Threads
- Generate config.hpp file.

Version 5.1.5
- update zeepConfig.cmake to include required link file
- fix infinite loop in processing incorrect :inline constructs

Version 5.1.4
- Update cmakefile to work more reliably

Version 5.1.3
- Update SONAME to 5.1
- Create reproducible builds of documentation (and thus whole package)

Version 5.1.2
- Fix glob code to match empty path specifications for controllers
- Change CMakeLists file to generate only shared or static libs,
  but not both
- Generate pkgconfig file again

Version 5.1.1
- Removed uriparser again. URI implementation is now regex based.
- Replaced GNU configure with cmake

Version 5.1.0
- Added base32 encoding/decoding
- Various REST controller fixes, mainly in accepting parameters
- The library is now always compiled with PIC
- Requred boost version is now 71
- Ignore SIGCHLD in foreground mode, signals are now handled by 
  cross platform implementation
- reintroduced a Windows version
- Fixed a couple of security issues, all caused by incorrectly
  parsing uri's. Switched to using liburiparser for now.

Version 5.0.2
- Add support for building shared libraries
- Decoupled example code from rest, should now be build after installation,
  or use the STAGE=1 option to make.
- rest controller can now return a reply object, adding flexibility

Version 5.0.1

- Update makefile to include changes made for the Debian package
- Fix writing HTML, proper empty elements
- Added some workarounds to build on macOS
- Fixed endianness issue in sha implementation

Version 5.0.0

- Total rewrite of about everything
- Controllers are now the main handlers of requests, three major
  variants for HTML, REST and SOAP.
- Implemented some cryptographic routines in order to drop
  dependency on libcrypto++
- Redesigned authentication, dropped HTTP digest and opted for JWT,
  added security_context class for managing all of this
- Code now requires a c++17 compiler
- Lots of test code added
- Added some real world examples
- Tested with boost 1.65.1 up to 1.73
- Refactored request, it is now a class and credentials are
  always stored if a valid access-token was detected.
- A bunch of fixes to make web application work behind a
  reverse proxy.

Version 4.0.0

- Major rewrite, may break code.
- Added a JSON parser and compatible internal object, is analogous
  to the version of nlohmann. Replaces the old element class in
  webapp.
- Removed parameter_map, get request parameters from request itself.
- Reorganized code, separate folder for lib and examples.
- Refactored webapp and move the tag processing into a separate
  class. Added a second tag processor that mimics thymeleaf.

Version 3.0.2

- Change in zeep/xml/serialize.hpp for gcc 4.7 compiler

Version 3.0.1

- added cast to uint32 in webapp-el to allow compilation on s390

Version 3.0

- Support for non-intrusive serialization. The call to serialize is now
  done by the templated struct zeep::xml::struct_serializer. You can
  create a specialization for this struct to do something else than
  calling MyClass::serialize.
- xml::document now has serialize and deserialize members.
- A streaming input added, process_document_elements calls the callback
  for all elements that match a given xpath.
- ISO8859-1 support (finally)
- some xpath additions (matches e.g.)
- changed signature of various find routines to work with const char*
- changed authentication mechanism in webapp to allow multiple realms
- some small changes in writing out XML documents/xml::writer
- added line number to validation error messages
- process value tag of mrs:option tag
- el processing returns original string if it does not contain an expression
- in expression language, support var1[var2] constructs
- fix in writing doctype declaration
- insert/erase implementations of zeep::xml::node...
- fixed bug in el implementation (dividing numbers)
- extended log format of HTTP server to allow better awstat logs (using the
  extra fields for SOAP calls). Also writes the X-Forwarded-For client if any.
- Patches provided by Patrick Rotsaert: serializer for xsd:time and
  optional data types based on boost::optional.
- Split out log_request as a virtual method in http::server
- Added quick and dirty test for requests from mobile clients
- Added virtual destructors to all base classes.
- OPTIONS and HEAD support in web server

Version 2.9.0

- Added some calls to xml::writer to write e.g. xml-decl and doctypes
Version 2.8.2

- Fix in unicode support code
- Preliminary support for handling authentication

Version 2.8.1

- removed boost::ptr_vector/ptr_list.
- work around a crashing bug in el::object[string] when compiling with icpc

Version 2.8.0

- write_content added.
- nullptr instead of nil, added a stub for old compilers.
- fix in el::object (mixing up uint64 and size_t)

Version 2.6.3

- Fix for stack overflow in delete large XML documents

Version 2.6.2

- Apparently the word size has changed on amd64/GNUC targets. I've
  switched to a more robust template selection algorithm for WSDL
  generation.

Version 2.6.1

- Fix in keep-alive (clear reply object after each served reply)
- Implemented missing at() virtual method for el::vector
- Writing comments now validates output
- check mounted paths instead of only the root for handlers
- optimization flags in makefile

Version 2.6.0

- Changed parameter_map (for webapp) into a multimap

Version 2.5.2

- Throw exception when attempting to write null character.

Version 2.5.1
  
- Removed the use of split_iterator from webapp since it generated
  crashes when built as a shared library...

Version 2.5.0
  
- added webapp, a base class used to create web applications,
  it uses XHTML templates to fill in. It uses a script language
  to enable interaction with the C++ code.

Version 2.1.0

- support for HTTP/1.1
- added multiplication in xpath expression language... oops
- revised interface for container::iterator, now it is possible
  to use more STL and boost functions on a container directly, like:
  
  xml::container cnt = ...;
  foreach (node* n, cnt) { cout << n->name() << endl; }
