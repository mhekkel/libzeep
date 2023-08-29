find_package(Sphinx REQUIRED)

# Find all the public headers
# get_target_property(ZEEP_PUBLIC_HEADER_DIR libmcfp INTERFACE_INCLUDE_DIRECTORIES)
set(ZEEP_PUBLIC_HEADER_DIR ${PROJECT_SOURCE_DIR}/include/zeep)
file(GLOB_RECURSE ZEEP_PUBLIC_HEADERS ${ZEEP_PUBLIC_HEADER_DIR}/*.hpp)
file(GLOB_RECURSE ZEEP_DOC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.md ${CMAKE_CURRENT_SOURCE_DIR}/*.rst)
file(GLOB_RECURSE STATIC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/_static/*)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

add_custom_target(Sphinx ALL
                  COMMAND
                  ${SPHINX_EXECUTABLE} -b html
				  -Dbreathe_projects.libzeep=${CMAKE_CURRENT_BINARY_DIR}/doxygen/xml
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
				  DEPENDS ${ZEEP_PUBLIC_HEADERS} ${ZEEP_DOC_SOURCES}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating documentation with Sphinx")
