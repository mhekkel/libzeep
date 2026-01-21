#         Copyright Maarten L. Hekkelman, 2022-2026
#  Distributed under the Boost Software License, Version 1.0.
#     (See accompanying file LICENSE_1_0.txt or copy at
#           http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.16)

project(libzeep-examples LANGUAGES CXX)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only try to find package if we're not included from the main libzeep CMakeLists.txt
get_property(PARENT_DIRECTORY DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" PROPERTY PARENT_DIRECTORY)

if("${PARENT_DIRECTORY}" STREQUAL "")
	find_package(zeep REQUIRED)
endif()

list(APPEND examples http-server-0 http-server-1 http-server-2 http-server-3
	security-sample rest-sample rest-sample-2 # soap-sample
	)

if (ZEEP_HAS_UNIX_DAEMON)
	list(APPEND examples daemon-sample)
endif()

foreach(EXAMPLE IN LISTS examples)
	add_executable(${EXAMPLE} ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE}.cpp)
	target_link_libraries(${EXAMPLE} zeep::zeep)
endforeach()
