cmake_minimum_required(VERSION 3.16)

project(libzeep VERSION 5.1.6 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(FindFilesystem)
include(CheckLibraryExists)
include(CMakePackageConfigHelpers)
include(Dart)
include(GenerateExportHeader)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Filesystem REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
endif()

# Build shared libraries by default (not my cup of tea, but hey)
option(BUILD_SHARED_LIBS "Build a shared library instead of a static one" OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(MSVC)
    # make msvc standards compliant...
    add_compile_options(/permissive-)

	macro(get_WIN32_WINNT version)
		if (WIN32 AND CMAKE_SYSTEM_VERSION)
			set(ver ${CMAKE_SYSTEM_VERSION})
			string(REPLACE "." "" ver ${ver})
			string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})

			set(${version} "0x${ver}")
		endif()
	endmacro()

	get_WIN32_WINNT(ver)
	add_definitions(-D_WIN32_WINNT=${ver})

	# On Windows, do not install in the system location by default
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		message("The library and auxiliary files will be installed in $ENV{LOCALAPPDATA}/zeep")
		set(CMAKE_INSTALL_PREFIX "$ENV{LOCALAPPDATA}/zeep" CACHE PATH "..." FORCE)
	endif()
endif()

# Optionally use mrc to create resources
if(WIN32 AND BUILD_SHARED_LIBS)
	message("Not using resources when building shared libraries for Windows")
else()
	find_package(Mrc)

	if(MRC_FOUND)
		option(USE_RSRC "Use mrc to create resources" ON)
	else()
		message(STATUS "Not using resources since mrc was not found")
	endif()

	if(USE_RSRC)
		message("Using resources compiled with ${MRC_EXECUTABLE}")
		set(WEBAPP_USES_RESOURCES 1)
		mrc_write_header(${CMAKE_BINARY_DIR}/mrsrc.hpp)
	endif()
endif()

set(CMAKE_THREAD_PREFER_PTHREAD)
set(THREADS_PREFER_PTHREAD_FLAG)
find_package(Threads REQUIRED)

set(Boost_DETAILED_FAILURE_MSG ON)
if(NOT BUILD_SHARED_LIBS)
	set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost 1.70.0 REQUIRED COMPONENTS program_options system date_time regex)

if(UNIX)
	find_file(HAVE_SYS_WAIT_H "sys/wait.h")
	if(HAVE_SYS_WAIT_H)
		set(HTTP_SERVER_HAS_PREFORK 1)
	endif()
endif()

list(APPEND ZEEP_HEADERS
	${CMAKE_SOURCE_DIR}/include/zeep/crypto.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/streambuf.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/value-serializer.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/config.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/xml/document.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/xml/parser.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/xml/xpath.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/xml/node.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/xml/character-classification.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/xml/serialize.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/xml/doctype.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/type-traits.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/to_element.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/element.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/factory.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/serializer.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/parser.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/type_traits.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/from_element.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/element_fwd.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/json/iterator.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/controller.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/message-parser.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/tag-processor.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/connection.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/server.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/daemon.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/error-handler.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/reply.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/template-processor.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/header.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/request.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/rest-controller.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/soap-controller.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/html-controller.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/security.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/uri.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/el-processing.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/http/login-controller.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/unicode-support.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/nvp.hpp
	${CMAKE_SOURCE_DIR}/include/zeep/exception.hpp
)

set(ZEEP_SRC
	${CMAKE_SOURCE_DIR}/lib-http/src/connection.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/controller.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/controller-rsrc.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/crypto.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/daemon.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/el-processing.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/error-handler.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/format.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/glob.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/html-controller.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/login-controller.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/message-parser.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/reply.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/request.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/rest-controller.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/security.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/server.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/uri.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/soap-controller.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/tag-processor-v2.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/tag-processor.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/template-processor.cpp
	${CMAKE_SOURCE_DIR}/lib-http/src/signals.cpp
	${CMAKE_SOURCE_DIR}/lib-json/src/element.cpp
	${CMAKE_SOURCE_DIR}/lib-json/src/json-parser.cpp
	${CMAKE_SOURCE_DIR}/lib-xml/src/character-classification.cpp
	${CMAKE_SOURCE_DIR}/lib-xml/src/doctype.cpp
	${CMAKE_SOURCE_DIR}/lib-xml/src/document.cpp
	${CMAKE_SOURCE_DIR}/lib-xml/src/node.cpp
	${CMAKE_SOURCE_DIR}/lib-xml/src/xml-parser.cpp
	${CMAKE_SOURCE_DIR}/lib-xml/src/xpath.cpp
)

