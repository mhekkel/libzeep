#           Copyright Maarten L. Hekkelman, 2022
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
	serialize-xml synopsis-json synopsis-xml validating-xml-sample xpath-sample
	synopsis-el-1 security-sample rest-sample rest-sample-2 daemon-sample
	soap-sample)

foreach(EXAMPLE IN LISTS examples)
	add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
	target_link_libraries(${EXAMPLE} zeep::zeep)
endforeach()
