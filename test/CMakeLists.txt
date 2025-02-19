find_package(Catch2 QUIET)

if(NOT Catch2_FOUND)
	include(FetchContent)

	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG v3.8.0)

	FetchContent_MakeAvailable(Catch2)
endif()

find_package(libmcfp QUIET)

if(NOT libmcfp_FOUND)
	if(ZEEP_ALLOW_FETCH_CONTENT)
		include(FetchContent)

		FetchContent_Declare(libmcfp GIT_REPOSITORY https://github.com/mhekkel/libmcfp GIT_TAG v1.2.4)
		FetchContent_MakeAvailable(libmcfp)
	elseif(EXISTS libmcfp/CMakeLists.txt)
		add_subdirectory(libmcfp EXCLUDE_FROM_ALL)
	else()
		message(FATAL_ERROR "The libmcfp package was not found, please install https://github.com/mhekkel/libmcfp and try again")
	endif()
endif()

# # data files for the parser test
# add_library(client_test OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/client-test-code.cpp)

# target_include_directories(client_test PRIVATE
# 	${CMAKE_CURRENT_SOURCE_DIR}/../include
# )

# if(ZEEP_USE_BOOST_ASIO)
# 	target_link_libraries(client_test PRIVATE Boost::boost)
# endif()

# unit parser serializer xpath json crypto http processor webapp soap rest security uri
list(APPEND zeep_tests
	crypto
	http
	processor
	webapp
	json
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

	target_link_libraries(${ZEEP_TEST} PRIVATE zeep libmcfp::libmcfp Catch2::Catch2)

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(${ZEEP_TEST} PRIVATE /EHsc)
	endif()

	add_test(NAME ${ZEEP_TEST}
		COMMAND $<TARGET_FILE:${ZEEP_TEST}> --docroot ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()