if(HTTP_SERVER_HAS_PREFORK)
	list(APPEND ZEEP_HEADERS ${CMAKE_SOURCE_DIR}/include/zeep/http/preforked-server.hpp)
	list(APPEND ZEEP_SRC ${CMAKE_SOURCE_DIR}/lib-http/src/preforked-server.cpp)
endif()

add_library(zeep ${ZEEP_SRC} ${ZEEP_HEADERS})
set_target_properties(zeep PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(zeep
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
	${Boost_INCLUDE_DIR}
)

target_link_libraries(zeep PUBLIC Boost::regex Boost::date_time Threads::Threads ${STDCPPFS_LIBRARY})

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_options(zeep PRIVATE -undefined dynamic_lookup)
endif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} )
set(LIBRARY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} )

generate_export_header(zeep EXPORT_FILE_NAME zeep/zeep_export.hpp)

# Install rules

install(TARGETS zeep
	EXPORT zeepTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT zeepTargets
	FILE "zeepTargets.cmake"
	NAMESPACE zeep::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
)

install(
	DIRECTORY include/zeep
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT Devel
	PATTERN "config.hpp.in" EXCLUDE
)

install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep_export.hpp"
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zeep
	COMPONENT Devel
)

configure_package_config_file(Config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
	PATH_VARS INCLUDE_INSTALL_DIR LIBRARY_INSTALL_DIR
)

install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfigVersion.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
	COMPONENT Devel
)

set(zeep_MAJOR_VERSION 5)
set_target_properties(zeep PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION 5.1
	INTERFACE_zeep_MAJOR_VERSION 5)

set_property(TARGET zeep APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING zeep_MAJOR_VERSION
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Config file

set(LIBZEEP_VERSION ${PROJECT_VERSION})
set(LIBZEEP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(LIBZEEP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(LIBZEEP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
configure_file("${CMAKE_SOURCE_DIR}/include/zeep/config.hpp.in" "${CMAKE_SOURCE_DIR}/include/zeep/config.hpp" @ONLY)

# pkgconfig support

set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libzeep.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc.in @ONLY)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc
	INPUT ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc.in)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# Documentation
option(ZEEP_BUILD_DOC "Build documentation" OFF)

if(ZEEP_BUILD_DOC)
	find_program(DOXYGEN doxygen)
	find_program(QUICKBOOK quickbook)
	find_program(XSLTPROC xsltproc)
	find_program(FOP fop)
	find_program(BJAM bjam)

	if(NOT DOXYGEN)
		list(APPEND MISSING_DOC_TOOL_LIST doxygen)
	endif()
	if(NOT QUICKBOOK)
		list(APPEND MISSING_DOC_TOOL_LIST quickbook)
	endif()
	if(NOT XSLTPROC)
		list(APPEND MISSING_DOC_TOOL_LIST xsltproc)
	endif()
	if(NOT FOP)
		list(APPEND MISSING_DOC_TOOL_LIST fop)
	endif()
	if(NOT BJAM)
		list(APPEND MISSING_DOC_TOOL_LIST bjam)
	endif()

	if(DEFINED MISSING_DOC_TOOL_LIST)
		list(JOIN MISSING_DOC_TOOL_LIST ", " MISSING_DOC_TOOLS)
		message(FATAL_ERROR "Cannot create documentation since the following applications could not be found: ${MISSING_DOC_TOOLS}")
	endif()

	make_directory(doc)

	if(UNIX)
		set(QUICKBOOK_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/quickbook.sh")

		configure_file(${CMAKE_SOURCE_DIR}/doc/tools/quickbook.sh.in
			${CMAKE_CURRENT_BINARY_DIR}/tmp/quickbook.sh
			@ONLY)
		file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/quickbook.sh
			DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
			FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
		file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/tmp/)
	else()
		set(QUICKBOOK_SCRIPT ${QUICKBOOK})
	endif()

	configure_file(${CMAKE_SOURCE_DIR}/doc/Jamfile.v2.in ${CMAKE_SOURCE_DIR}/doc/Jamfile.v2 @ONLY)
	
	add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/doc/html/index.html
			${CMAKE_SOURCE_DIR}/doc/html ${CMAKE_SOURCE_DIR}/doc/bin
			${CMAKE_SOURCE_DIR}/doc/autodoc.xml
		COMMAND ${BJAM} ${CMAKE_SOURCE_DIR}/doc
		DEPENDS ${CMAKE_SOURCE_DIR}/doc/Jamfile.v2)
	add_custom_target(doc ALL DEPENDS ${CMAKE_SOURCE_DIR}/doc/html/index.html)
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/html DESTINATION ${CMAKE_INSTALL_DOCDIR}/${PACKAGE_NAME})
endif()

# Test applications

option(ZEEP_BUILD_TESTS "Build test executables" OFF)

