cmake_minimum_required(VERSION 3.1)
project(libzeep CXX C)

include(GNUInstallDirs)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CMakePackageConfigHelpers)

set(PACKAGE "zeep")
set(PACKAGE_VERSION "5.0.0")

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_SHARED_LIBS "build shared libs (true)" TRUE)
option(MRC "build resources" FALSE)

add_definitions(-DVERSION=${PACKAGE_VERSION})

find_package(Threads REQUIRED)

find_package(Boost 1.65 REQUIRED
             program_options system date_time)
# extra diagnostic -- helpful for problem with FindBoost.cmake
message(STATUS "Boost headers in: ${Boost_INCLUDE_DIR}")
message(STATUS "Boost libraries in: ${Boost_LIBRARY_DIRS}")

include_directories(${Boost_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/include)

set(ZEEP_HEADERS
        ${CMAKE_SOURCE_DIR}/include/zeep/crypto.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/streambuf.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/value-serializer.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/config.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/xml/document.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/xml/parser.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/xml/xpath.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/xml/node.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/xml/character-classification.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/xml/serialize.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/xml/doctype.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/type-traits.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/to_element.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/element.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/factory.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/serializer.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/parser.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/type_traits.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/from_element.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/element_fwd.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/json/iterator.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/controller.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/message-parser.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/tag-processor.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/connection.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/server.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/preforked-server.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/daemon.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/error-handler.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/reply.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/template-processor.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/header.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/request.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/rest-controller.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/soap-controller.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/uri.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/html-controller.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/security.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/el-processing.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/http/login-controller.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/unicode-support.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/nvp.hpp
        ${CMAKE_SOURCE_DIR}/include/zeep/exception.hpp
)

set(ZEEP_SRC
        ${CMAKE_SOURCE_DIR}/lib-http/src/connection.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/controller.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/controller-rsrc.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/crypto.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/daemon.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/el-processing.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/error-handler.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/format.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/glob.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/html-controller.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/login-controller.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/message-parser.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/preforked-server.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/reply.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/request.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/rest-controller.cpp 
        ${CMAKE_SOURCE_DIR}/lib-http/src/security.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/server.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/soap-controller.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/tag-processor-v2.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/tag-processor.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/template-processor.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/signals.cpp
        ${CMAKE_SOURCE_DIR}/lib-http/src/uri.cpp
        ${CMAKE_SOURCE_DIR}/lib-json/src/element.cpp
        ${CMAKE_SOURCE_DIR}/lib-json/src/json-parser.cpp
        ${CMAKE_SOURCE_DIR}/lib-xml/src/character-classification.cpp
        ${CMAKE_SOURCE_DIR}/lib-xml/src/doctype.cpp
        ${CMAKE_SOURCE_DIR}/lib-xml/src/document.cpp
        ${CMAKE_SOURCE_DIR}/lib-xml/src/node.cpp
        ${CMAKE_SOURCE_DIR}/lib-xml/src/xml-parser.cpp
        ${CMAKE_SOURCE_DIR}/lib-xml/src/xpath.cpp
)

add_library(zeep
${ZEEP_SRC}
${ZEEP_HEADERS}
)

set_target_properties(zeep PROPERTIES SOVERSION ${PACKAGE_VERSION})

target_link_libraries(zeep PRIVATE
    ${Boost_LIBRARIES} 
    ${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS zeep
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_options(zeep PRIVATE -undefined dynamic_lookup)
endif (CMAKE_CXX_COMPILER_ID)

configure_file(${PROJECT_SOURCE_DIR}/include/zeep/revision.hpp.in revision.hpp @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/revision.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zeep/)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")

if(BUILD_SHARED_LIBS)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libzeep.pc.cmakein
   ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc.in @ONLY)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc
   INPUT ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc.in)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libzeep.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif(BUILD_SHARED_LIBS)

