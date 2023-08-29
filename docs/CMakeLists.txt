find_package(Sphinx REQUIRED)

# Find all the public headers
# get_target_property(MCFP_PUBLIC_HEADER_DIR libmcfp INTERFACE_INCLUDE_DIRECTORIES)
set(MCFP_PUBLIC_HEADER_DIR ${PROJECT_SOURCE_DIR}/include/mcfp)
file(GLOB_RECURSE MCFP_PUBLIC_HEADERS ${MCFP_PUBLIC_HEADER_DIR}/*.hpp)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

add_custom_target(Sphinx ALL
                  COMMAND
                  ${SPHINX_EXECUTABLE} -b html
				  -Dbreathe_projects.libzeep=${CMAKE_CURRENT_BINARY_DIR}/doxygen/xml
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
				  DEPENDS ${MCFP_PUBLIC_HEADERS}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating documentation with Sphinx")
