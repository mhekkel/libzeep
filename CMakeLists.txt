# Copyright Maarten L. Hekkelman, 2023
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.22)

project(libzeep VERSION 6.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(FindFilesystem)
include(CheckLibraryExists)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(VersionString)
include(CTest)
include(CMakeDependentOption)

option(ZEEP_BUILD_LIBRARY "Build the library from source code, should be off when configuring on ReadTheDocs" ON)
option(ZEEP_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEP_USE_BOOST_ASIO "Use the asio library from Boost instead of the non-boost version" ON)
option(ZEEP_ALLOW_FETCH_CONTENT "Use cmake's FetchContent to retrieve missing packages" ON)
cmake_dependent_option(ZEEP_BUILD_EXAMPLES "Build the example applications" ON "ZEEP_BUILD_LIBRARY" OFF)

# New policy for Boost, set it here
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
	cmake_policy(SET CMP0167 NEW)
endif()

if(ZEEP_BUILD_LIBRARY)
	set(CXX_EXTENSIONS OFF)
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	# When building with ninja-multiconfig, build both debug and release by default
	if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
		set(CMAKE_CROSS_CONFIGS "Debug;Release")
		set(CMAKE_DEFAULT_CONFIGS "Debug;Release")
	endif()

	find_package(Filesystem REQUIRED)

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
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
		find_package(Boost 1.74 REQUIRED)

		message(STATUS "Using asio from boost")

		configure_file(${PROJECT_SOURCE_DIR}/cmake/asio-boost.hpp.in ${PROJECT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)

		set(FIND_DEPENDENCY_BOOST "find_dependency(Boost REQUIRED)")
	else()
		find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
		message(STATUS "Using standalone asio")
		configure_file(${PROJECT_SOURCE_DIR}/cmake/asio.hpp.in ${PROJECT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)
	endif()

	if(NOT (TARGET date OR date_FOUND))
		find_package(date 3.0.1 QUIET NAMES date libhowardhinnant-date)

		if(NOT date_FOUND)
			if(ZEEP_ALLOW_FETCH_CONTENT)
				include(FetchContent)

				FetchContent_Declare(date GIT_REPOSITORY https://github.com/HowardHinnant/date.git GIT_TAG v3.0.1)
				FetchContent_MakeAvailable(date)
			else()
				message(FATAL_ERROR "The date package was not found, please install https://github.com/HowardHinnant/date and try again")
			endif()
		endif()
	endif()

	if(UNIX)
		set(HTTP_HAS_UNIX_DAEMON 1)

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

	if(USE_RSRC)
		list(APPEND ZEEP_SRC
			${PROJECT_SOURCE_DIR}/lib-http/src/controller-rsrc.cpp)
	endif()

	if(HTTP_SERVER_HAS_PREFORK)
		list(APPEND ZEEP_HEADERS ${PROJECT_SOURCE_DIR}/include/zeep/http/preforked-server.hpp)
		list(APPEND ZEEP_SRC ${PROJECT_SOURCE_DIR}/lib-http/src/preforked-server.cpp)
	endif()

	add_library(zeep ${ZEEP_SRC} ${ZEEP_HEADERS})
	add_library(zeep::zeep ALIAS zeep)
	set_target_properties(zeep PROPERTIES POSITION_INDEPENDENT_CODE ON)

	target_include_directories(zeep
		PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${ASIO_INCLUDE_DIR}>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
	)

	target_link_libraries(zeep PUBLIC Threads::Threads date::date ${STDCPPFS_LIBRARY})

	if(ZEEP_USE_BOOST_ASIO)
		target_link_libraries(zeep PUBLIC Boost::boost)
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
		target_link_options(zeep PRIVATE -undefined dynamic_lookup)
	endif()

	# # Build debug library with d postfix
	# set(CMAKE_DEBUG_POSTFIX d)
	# set_target_properties(zeep PROPERTIES DEBUG_POSTFIX "d")

	# Install rules
	install(TARGETS zeep
		EXPORT zeep-targets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

	install(EXPORT zeep-targets
		NAMESPACE zeep::
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
	)

	install(
		DIRECTORY include/zeep
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		COMPONENT Devel
		PATTERN "config.hpp.in" EXCLUDE
	)

	configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/zeep-config.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
	)

	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
		COMPATIBILITY AnyNewerVersion
	)

	file(GLOB OLD_CONFIG_FILES
		${CMAKE_INSTALL_FULL_LIBDIR}/cmake/zeep/zeepConfig*.cmake
		${CMAKE_INSTALL_FULL_LIBDIR}/cmake/zeep/zeepTargets*.cmake)

	if(OLD_CONFIG_FILES)
		message(STATUS "Installation will remove old config files: ${OLD_CONFIG_FILES}")
		install(CODE "file(REMOVE ${OLD_CONFIG_FILES})")
	endif()

	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
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

# Examples
if(ZEEP_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()
