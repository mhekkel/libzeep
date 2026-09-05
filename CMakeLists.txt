# SPDX-FileCopyrightText: 2023-2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.29)

# Experimental import std support
# if(${CMAKE_VERSION} VERSION_EQUAL 4.3.3)
#     set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

#     set(CMAKE_CXX_STANDARD_REQUIRED OFF)
#     set(CMAKE_CXX_MODULE_STD 1)
# endif()

project(libzeep VERSION 8.4.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CPM)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(CTest)
include(VersionString)

option(ZEEP_BUILD_CXX_MODULE "Build the library as a C++ 20 module library" ON)
option(ZEEP_USE_BOOST_ASIO "Use the asio library from Boost instead of the non-boost version" ON)
option(ZEEP_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEP_BUILD_EXAMPLES "Build the example applications" ON)

# New policy for Boost, set it here
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
    cmake_policy(SET CMP0167 NEW)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if(ZEEP_BUILD_CXX_MODULE)
        message(WARNING "Sorry, macOS has no proper support for module libraries at the moment")
        set(ZEEP_BUILD_CXX_MODULE OFF)
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if(ZEEP_BUILD_CXX_MODULE AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19)
        message(WARNING "Sorry, your compiler is too old, not building the module library")
        set(ZEEP_BUILD_CXX_MODULE OFF)
    endif()
    add_compile_options(-Wall -Wextra)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(ZEEP_BUILD_CXX_MODULE AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 16)
        message(WARNING "Sorry, your compiler is too old, not building the module library")
        set(ZEEP_BUILD_CXX_MODULE OFF)
    endif()
    add_compile_options(-Wall -Wextra -Wno-bidi-chars)
elseif(MSVC)
    add_compile_options(/W4)
    if(ZEEP_BUILD_CXX_MODULE)
        message(WARNING "Sorry, adding asio to a module library is not working in Windows")
        set(ZEEP_BUILD_CXX_MODULE OFF)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    if(ZEEP_BUILD_CXX_MODULE AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 20)
        message(WARNING "Sorry, your compiler is too old, not building the module library")
        set(ZEEP_BUILD_CXX_MODULE OFF)
    endif()
endif()

if(WIN32)
    # Windows specific stuff
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

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
    message(STATUS "zeep: Not using resources when building shared libraries for Windows")
else()
    find_package(mrc QUIET)

    if(MRC_FOUND)
        option(USE_RSRC "Use mrc to create resources" ON)

        if(USE_RSRC)
            message(STATUS "zeep: Using resources compiled with ${MRC_EXECUTABLE} version ${MRC_VERSION_STRING}")
            set(WEBAPP_USES_RESOURCES 1)
        endif()
    else()
        message(STATUS "zeep: Not using resources since mrc was not found")
    endif()
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# OpenSSL is now used in the client code
find_package(OpenSSL REQUIRED)

if(ZEEP_USE_BOOST_ASIO)
    find_package(Boost 1.74 REQUIRED COMPONENTS headers)

    message(STATUS "zeep: Using asio from boost")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio-boost.hpp.in
        ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp
        @ONLY)

    set(FIND_DEPENDENCY_BOOST "find_dependency(Boost REQUIRED)")
else()
    find_path(ASIO_INCLUDE_DIR asio.hpp)

    if(ASIO_INCLUDE_DIR STREQUAL "ASIO_INCLUDE_DIR-NOTFOUND")
        CPMAddPackage(
            NAME asio
            URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-36-0.tar.gz)

        set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)
    endif()

    message(STATUS "zeep: Using asio from ${ASIO_INCLUDE_DIR}")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio.hpp.in
        ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp
        @ONLY)
endif()

try_run(TEST_STD_CHRONO_ZONED_TIME_R TEST_STD_CHRONO_ZONED_TIME_C
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-chrono-zoned_time.cpp
    CXX_STANDARD 20)

if("${TEST_STD_CHRONO_ZONED_TIME_R}" EQUAL "0")
    set(USE_STD_CHRONO_ZONED_TIME TRUE)
else()
    set(USE_STD_CHRONO_ZONED_TIME FALSE)
endif()

if(NOT USE_STD_CHRONO_ZONED_TIME)
    message(NOTICE "zeep: Using Howard Hinnants date for from_stream")
    CPMFindPackage(
        NAME date
        GIT_REPOSITORY "https://github.com/HowardHinnant/date"
        VERSION 3.0.3
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "USE_SYSTEM_TZ_DB ON"
        "BUILD_TZ_LIB ON")
    add_compile_definitions(USE_DATE_H)
endif()

CPMFindPackage(NAME zeem
    GIT_REPOSITORY "https://forge.hekkelman.net/maarten/zeem.git"
    GIT_TAG v2.4.1
    EXCLUDE_FROM_ALL YES)

if(UNIX)
    find_file(HAVE_UNISTD_H "unistd.h")

    if(HAVE_UNISTD_H)
        set(HTTP_HAS_UNIX_DAEMON 1)
    endif()
