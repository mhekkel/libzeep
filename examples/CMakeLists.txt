# SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.29)

project(libzeep-examples LANGUAGES CXX)

# Only try to find package if we're not included from the main libzeep CMakeLists.txt
if(PROJECT_IS_TOP_LEVEL)
    find_package(zeep REQUIRED)
endif()

if(UNIX)
    find_file(HAVE_UNISTD_H "unistd.h")

    if(HAVE_UNISTD_H)
        set(HTTP_HAS_UNIX_DAEMON 1)
    endif()
endif()

list(APPEND examples
	http-server-0
	http-server-1
	http-server-2
	http-server-3
    security-sample
	rest-sample
	rest-sample-2
	soap-sample
)

if(HTTP_HAS_UNIX_DAEMON)
    list(APPEND examples daemon-sample)
endif()

foreach(EXAMPLE IN LISTS examples)
    add_executable(${EXAMPLE} ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE}.cpp)

    if(ZEEP_BUILD_CXX_MODULE)
        target_link_libraries(${EXAMPLE} zeep::zeep-module)
    else()
        target_link_libraries(${EXAMPLE} zeep::zeep)
    endif()
endforeach()
