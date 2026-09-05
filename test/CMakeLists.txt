# SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSL-1.0

CPMFindPackage(
    NAME Catch2
    GIT_REPOSITORY "https://github.com/catchorg/Catch2"
    VERSION 3.4.0
    EXCLUDE_FROM_ALL YES)

# unit parser serializer xpath json crypto http processor webapp soap rest security uri
list(APPEND zeep_tests
    crypto
    el
    fuzz-parser
    fuzz-el
    fuzz-json
    fuzz-request-multipart
    http
    processor
    webapp
    soap
    rest
    security
    uri
)

if(HTTP_HAS_UNIX_DAEMON)
    list(APPEND zeep_tests daemon)
endif()

if(USE_RSRC)
    list(APPEND zeep_tests rsrc_webapp)
endif()

add_library(zeep-test-lib OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/test-main.cpp)

if(ZEEP_BUILD_CXX_MODULE)
    target_link_libraries(zeep-test-lib zeep::zeep-module Catch2::Catch2)
else()
    target_link_libraries(zeep-test-lib zeep::zeep Catch2::Catch2)
endif()

foreach(TEST IN LISTS zeep_tests)
    set(ZEEP_TEST "${TEST}-test")
    set(ZEEP_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${ZEEP_TEST}.cpp")

    add_executable(${ZEEP_TEST})
    target_sources(${ZEEP_TEST} PRIVATE ${ZEEP_TEST_SOURCE})

    if(USE_RSRC AND ("${TEST}" STREQUAL "processor" OR "${TEST}" STREQUAL "rsrc_webapp"))
        mrc_target_resources(${ZEEP_TEST} ${CMAKE_CURRENT_SOURCE_DIR}/fragment-file.xhtml)
    endif()

    target_link_libraries(${ZEEP_TEST} PRIVATE zeep-test-lib)

    if(MSVC)
        # Specify unwind semantics so that MSVC knowns how to handle exceptions
        target_compile_options(${ZEEP_TEST} PRIVATE /EHsc)
    endif()

    add_test(NAME ${ZEEP_TEST}
        COMMAND $<TARGET_FILE:${ZEEP_TEST}> --docroot ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
