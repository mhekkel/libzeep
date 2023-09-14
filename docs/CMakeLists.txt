find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

# Find all the public headers
# get_target_property(ZEEP_PUBLIC_HEADER_DIR libzeep INTERFACE_INCLUDE_DIRECTORIES)
set(ZEEP_PUBLIC_HEADER_DIR ${PROJECT_SOURCE_DIR}/include/zeep)
file(GLOB_RECURSE ZEEP_PUBLIC_HEADERS ${ZEEP_PUBLIC_HEADER_DIR}/*.hpp)

set(DOXYGEN_INPUT_DIR ${ZEEP_PUBLIC_HEADER_DIR})
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/xml)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

#Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_command(
	OUTPUT ${DOXYGEN_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXYGEN_OUTPUT_DIR})

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
				   BYPRODUCTS ${DOXYGEN_OUTPUT_DIR}
                   DEPENDS ${ZEEP_PUBLIC_HEADERS}  ${DOXYGEN_OUTPUT_DIR} ${DOXYFILE_OUT}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Generating docs")

add_custom_target("Doxygen-${PROJECT_NAME}" ALL DEPENDS ${DOXYGEN_INDEX_FILE})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_SOURCE_DIR}/conf.py @ONLY)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

add_custom_target("Sphinx-${PROJECT_NAME}" ALL
                  COMMAND ${SPHINX_EXECUTABLE} -b html
				  -Dbreathe_projects.libzeep=${DOXYGEN_OUTPUT_DIR}
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
				  DEPENDS ${DOXYGEN_INDEX_FILE}
				  BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/api
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating documentation with Sphinx")

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/sphinx/ DESTINATION ${CMAKE_INSTALL_DOCDIR} PATTERN .doctrees EXCLUDE)