endif()

check_function_exists(explicit_bzero HAVE_EXPLICIT_BZERO)


# Config file
set(LIBZEEP_VERSION ${PROJECT_VERSION})
set(LIBZEEP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(LIBZEEP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(LIBZEEP_VERSION_PATCH ${PROJECT_VERSION_PATCH})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/config.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/config.hpp"
    @ONLY)

# Generate the revision.hpp file
write_version_header("${CMAKE_CURRENT_SOURCE_DIR}/src" LIB_NAME "libzeep")

# Library rules
file(GLOB_RECURSE zeep_headers include/*.hpp)
file(GLOB_RECURSE zeep_modules include/*.cppm)

file(GLOB_RECURSE zeep_cpp src/*.cpp)
file(GLOB_RECURSE zeep_cxx src/*.cxx)

list(APPEND zeep_headers include/zeep/config.hpp)

if(NOT USE_RSRC)
    list(FILTER zeep_cpp EXCLUDE REGEX src/controller-rsrc.cpp)
    list(FILTER zeep_cxx EXCLUDE REGEX src/controller-rsrc.cxx)
endif()

add_library(zeep ${zeep_cpp})
add_library(zeep::zeep ALIAS zeep)

target_compile_features(zeep PUBLIC cxx_std_23)

if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(zeep PRIVATE ZEEP_SHARED_BUILD)
    set_target_properties(zeep PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# The classic target
target_sources(zeep
    PUBLIC
    FILE_SET zeep_classic TYPE HEADERS
    BASE_DIRS include
    FILES ${zeep_headers})

target_include_directories(zeep PUBLIC "$<BUILD_INTERFACE:${ASIO_INCLUDE_DIR}>")
target_link_libraries(zeep PUBLIC Threads::Threads zeem::zeem OpenSSL::SSL OpenSSL::Crypto)

if(NOT USE_STD_CHRONO_ZONED_TIME)
    target_link_libraries(zeep PUBLIC date::date-tz)
endif()

if(ZEEP_USE_BOOST_ASIO)
    target_link_libraries(zeep PUBLIC Boost::headers)
endif()

set_target_properties(zeep PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(zeep PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
    COMPATIBLE_INTERFACE_STRING zeep_MAJOR_VERSION)

install(TARGETS zeep
    EXPORT zeep-targets
    FILE_SET zeep_classic
    DESTINATION include)

install(EXPORT zeep-targets
    NAMESPACE zeep::
    DESTINATION lib/cmake/zeep)

if(ZEEP_BUILD_CXX_MODULE)
    if(BUILD_SHARED_LIBS)
        add_library(zeep-module SHARED ${zeep_cxx})

        if(WIN32)
            target_compile_definitions(zeep-module PRIVATE ZEEP_SHARED_BUILD)
            set_target_properties(zeep-module PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
        endif()
    else()
        add_library(zeep-module STATIC ${zeep_cxx})
    endif()
    add_library(zeep::zeep-module ALIAS zeep-module)

    target_compile_features(zeep-module PUBLIC cxx_std_23)

    target_sources(zeep-module
        PUBLIC
        FILE_SET zeep_module TYPE CXX_MODULES
        BASE_DIRS include
        FILES ${zeep_modules})

    target_compile_definitions(zeep-module PUBLIC ZEEP_CXX_MODULE)
    target_include_directories(zeep-module PUBLIC "$<BUILD_INTERFACE:${ASIO_INCLUDE_DIR}>")

    target_link_libraries(zeep-module PUBLIC Threads::Threads zeem::zeem-module OpenSSL::SSL OpenSSL::Crypto)

    if(NOT USE_STD_CHRONO_ZONED_TIME)
        target_link_libraries(zeep-module PUBLIC date::date-tz)
    endif()

    if(ZEEP_USE_BOOST_ASIO)
        target_link_libraries(zeep-module PUBLIC Boost::headers)
    endif()

	set_target_properties(zeep-module PROPERTIES POSITION_INDEPENDENT_CODE ON)

    set_target_properties(zeep-module PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        INTERFACE_zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
        COMPATIBLE_INTERFACE_STRING zeep_MAJOR_VERSION)

    target_include_directories(zeep-module
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/zeep>)

    install(TARGETS zeep-module
        EXPORT zeep-module-targets
        CXX_MODULES_BMI DESTINATION "lib/cxx/bmi"
        FILE_SET zeep_module DESTINATION "lib/cxx/miu")

    install(EXPORT zeep-module-targets
        NAMESPACE zeep::
        DESTINATION lib/cmake/zeep
        FILE "zeep-module-targets.cmake")

endif()

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/zeep-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake
    INSTALL_DESTINATION lib/cmake/zeep)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
    DESTINATION lib/cmake/zeep
    COMPONENT Devel)


# Test applications, but only if we're not included as a subproject
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()

# Examples
if(ZEEP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
if(ZEEP_BUILD_DOCUMENTATION)
    add_subdirectory(docs)
endif()

