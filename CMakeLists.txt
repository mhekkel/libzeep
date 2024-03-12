# Copyright Maarten L. Hekkelman, 2023
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.28)

project(libzeep VERSION 7.0.0 LANGUAGES CXX)

if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
	message(FATAL_ERROR "This project requires the Ninja generator. Refers to https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html#generator-support")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(FetchContent)
include(VersionString)
include(CTest)
include(CMakeDependentOption)

option(ZEEP_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEP_USE_BOOST_ASIO "Use the asio library from Boost instead of the non-boost version" ON)
option(ZEEP_ALLOW_FETCH_CONTENT "Use cmake's FetchContent to retrieve missing packages" ON)
cmake_dependent_option(ZEEP_BUILD_EXAMPLES "Build the example applications" ON "ZEEP_BUILD_LIBRARY" OFF)

# When building with ninja-multiconfig, build both debug and release by default
if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
	set(CMAKE_CROSS_CONFIGS "Debug;Release")
	set(CMAKE_DEFAULT_CONFIGS "Debug;Release")
endif()

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
	find_package(Boost 1.74 REQUIRED)

	message(STATUS "Using asio from boost")

	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio-boost.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)

	set(FIND_DEPENDENCY_BOOST "find_dependency(Boost REQUIRED)")
else()
	find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
	message(STATUS "Using standalone asio")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)
endif()

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

# find_package(mxml REQUIRED)

# FetchContent_Declare(mxml GIT_REPOSITORY s4.hekkelman.net:git-repo/mxml.git GIT_TAG no-module)
# FetchContent_MakeAvailable(mxml)

add_subdirectory(mxml)

if(UNIX)
	find_file(HAVE_SYS_WAIT_H "sys/wait.h")

	if(HAVE_SYS_WAIT_H)
		set(HTTP_SERVER_HAS_PREFORK 1)
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
	include/zeep/value-serializer.hpp
	include/zeep/config.hpp
	include/zeep/type-traits.hpp
	include/zeep/json/to_element.hpp
	include/zeep/json/element.hpp
	include/zeep/json/factory.hpp
	include/zeep/json/serializer.hpp
	include/zeep/json/parser.hpp
	include/zeep/json/type_traits.hpp
	include/zeep/json/from_element.hpp
	include/zeep/json/element_fwd.hpp
	include/zeep/json/iterator.hpp
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
	include/zeep/http/template-processor.hpp
	include/zeep/http/header.hpp
	include/zeep/http/request.hpp
	include/zeep/http/rest-controller.hpp
	# include/zeep/http/soap-controller.hpp
	include/zeep/http/html-controller.hpp
	include/zeep/http/security.hpp
	include/zeep/http/uri.hpp
	include/zeep/http/el-processing.hpp
	include/zeep/http/login-controller.hpp
	include/zeep/unicode-support.hpp
	include/zeep/nvp.hpp
	include/zeep/exception.hpp
)

list(APPEND ZEEP_SRC
	lib-http/src/access-control.cpp
	lib-http/src/connection.cpp
	lib-http/src/controller.cpp
	lib-http/src/crypto.cpp
	lib-http/src/daemon.cpp
	lib-http/src/el-object.cpp
	lib-http/src/el-processing.cpp
	lib-http/src/error-handler.cpp
	lib-http/src/format.cpp
	lib-http/src/glob.cpp
	lib-http/src/html-controller.cpp
	lib-http/src/login-controller.cpp
	lib-http/src/message-parser.cpp
	lib-http/src/reply.cpp
	lib-http/src/request.cpp
	lib-http/src/rest-controller.cpp
	lib-http/src/security.cpp
	lib-http/src/server.cpp
	lib-http/src/uri.cpp
	# lib-http/src/soap-controller.cpp
	lib-http/src/tag-processor-v2.cpp
	lib-http/src/tag-processor.cpp
	lib-http/src/template-processor.cpp
	lib-http/src/signals.cpp
	lib-json/src/element.cpp
	lib-json/src/json-parser.cpp
)

if(USE_RSRC)
	list(APPEND ZEEP_SRC
		lib-http/src/controller-rsrc.cpp)
endif()

if(HTTP_SERVER_HAS_PREFORK)
	list(APPEND ZEEP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/preforked-server.hpp)
	list(APPEND ZEEP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib-http/src/preforked-server.cpp)
endif()

target_sources(zeep
	PRIVATE ${ZEEP_SRC}
	PUBLIC
	FILE_SET zeep_headers TYPE HEADERS
	BASE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
	${ZEEP_HEADERS}
)

set_target_properties(zeep PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(zeep PUBLIC cxx_std_20)

target_include_directories(zeep
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(zeep PUBLIC Threads::Threads date::date mxml::mxml)

if(ZEEP_USE_BOOST_ASIO)
	target_link_libraries(zeep PUBLIC Boost::boost)
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

# configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/zeep-config.cmake.in
# 	${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake
# 	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
# )

# write_basic_package_version_file(
# 	"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
# 	COMPATIBILITY AnyNewerVersion
# )

# file(GLOB OLD_CONFIG_FILES
# 	${CMAKE_INSTALL_FULL_LIBDIR}/cmake/zeep/zeepConfig*.cmake
# 	${CMAKE_INSTALL_FULL_LIBDIR}/cmake/zeep/zeepTargets*.cmake)

# if(OLD_CONFIG_FILES)
# 	message(STATUS "Installation will remove old config files: ${OLD_CONFIG_FILES}")
# 	install(CODE "file(REMOVE ${OLD_CONFIG_FILES})")
# endif()

# install(FILES
# 	"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake"
# 	"${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
# 	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zeep
# 	COMPONENT Devel
# )

# set(zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
# set_target_properties(zeep PROPERTIES
# 	VERSION ${PROJECT_VERSION}
# 	SOVERSION ${PROJECT_VERSION_MAJOR}
# 	INTERFACE_zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
# 	COMPATIBLE_INTERFACE_STRING zeep_MAJOR_VERSION
# )

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
get_directory_property(HAS_PARENT PARENT_DIRECTORY)

if(BUILD_TESTING AND NOT HAS_PARENT)
	add_subdirectory(test)
endif()

# add_subdirectory(examples)