if(ZEEP_BUILD_TESTS)

	enable_testing()

	# data files for the parser test

	add_library(client_test OBJECT ${CMAKE_SOURCE_DIR}/lib-http/test/client-test-code.cpp)
	target_include_directories(client_test PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_BINARY_DIR}  # for config.h
		${CMAKE_SOURCE_DIR}/include
		${Boost_INCLUDE_DIR}
	)

	# data files for the parser test
	set(XML_CONF_TAR ${CMAKE_SOURCE_DIR}/lib-xml/test/XML-Test-Suite.tbz)
	set(XML_CONF_FILE ${CMAKE_SOURCE_DIR}/lib-xml/test/XML-Test-Suite/xmlconf/xmlconf.xml)

	if (NOT EXISTS ${XML_CONF_FILE})
		if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
			find_program(TAR tar)

			if(TAR)
				add_custom_command(OUTPUT ${XML_CONF_FILE}
					COMMAND ${TAR} xf ${XML_CONF_TAR}
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib-xml/test/)
			else()
				message(FATAL_ERROR "Please extract the archive ${XML_CONF_TAR} manually or update your cmake version to at least 3.18")
			endif()
		else()
			file(ARCHIVE_EXTRACT INPUT ${XML_CONF_TAR}
				DESTINATION ${CMAKE_SOURCE_DIR}/lib-xml/test/
				VERBOSE)
		endif()
	endif()

	add_custom_target(XML_CONF ALL DEPENDS ${XML_CONF_FILE})

	#  unit parser serializer xpath json crypto http processor webapp soap rest security uri

	list(APPEND zeep_tests
		xml/unit
		xml/parser
		xml/serializer
		xml/xpath
		json/json
		http/crypto
		http/http
		http/processor
		http/webapp
		http/soap
		http/rest
		http/security
		http/uri)

	if(USE_RSRC)
		list(APPEND zeep_tests http/rsrc_webapp)
	endif()

	foreach(TEST IN LISTS zeep_tests)
		string(REGEX MATCH "^[^/]+" ZEEP_TEST_DIR ${TEST})
		string(REGEX MATCH "[^/]+$" ZEEP_TEST ${TEST})

		set(ZEEP_TEST "${ZEEP_TEST}-test")
		set(ZEEP_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/lib-${ZEEP_TEST_DIR}/test/${ZEEP_TEST}.cpp")

		add_executable(${ZEEP_TEST} ${ZEEP_TEST_SOURCE} $<TARGET_OBJECTS:client_test> ${ZEEP_TEST_RESOURCE})

		if("${TEST}" STREQUAL "http/processor")
			target_compile_definitions(${ZEEP_TEST} PUBLIC DOCROOT=".")
		endif()

		if(USE_RSRC AND ("${TEST}" STREQUAL "http/processor" OR "${TEST}" STREQUAL "http/rsrc_webapp"))
			mrc_target_resources(${ZEEP_TEST} ${CMAKE_SOURCE_DIR}/lib-http/test/fragment-file.xhtml)
		endif()

		target_include_directories(${ZEEP_TEST} PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_BINARY_DIR}  # for config.h
			${CMAKE_SOURCE_DIR}/include
		)

		target_link_libraries(${ZEEP_TEST} PRIVATE zeep ${STDCPPFS_LIBRARY} Boost::program_options)

		if(MSVC)
			# Specify unwind semantics so that MSVC knowns how to handle exceptions
			target_compile_options(${ZEEP_TEST} PRIVATE /EHsc)
		endif()

		add_custom_target("run-${ZEEP_TEST}" DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Run${ZEEP_TEST}.touch ${ZEEP_TEST})

		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Run${ZEEP_TEST}.touch
			COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${ZEEP_TEST}
		)

		if("${TEST}" STREQUAL "xml/parser")
			# Some tests should be skipped
			list(APPEND ZEEP_TEST_ARGS 
				ibm-valid-P28-ibm28v02.xml ibm-valid-P29-ibm29v01.xml ibm-valid-P29-ibm29v02.xml
				ibm-1-1-valid-P03-ibm03v09.xml rmt-e2e-34 rmt-e2e-55 rmt-054 rmt-ns10-006 rmt-e3e-13)
			list(TRANSFORM ZEEP_TEST_ARGS PREPEND "--questionable=")
			list(PREPEND ZEEP_TEST_ARGS ${XML_CONF_FILE} "--print-ids")
			add_dependencies(${ZEEP_TEST} XML_CONF)
		else()
			set(ZEEP_TEST_ARGS "")
		endif()

		add_test(NAME ${ZEEP_TEST}
			COMMAND $<TARGET_FILE:${ZEEP_TEST}> ${ZEEP_TEST_ARGS}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib-${ZEEP_TEST_DIR}/test)

	endforeach()
endif()
