# Copyright Maarten L. Hekkelman, 2025
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

find_package(Boost REQUIRED)

if(NOT (mcfp_FOUND OR TARGET mcfp))
	find_package(mcfp QUIET)

	if(NOT mcfp_FOUND)
		if(ZEEP_ALLOW_FETCH_CONTENT)
			include(FetchContent)

			FetchContent_Declare(mcfp GIT_REPOSITORY https://github.com/mhekkel/libmcfp GIT_TAG v1.3.3)
			FetchContent_MakeAvailable(mcfp)
		else()
			message(FATAL_ERROR "The mcfp package was not found, please install https://github.com/mhekkel/libmcfp and try again")
		endif()
	endif()
endif()

# data files for the parser test
add_library(client_test OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/http/client-test-code.cpp)

target_include_directories(client_test PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR} # for config.h
	${PROJECT_SOURCE_DIR}/include
)

if(ZEEP_USE_BOOST_ASIO)
	target_link_libraries(client_test PRIVATE Boost::boost)
endif()

# data files for the parser test
set(XML_CONF_TAR ${CMAKE_CURRENT_SOURCE_DIR}/xml/XML-Test-Suite.tbz)
set(XML_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/xml/XML-Test-Suite/xmlconf/xmlconf.xml)

if(NOT EXISTS ${XML_CONF_FILE})
	if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
		find_program(TAR tar)

		if(TAR)
			add_custom_command(OUTPUT ${XML_CONF_FILE}
				COMMAND ${TAR} xf ${XML_CONF_TAR}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/xml/)
		else()
			message(FATAL_ERROR "Please extract the archive ${XML_CONF_TAR} manually or update your cmake version to at least 3.18")
		endif()
	else()
		file(ARCHIVE_EXTRACT INPUT ${XML_CONF_TAR}
			DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/xml/)
	endif()
endif()

add_custom_target(XML_CONF ALL DEPENDS ${XML_CONF_FILE})

# unit parser serializer xpath json crypto http processor webapp soap rest security uri
list(APPEND zeep_tests
	xml/unit
	xml/parser
	xml/serializer
	xml/xpath
	json/json
	http/crypto
	http/http
	http/processor
	http/webapp
	http/soap
	http/rest
	http/security
	http/uri)

if(USE_RSRC)
	list(APPEND zeep_tests http/rsrc_webapp)
endif()

foreach(TEST IN LISTS zeep_tests)
	string(REGEX MATCH "^[^/]+" ZEEP_TEST_DIR ${TEST})
	string(REGEX MATCH "[^/]+$" ZEEP_TEST ${TEST})

	set(ZEEP_TEST "${ZEEP_TEST}-test")
	set(ZEEP_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${ZEEP_TEST_DIR}/${ZEEP_TEST}.cpp")

	add_executable(${ZEEP_TEST} ${ZEEP_TEST_SOURCE} $<TARGET_OBJECTS:client_test> ${ZEEP_TEST_RESOURCE})

	if("${TEST}" STREQUAL "http/processor")
		target_compile_definitions(${ZEEP_TEST} PUBLIC DOCROOT=".")
	endif()

	if(USE_RSRC AND("${TEST}" STREQUAL "http/processor" OR "${TEST}" STREQUAL "http/rsrc_webapp"))
		mrc_target_resources(${ZEEP_TEST} ${CMAKE_CURRENT_SOURCE_DIR}/http/fragment-file.xhtml)
	endif()

	target_include_directories(${ZEEP_TEST} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_BINARY_DIR} # for config.h
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/lib-http/src # for signals.hpp
	)

	target_link_libraries(${ZEEP_TEST} PRIVATE zeep ${STDCPPFS_LIBRARY} mcfp::mcfp)

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(${ZEEP_TEST} PRIVATE /EHsc)
	endif()

	add_custom_target("run-${ZEEP_TEST}" DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Run${ZEEP_TEST}.touch ${ZEEP_TEST})

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Run${ZEEP_TEST}.touch
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${ZEEP_TEST}
	)

	if("${TEST}" STREQUAL "xml/parser")
		# Some tests should be skipped
		list(APPEND ZEEP_TEST_ARGS
			ibm-valid-P28-ibm28v02.xml ibm-valid-P29-ibm29v01.xml ibm-valid-P29-ibm29v02.xml
			ibm-1-1-valid-P03-ibm03v09.xml rmt-e2e-34 rmt-e2e-55 rmt-054 rmt-ns10-006 rmt-e3e-13)
		list(TRANSFORM ZEEP_TEST_ARGS PREPEND "--questionable=")
		list(PREPEND ZEEP_TEST_ARGS ${XML_CONF_FILE} "--print-ids")
		add_dependencies(${ZEEP_TEST} XML_CONF)
	else()
		set(ZEEP_TEST_ARGS "")
	endif()

	add_test(NAME ${ZEEP_TEST}
		COMMAND $<TARGET_FILE:${ZEEP_TEST}> ${ZEEP_TEST_ARGS}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ZEEP_TEST_DIR})
endforeach()