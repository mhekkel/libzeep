cmake_minimum_required(VERSION 3.16)

project(libzeep VERSION 5.0.0 LANGUAGES CXX C)

enable_testing()

include(GNUInstallDirs)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CMakePackageConfigHelpers)
include(Dart)

set(PACKAGE "zeep")
set(PACKAGE_VERSION "5.0.0")
add_definitions(-DVERSION=${PACKAGE_VERSION})

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
endif()
if(MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
endif()

set(CMAKE_DEBUG_POSTFIX d)

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
		message(WARNING "The library and auxiliary files will be installed in $ENV{LOCALAPPDATA}/${PROJECT_NAME}")
		set(CMAKE_INSTALL_PREFIX "$ENV{LOCALAPPDATA}/${PROJECT_NAME}" CACHE PATH "..." FORCE)
	endif()

	# Find out the processor type for the target
	if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
		set(COFF_TYPE "x64")
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i386")
		set(COFF_TYPE "x86")
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ARM64")
		set(COFF_TYPE "arm64")
	else()
		message(FATAL_ERROR "Unsupported or unknown processor type ${CMAKE_SYSTEM_PROCESSOR}")
	endif()	

	set(COFF_SPEC "--coff=${COFF_TYPE}")
endif()

# Optionally use mrc to create resources
find_program(MRC mrc HINTS "$ENV{LOCALAPPDATA}/mrc" "${CMAKE_INSTALL_PREFIX}/../mrc" "/usr/local/bin")

if(MRC)
	option(USE_RSRC "Use mrc to create resources" ON)
else()
	message(WARNING "Not using resources since mrc was not found")
endif()

if(USE_RSRC STREQUAL "ON")
	set(USE_RSRC 1)

	message("Using resources compiled with ${MRC}")
	add_compile_definitions(USE_RSRC)
endif()

find_package(Threads REQUIRED)

set (Boost_DETAILED_FAILURE_MSG ON)
find_package(Boost 1.71.0 REQUIRED COMPONENTS program_options system date_time)
# extra diagnostic -- helpful for problem with FindBoost.cmake
message(STATUS "Boost headers in: ${Boost_INCLUDE_DIR}")
message(STATUS "Boost libraries in: ${Boost_LIBRARY_DIRS}")

set(CMAKE_THREAD_PREFER_PTHREAD)
set(THREADS_PREFER_PTHREAD_FLAG)
find_package(Threads)

find_package(ZLIB)
find_package(BZip2)

include_directories(${Boost_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/include)
link_libraries(${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

set(ZEEP_HEADERS
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
        ${CMAKE_SOURCE_DIR}/include/zeep/http/preforked-server.hpp
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
        ${CMAKE_SOURCE_DIR}/lib-http/src/preforked-server.cpp
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

add_library(zeep_obj OBJECT ${ZEEP_SRC} ${ZEEP_HEADERS})
set_target_properties(zeep_obj PROPERTIES POSITION_INDEPENDENT_CODE 1)

add_library(zeep SHARED $<TARGET_OBJECTS:zeep_obj>)
set_target_properties(zeep PROPERTIES SOVERSION ${PACKAGE_VERSION})

add_library(zeep_static STATIC $<TARGET_OBJECTS:zeep_obj>)

install(TARGETS zeep zeep_static
	  EXPORT zeepTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	  DOCUMENTATION DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_options(zeep PRIVATE -undefined dynamic_lookup)
endif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

configure_file(${PROJECT_SOURCE_DIR}/include/zeep/revision.hpp.in revision.hpp @ONLY)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp")
install(FILES ${CMAKE_BINARY_DIR}/revision.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zeep/)

set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libzeep.pc.cmakein
	${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc.in @ONLY)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc
	INPUT ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc.in)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set(ConfigPackageLocation lib/cmake)

configure_package_config_file(zeepConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfig.cmake
	INSTALL_DESTINATION ${ConfigPackageLocation}
	PATH_VARS includedir libdir)

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion)

install(EXPORT zeepTargets
        FILE zeepTargets.cmake
        NAMESPACE zeep::
        DESTINATION ${ConfigPackageLocation}
		COMPONENT Devel
)

install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfigVersion.cmake
		DESTINATION ${ConfigPackageLocation}
		COMPONENT Devel
)

# Documentation
find_program(DOXYGEN doxygen)
find_program(QUICKBOOK quickbook)
find_program(XSLTPROC xsltproc)
find_program(FOP fop)
find_program(BJAM bjam)

if(DOXYGEN AND QUICKBOOK AND XSLTPROC AND FOP AND BJAM)
	option(ZEEP_BUILD_DOC "Build documentation" OFF)

	if(ZEEP_BUILD_DOC)
		make_directory(doc)
		configure_file(${CMAKE_SOURCE_DIR}/doc/Jamfile.v2.in ${CMAKE_SOURCE_DIR}/doc/Jamfile.v2 @ONLY)
		add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/doc/html/index.html
			COMMAND ${BJAM} ${CMAKE_SOURCE_DIR}/doc
			DEPENDS ${CMAKE_SOURCE_DIR}/doc/Jamfile.v2)
		add_custom_target(doc ALL DEPENDS ${CMAKE_SOURCE_DIR}/doc/html/index.html)
		install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/html DESTINATION ${CMAKE_INSTALL_DOCDIR}/${PACKAGE_NAME})
	endif()
else()
	message(WARNING "Cannot create documentation since the following applications could not be found:")

	if(NOT DOXYGEN)
		message(WARNING " => doxygen")
	endif()
	if(NOT QUICKBOOK)
		message(WARNING " => quickbook")
	endif()
	if(NOT XSLTPROC)
		message(WARNING " => xsltproc")
	endif()
	if(NOT FOP)
		message(WARNING " => fop")
	endif()
	if(NOT BJAM)
		message(WARNING " => bjam")
	endif()
endif()

# Test applications

option(ZEEP_BUILD_TESTS "Build test exectuables" OFF)

if(ZEEP_BUILD_TESTS)

	add_library(client_test OBJECT ${CMAKE_SOURCE_DIR}/lib-http/test/client-test-code.cpp)

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

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rsrc_webapp_rsrc.obj
			COMMAND ${MRC} -o ${CMAKE_CURRENT_BINARY_DIR}/rsrc_webapp_rsrc.obj ${CMAKE_SOURCE_DIR}/lib-http/test/fragment-file.xhtml ${COFF_SPEC}
		)
	endif()

	foreach(TEST IN LISTS zeep_tests)
		string(REGEX MATCH "^[^/]+" ZEEP_TEST_DIR ${TEST})
		string(REGEX MATCH "[^/]+$" ZEEP_TEST ${TEST})

		set(ZEEP_TEST "${ZEEP_TEST}-test")
		set(ZEEP_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/lib-${ZEEP_TEST_DIR}/test/${ZEEP_TEST}.cpp")
		set(ZEEP_TEST_RESOURCE "")

		if("${TEST}" STREQUAL "http/rsrc_webapp" AND USE_RSRC)
			set(ZEEP_TEST_RESOURCE ${CMAKE_CURRENT_BINARY_DIR}/rsrc_webapp_rsrc.obj)
		endif()

		add_executable(${ZEEP_TEST} ${ZEEP_TEST_SOURCE} $<TARGET_OBJECTS:client_test> ${ZEEP_TEST_RESOURCE})

		if("${TEST}" STREQUAL "http/processor")
			target_compile_definitions(${ZEEP_TEST} PUBLIC DOCROOT=".")
		endif()

		target_include_directories(${ZEEP_TEST} PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_BINARY_DIR}  # for config.h
		)

		target_link_libraries(${ZEEP_TEST} zeep_static)

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
			list(PREPEND ZEEP_TEST_ARGS ${XML_CONF_FILE})
		else()
			set(ZEEP_TEST_ARGS "")
		endif()

		add_test(NAME ${ZEEP_TEST}
			COMMAND $<TARGET_FILE:${ZEEP_TEST}> ${ZEEP_TEST_ARGS}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib-${ZEEP_TEST_DIR}/test)

	endforeach()
endif()
