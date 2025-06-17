# Copyright Maarten L. Hekkelman, 2023-2025
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.28)

project(libzeep VERSION 7.0.3 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakePackageConfigHelpers)
include(CPM)
include(CTest)
include(VersionString)

option(ZEEP_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEP_USE_BOOST_ASIO "Use the asio library from Boost instead of the non-boost version" ON)
option(ZEEP_BUILD_EXAMPLES "Build the example applications" ON)

# When building with ninja-multiconfig, build both debug and release by default
if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
	set(CMAKE_CROSS_CONFIGS "Debug;Release")
	set(CMAKE_DEFAULT_CONFIGS "Debug;Release")
endif()

# New policy for Boost, set it here
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
	cmake_policy(SET CMP0167 NEW)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wsuggest-override")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# Windows specific stuff
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
	find_package(mrc QUIET)

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
	find_package(Boost 1.74 REQUIRED COMPONENTS headers)

	message(STATUS "Using asio from boost")

	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio-boost.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)

	set(FIND_DEPENDENCY_BOOST "find_dependency(Boost REQUIRED)")
else()
	find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
	message(STATUS "Using standalone asio")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)
endif()

set(CPM_USE_LOCAL_PACKAGES ON)
CPMAddPackage("gh:HowardHinnant/date@3.0.3")
CPMAddPackage(NAME nlohmann_json VERSION 3.9.1 GITHUB_REPOSITORY nlohmann/json OPTIONS "JSON_BuildTests OFF")
CPMAddPackage("gh:mhekkel/mxml@2.0.0")

if(UNIX)
	find_file(HAVE_SYS_WAIT_H "sys/wait.h")

	if(HAVE_SYS_WAIT_H)
		set(HTTP_SERVER_HAS_PREFORK 1)
	endif()

	find_file(HAVE_UNISTD_H "unistd.h")
	if(HAVE_UNISTD_H)
		set(HTTP_HAS_UNIX_DAEMON 1)
	endif()
endif()

# Generate the revision.hpp file
write_version_header("${CMAKE_CURRENT_SOURCE_DIR}/src" LIB_NAME "libzeep")

# Library rules
add_library(zeep)
add_library(zeep::zeep ALIAS zeep)

list(APPEND ZEEP_HEADERS
	include/zeep/crypto.hpp
	include/zeep/streambuf.hpp
	include/zeep/config.hpp

	include/zeep/el/object.hpp
	include/zeep/el/serializer.hpp
	include/zeep/el/processing.hpp

	include/zeep/http/asio.hpp
	include/zeep/http/access-control.hpp
	include/zeep/http/controller.hpp
	include/zeep/http/message-parser.hpp
	include/zeep/http/tag-processor.hpp
	include/zeep/http/connection.hpp
	include/zeep/http/server.hpp
	include/zeep/http/daemon.hpp
	include/zeep/http/error-handler.hpp
	include/zeep/http/reply.hpp
	include/zeep/http/scope.hpp
	include/zeep/http/template-processor.hpp
	include/zeep/http/header.hpp
	include/zeep/http/request.hpp

	# include/zeep/http/soap-controller.hpp
	include/zeep/http/html-controller.hpp
	include/zeep/http/security.hpp
	include/zeep/http/uri.hpp
	include/zeep/http/login-controller.hpp
	include/zeep/unicode-support.hpp
	include/zeep/exception.hpp
)

list(APPEND ZEEP_SRC
	src/access-control.cpp
	src/connection.cpp
	src/controller.cpp
	src/crypto.cpp
	src/daemon.cpp
	src/el-object.cpp
	src/el-processing.cpp
	src/error-handler.cpp
	src/format.cpp
	src/glob.cpp
	src/html-controller.cpp
	src/login-controller.cpp
	src/message-parser.cpp
	src/reply.cpp
	src/request.cpp
	src/scope.cpp
	src/security.cpp
	src/server.cpp
	src/uri.cpp

	# src/soap-controller.cpp
	src/tag-processor.cpp
	src/template-processor.cpp
	src/signals.cpp
)

if(USE_RSRC)
	list(APPEND ZEEP_SRC src/controller-rsrc.cpp)
endif()

if(HTTP_SERVER_HAS_PREFORK)
	list(APPEND ZEEP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/preforked-server.hpp)
	list(APPEND ZEEP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/preforked-server.cpp)
endif()

target_sources(zeep
	PRIVATE ${ZEEP_SRC}
	PUBLIC FILE_SET zeep_headers TYPE HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES ${ZEEP_HEADERS}
)

set_target_properties(zeep PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(zeep PUBLIC cxx_std_20)

target_include_directories(zeep
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(zeep PUBLIC Threads::Threads date::date mxml::mxml nlohmann_json::nlohmann_json)

if(ZEEP_USE_BOOST_ASIO)
	target_link_libraries(zeep PUBLIC Boost::headers)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	target_link_options(zeep PRIVATE -undefined dynamic_lookup)
endif()

# Build debug library with d postfix
set(CMAKE_DEBUG_POSTFIX d)
set_target_properties(zeep PROPERTIES DEBUG_POSTFIX "d")

# Install rules
install(TARGETS zeep
	EXPORT zeep-targets
	FILE_SET zeep_headers
	DESTINATION include)

install(EXPORT zeep-targets
	NAMESPACE zeep::
	DESTINATION lib/cmake/zeep
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/zeep-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake
	INSTALL_DESTINATION lib/cmake/zeep
)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
	COMPATIBILITY AnyNewerVersion
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
	DESTINATION lib/cmake/zeep
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

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/config.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/config.hpp" @ONLY)

# Documentation
if(ZEEP_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()

# Examples
if(ZEEP_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# Test applications, but only if we're not included as a subproject
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()
