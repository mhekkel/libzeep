cmake_minimum_required(VERSION 3.24)

project(libzeep VERSION 6.0.8 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(ZEEP_BUILD_LIBRARY "Build the library from source code, should be off when configuring on ReadTheDocs" ON)
option(ZEEP_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEP_USE_BOOST_ASIO "Use the asio library from Boost instead of the non-boost version" ON)
option(ZEEP_ALLOW_FETCH_CONTENT "Use cmake's FetchContent to retrieve missing packages" ON)

include(GNUInstallDirs)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(FindFilesystem)
include(CheckLibraryExists)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(VersionString)
include(CTest)

if(ZEEP_BUILD_LIBRARY)
	set(CXX_EXTENSIONS OFF)
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	# Build debug library with d postfix
	set(CMAKE_DEBUG_POSTFIX d)

	# When building with ninja-multiconfig, build both debug and release by default
	if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
		set(CMAKE_CROSS_CONFIGS "Debug;Release")
		set(CMAKE_DEFAULT_CONFIGS "Debug;Release")
	endif()

	find_package(Filesystem REQUIRED)

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
	elseif(MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()

	# Build shared libraries by default (not my cup of tea, but hey)
	option(BUILD_SHARED_LIBS "Build a shared library instead of a static one" OFF)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

	if(WIN32)
		if(${CMAKE_SYSTEM_VERSION} GREATER_EQUAL 10) # Windows 10
			add_definitions(-D _WIN32_WINNT=0x0A00)
		elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.3) # Windows 8.1
			add_definitions(-D _WIN32_WINNT=0x0603)
		elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.2) # Windows 8
			add_definitions(-D _WIN32_WINNT=0x0602)
		elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.1) # Windows 7
			add_definitions(-D _WIN32_WINNT=0x0601)
		elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.0) # Windows Vista
			add_definitions(-D _WIN32_WINNT=0x0600)
		else() # Windows XP (5.1)
			add_definitions(-D _WIN32_WINNT=0x0501)
		endif()

		add_definitions(-DNOMINMAX)
	endif()

	if(MSVC)
		# make msvc standards compliant...
		add_compile_options(/permissive-)
	endif()

	# Optionally use mrc to create resources
	if(WIN32 AND BUILD_SHARED_LIBS)
		message("Not using resources when building shared libraries for Windows")
	else()
		find_package(Mrc QUIET)

		if(MRC_FOUND)
			option(USE_RSRC "Use mrc to create resources" ON)

			if(USE_RSRC)
				message("Using resources compiled with ${MRC_EXECUTABLE} version ${MRC_VERSION_STRING}")
				set(WEBAPP_USES_RESOURCES 1)
			endif()
		else()
			message(STATUS "Not using resources since mrc was not found")
		endif()
	endif()

	set(CMAKE_THREAD_PREFER_PTHREAD)
	set(THREADS_PREFER_PTHREAD_FLAG)
	find_package(Threads REQUIRED)

	if(ZEEP_USE_BOOST_ASIO)
		set(BOOST_INCLUDE_LIBRARIES headers test)
		set(BOOST_ENABLE_CMAKE ON)

		find_package(Boost 1.74 QUIET)

		if(NOT Boost_FOUND)
			if(ZEEP_ALLOW_FETCH_CONTENT)
				message(STATUS "Fetching boost, may take a while...")

				include(FetchContent)
				FetchContent_Declare(
					Boost
					GIT_REPOSITORY https://github.com/boostorg/boost.git
					GIT_TAG boost-1.74.0
				)
				FetchContent_MakeAvailable(Boost)

				set(Boost_FETCHED ON)
			else()
				message(FATAL_ERROR "No boost found and cannot use FetchContent")
			endif()
		endif()

		set(ASIO_INCLUDE_DIR ${Boost_INCLUDE_DIR})
		message(STATUS "Using asio from boost")

		configure_file(${PROJECT_SOURCE_DIR}/cmake/asio-boost.hpp.in ${PROJECT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)
	else()
		find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
		message(STATUS "Using standalone asio")
		configure_file(${PROJECT_SOURCE_DIR}/cmake/asio.hpp.in ${PROJECT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)
	endif()

	find_package(date 3.0.1 QUIET)

	if(NOT date_FOUND)
		if(ZEEP_ALLOW_FETCH_CONTENT)
			include(FetchContent)

			FetchContent_Declare(date GIT_REPOSITORY https://github.com/HowardHinnant/date.git GIT_TAG v3.0.1)
			FetchContent_MakeAvailable(date)
		else()
			message(FATAL_ERROR "The date package was not found, please install https://github.com/HowardHinnant/date and try again")
		endif()
	endif()

	if(UNIX)
		find_file(HAVE_SYS_WAIT_H "sys/wait.h")

		if(HAVE_SYS_WAIT_H)
			set(HTTP_SERVER_HAS_PREFORK 1)
		endif()
	endif()

	# Generate the revision.hpp file
	write_version_header("${PROJECT_SOURCE_DIR}/lib-xml/src" LIB_NAME "libzeep")

	list(APPEND ZEEP_HEADERS
		${PROJECT_SOURCE_DIR}/include/zeep/crypto.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/streambuf.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/value-serializer.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/config.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/xml/document.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/xml/parser.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/xml/xpath.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/xml/node.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/xml/character-classification.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/xml/serialize.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/xml/doctype.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/type-traits.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/to_element.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/element.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/factory.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/serializer.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/parser.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/type_traits.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/from_element.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/element_fwd.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/json/iterator.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/asio.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/access-control.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/controller.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/message-parser.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/tag-processor.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/connection.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/server.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/daemon.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/error-handler.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/reply.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/template-processor.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/header.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/request.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/rest-controller.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/soap-controller.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/html-controller.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/security.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/uri.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/el-processing.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/http/login-controller.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/unicode-support.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/nvp.hpp
		${PROJECT_SOURCE_DIR}/include/zeep/exception.hpp
	)

	list(APPEND ZEEP_SRC
		${PROJECT_SOURCE_DIR}/lib-http/src/access-control.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/connection.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/controller.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/controller-rsrc.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/crypto.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/daemon.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/el-processing.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/error-handler.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/format.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/glob.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/html-controller.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/login-controller.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/message-parser.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/reply.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/request.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/rest-controller.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/security.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/server.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/uri.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/soap-controller.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/tag-processor-v2.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/tag-processor.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/template-processor.cpp
		${PROJECT_SOURCE_DIR}/lib-http/src/signals.cpp
		${PROJECT_SOURCE_DIR}/lib-json/src/element.cpp
		${PROJECT_SOURCE_DIR}/lib-json/src/json-parser.cpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/character-classification.cpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/doctype.cpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/document.cpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/html-named-characters.hpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/html-named-characters.cpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/node.cpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/xml-parser.cpp
		${PROJECT_SOURCE_DIR}/lib-xml/src/xpath.cpp
	)

	if(HTTP_SERVER_HAS_PREFORK)
		list(APPEND ZEEP_HEADERS ${PROJECT_SOURCE_DIR}/include/zeep/http/preforked-server.hpp)
		list(APPEND ZEEP_SRC ${PROJECT_SOURCE_DIR}/lib-http/src/preforked-server.cpp)
	endif()

	add_library(zeep ${ZEEP_SRC} ${ZEEP_HEADERS})
	add_library(zeep::zeep ALIAS zeep)
	set_target_properties(zeep PROPERTIES POSITION_INDEPENDENT_CODE ON)

	target_include_directories(zeep
		PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
	)

	target_link_libraries(zeep PUBLIC Threads::Threads date::date ${STDCPPFS_LIBRARY})

	if(ZEEP_USE_BOOST_ASIO)
		target_link_libraries(zeep PUBLIC Boost::headers)
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
		target_link_options(zeep PRIVATE -undefined dynamic_lookup)
	endif()

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

	if(ZEEP_USE_BOOST_ASIO)
		set(FIND_DEPENDENCY_BOOST "find_dependency(Boost REQUIRED)")
	endif()

	configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/zeepConfig.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfig.cmake
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
	)

	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfigVersion.cmake"
		COMPATIBILITY AnyNewerVersion
	)

	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeepConfigVersion.cmake"
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
		COMPONENT Devel
	)

	set(zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
	set_target_properties(zeep PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}
		INTERFACE_zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
		COMPATIBLE_INTERFACE_STRING zeep_MAJOR_VERSION
	)

	# Config file
	set(LIBZEEP_VERSION ${PROJECT_VERSION})
	set(LIBZEEP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
	set(LIBZEEP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
	set(LIBZEEP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
	configure_file("${PROJECT_SOURCE_DIR}/include/zeep/config.hpp.in" "${PROJECT_SOURCE_DIR}/include/zeep/config.hpp" @ONLY)
endif(ZEEP_BUILD_LIBRARY)

# Documentation
if(ZEEP_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()

# Test applications
if(BUILD_TESTING)

	if(NOT Boost_FETCHED)
		find_package(Boost REQUIRED)
	endif()

	find_package(libmcfp QUIET)

	if(NOT libmcfp_FOUND)
		if(ZEEP_ALLOW_FETCH_CONTENT)
			include(FetchContent)

			FetchContent_Declare(libmcfp GIT_REPOSITORY https://github.com/mhekkel/libmcfp GIT_TAG v1.2.4)
			FetchContent_MakeAvailable(libmcfp)
		elseif(EXISTS libmcfp/CMakeLists.txt)
			add_subdirectory(libmcfp EXCLUDE_FROM_ALL)
		else()
			message(FATAL_ERROR "The libmcfp package was not found, please install https://github.com/mhekkel/libmcfp and try again")
		endif()
	endif()

	# data files for the parser test
	add_library(client_test OBJECT ${PROJECT_SOURCE_DIR}/lib-http/test/client-test-code.cpp)

	target_include_directories(client_test PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_BINARY_DIR} # for config.h
		${PROJECT_SOURCE_DIR}/include
	)
	target_link_libraries(client_test PRIVATE Boost::boost)

	# data files for the parser test
	set(XML_CONF_TAR ${PROJECT_SOURCE_DIR}/lib-xml/test/XML-Test-Suite.tbz)
	set(XML_CONF_FILE ${PROJECT_SOURCE_DIR}/lib-xml/test/XML-Test-Suite/xmlconf/xmlconf.xml)

	if(NOT EXISTS ${XML_CONF_FILE})
		if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
			find_program(TAR tar)

			if(TAR)
				add_custom_command(OUTPUT ${XML_CONF_FILE}
					COMMAND ${TAR} xf ${XML_CONF_TAR}
					WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib-xml/test/)
			else()
				message(FATAL_ERROR "Please extract the archive ${XML_CONF_TAR} manually or update your cmake version to at least 3.18")
			endif()
		else()
			file(ARCHIVE_EXTRACT INPUT ${XML_CONF_TAR}
				DESTINATION ${PROJECT_SOURCE_DIR}/lib-xml/test/)
		endif()
	endif()

	add_custom_target(XML_CONF ALL DEPENDS ${XML_CONF_FILE})

	# unit parser serializer xpath json crypto http processor webapp soap rest security uri
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

		if(USE_RSRC AND("${TEST}" STREQUAL "http/processor" OR "${TEST}" STREQUAL "http/rsrc_webapp"))
			mrc_target_resources(${ZEEP_TEST} ${PROJECT_SOURCE_DIR}/lib-http/test/fragment-file.xhtml)
		endif()

		target_include_directories(${ZEEP_TEST} PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_BINARY_DIR} # for config.h
			${PROJECT_SOURCE_DIR}/include
		)

		target_link_libraries(${ZEEP_TEST} PRIVATE zeep ${STDCPPFS_LIBRARY} libmcfp::libmcfp)

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
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib-${ZEEP_TEST_DIR}/test)
	endforeach()
endif()

# add_subdirectory(examples)