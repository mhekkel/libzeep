#          Copyright Maarten L. Hekkelman 2025
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

CPMAddPackage(
	NAME Catch2
	GITHUB_REPOSITORY "catchorg/Catch2"
	VERSION 3.4.0)

CPMAddPackage(
	NAME mcfp
	GITHUB_REPOSITORY "mhekkel/libmcfp"
	VERSION 1.3.5)

# unit parser serializer xpath json crypto http processor webapp soap rest security uri
list(APPEND zeep_tests
	crypto
	el
	http
	processor
	webapp
	# soap
	rest
	security
	uri
)

if(USE_RSRC)
	list(APPEND zeep_tests rsrc_webapp)
endif()

foreach(TEST IN LISTS zeep_tests)
	set(ZEEP_TEST "${TEST}-test")
	set(ZEEP_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${ZEEP_TEST}.cpp")

	add_executable(${ZEEP_TEST})
	target_sources(${ZEEP_TEST}
		PRIVATE
		${ZEEP_TEST_SOURCE}
		${CMAKE_CURRENT_SOURCE_DIR}/test-main.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/client-test-code.cpp)

	if("${ZEEP_TEST}" STREQUAL "processor")
		target_compile_definitions(${ZEEP_TEST} PUBLIC DOCROOT=".")
	endif()

	if(USE_RSRC AND("${TEST}" STREQUAL "processor" OR "${TEST}" STREQUAL "rsrc_webapp"))
		mrc_target_resources(${ZEEP_TEST} ${CMAKE_CURRENT_SOURCE_DIR}/fragment-file.xhtml)
	endif()

	target_link_libraries(${ZEEP_TEST} PRIVATE zeep mcfp::mcfp Catch2::Catch2)

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(${ZEEP_TEST} PRIVATE /EHsc)
	endif()

	add_test(NAME ${ZEEP_TEST}
		COMMAND $<TARGET_FILE:${ZEEP_TEST}> --docroot ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()