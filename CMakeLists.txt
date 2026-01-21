# Copyright Maarten L. Hekkelman, 2023-2026
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.28)

project(libzeep VERSION 7.3.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakePackageConfigHelpers)
include(CPM)
include(CTest)
include(VersionString)

option(ZEEP_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEP_USE_BOOST_ASIO "Use the asio library from Boost instead of the non-boost version" ON)
option(ZEEP_BUILD_EXAMPLES "Build the example applications" ON)

# New policy for Boost, set it here
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
    cmake_policy(SET CMP0167 NEW)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
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

if(ZEEP_USE_BOOST_ASIO)
    find_package(Boost 1.74 REQUIRED COMPONENTS headers)

    message(STATUS "zeep: Using asio from boost")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio-boost.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)

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

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/asio.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/zeep/http/asio.hpp @ONLY)
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
    GIT_REPOSITORY "https://github.com/mhekkel/zeem.git"
    VERSION 2.1.0
    EXCLUDE_FROM_ALL YES)

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
file(GLOB_RECURSE zeep_cpp src/*.cpp)
file(GLOB_RECURSE zeep_hpp include/*.hpp)

# TODO: SOAP is not working at the moment, really need to fix that
list(FILTER zeep_cpp EXCLUDE REGEX src/soap-controller.cpp)

if(NOT USE_RSRC)
    list(FILTER zeep_cpp EXCLUDE REGEX src/controller-rsrc.cpp)
endif()

if(NOT HTTP_SERVER_HAS_PREFORK)
    list(FILTER zeep_hpp EXCLUDE REGEX zeep/http/preforked-server.hpp)
    list(FILTER zeep_cpp EXCLUDE REGEX src/preforked-server.cpp)
endif()

add_library(zeep ${zeep_cpp})
add_library(zeep::zeep ALIAS zeep)

target_sources(zeep
    PUBLIC FILE_SET zeep_headers TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES ${zeep_hpp})

set_target_properties(zeep PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(zeep PUBLIC cxx_std_20)
target_include_directories(zeep PUBLIC "${ASIO_INCLUDE_DIR}")
target_link_libraries(zeep PUBLIC Threads::Threads zeem::zeem)

if(NOT USE_STD_CHRONO_ZONED_TIME)
    target_link_libraries(zeep PUBLIC date::date-tz)
endif()

if(ZEEP_USE_BOOST_ASIO)
    target_link_libraries(zeep PUBLIC Boost::headers)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_options(zeep PRIVATE -undefined dynamic_lookup)
endif()

# Install rules
install(TARGETS zeep
    EXPORT zeep-targets
    FILE_SET zeep_headers
    DESTINATION include)

install(EXPORT zeep-targets
    NAMESPACE zeep::
    DESTINATION lib/cmake/zeep)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/zeep-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake
    INSTALL_DESTINATION lib/cmake/zeep)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
    COMPATIBILITY AnyNewerVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/zeep/zeep-config-version.cmake"
    DESTINATION lib/cmake/zeep
    COMPONENT Devel)

set(zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(zeep PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_zeep_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
    COMPATIBLE_INTERFACE_STRING zeep_MAJOR_VERSION)

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
