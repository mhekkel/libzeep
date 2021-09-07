# SPDX-License-Identifier: BSD-2-Clause

# Copyright (c) 2021 Maarten L. Hekkelman

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#[=======================================================================[.rst:
FindMrc
-------

The module defines the following variables:

``MRC_EXECUTABLE``
  Path to mrc executable.
``mrc_FOUND``, ``MRC_FOUND``
  True if the mrc executable was found.
``MRC_VERSION_STRING``
  The version of mrc found.

Additionally, the following commands will be added:
  :command:`mrc_write_header`
  :command:`mrc_target_resource`

.. versionadded:: 3.14
  The module defines the following ``IMPORTED`` targets (when
  :prop_gbl:`CMAKE_ROLE` is ``PROJECT``):

``mrc::mrc``
  the mrc executable.

Example usage:

.. code-block:: cmake

   find_package(Mrc)
   if(mrc_FOUND)
     message("mrc found: ${MRC_EXECUTABLE}")

     mrc_write_header(${CMAKE_CURRENT_BINARY_DIR}/mrsrc.hpp)
     mrc_target_resource(my-app rsrc/hello-world.txt)
   endif()
#]=======================================================================]

find_program(MRC_EXECUTABLE
    NAMES mrc
    DOC "mrc executable"
)

if(CMAKE_HOST_WIN32)
    find_program(MRC_EXECUTABLE
        NAMES mrc
		PATHS $ENV{LOCALAPPDATA}
        PATH_SUFFIXES mrc/bin
        DOC "mrc executable"
    )
endif()

mark_as_advanced(MRC_EXECUTABLE)

# Retrieve the version number

execute_process(COMMAND ${MRC_EXECUTABLE} --version
                OUTPUT_VARIABLE mrc_version
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if (mrc_version MATCHES "^mrc version [0-9]")
    string(REPLACE "mrc version " "" MRC_VERSION_STRING "${mrc_version}")
endif()
unset(mrc_version)

find_package(PackageHandleStandardArgs REQUIRED)
find_package_handle_standard_args(Mrc
	REQUIRED_VARS MRC_EXECUTABLE
	VERSION_VAR MRC_VERSION_STRING)

#[=======================================================================[.rst:
.. command:: mrc_target_resources

  Add resources to a target. The first argument should be the target name,
  the rest of the arguments are passed to mrc to pack into a resource 
  object file.
#]=======================================================================]

function(mrc_target_resources _target)

    set(RSRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${_target}_rsrc.obj")

	if(CMAKE_HOST_WIN32)
		# Find out the processor type for the target
		if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
			set(COFF_TYPE "x64")
		elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i386")
			set(COFF_TYPE "x86")
		elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ARM64")
			set(COFF_TYPE "arm64")
		else()
			message(FATAL_ERROR "Unsupported or unknown processor type ${CMAKE_SYSTEM_PROCESSOR}")
		endif()

		set(COFF_SPEC "--coff=${COFF_TYPE}")
	endif()

    add_custom_command(OUTPUT ${RSRC_FILE}
        COMMAND ${MRC_EXECUTABLE} -o ${RSRC_FILE} ${ARGN} ${COFF_SPEC})

    target_sources(${_target} PRIVATE ${RSRC_FILE})
endfunction()

#[=======================================================================[.rst:
.. command:: mrc_write_header

  Write out the header file needed to use resources in a C++ application.
  The argument specifies the file to create.
#]=======================================================================]

function(mrc_write_header _header)
    execute_process(
        COMMAND ${MRC_EXECUTABLE} --header --output ${_header}
    )
endfunction()
