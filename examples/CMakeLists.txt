cmake_minimum_required(VERSION 3.16)

project(libzeep-examples LANGUAGES CXX)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The minimum version of C++ required for this library")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(zeep REQUIRED)

list(APPEND
	examples http-server-0 http-server-1 http-server-2
	serialize-xml synopsis-json synopsis-xml validating-xml-sample xpath-sample
	synopsis-el-1 security-sample rest-sample rest-sample-2 daemon-sample
	soap-sample)

foreach(EXAMPLE IN LISTS examples)
	add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
	target_link_libraries(${EXAMPLE} zeep::zeep)
endforeach()
