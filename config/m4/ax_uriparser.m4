# SPDX-License-Identifier: BSD-2-Clause
# 
# Copyright (c) 2020 NKI/AVL, Netherlands Cancer Institute
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
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
#
# Description: m4 macro to detect std::filesystem and optionally the linker flags to use it
# 
# Description: Check for uriparser

AC_DEFUN([AX_URIPARSER],
[
	AC_ARG_WITH([uriparser],
		AS_HELP_STRING([--with-uriparser=@<:@location@:>@],
			[Use the uriparser library as specified.]),
			[
				AS_IF([test -d ${withval}/include], [], [
					AC_MSG_ERROR(['${withval}'' is not a valid directory for --with-uriparser])
				])

				URIPARSER_CFLAGS="-I ${withval}/include"
				URIPARSER_LIBS="-L${withval}/.libs -luriparser"

				AC_SUBST([URIPARSER_CFLAGS], [$URIPARSER_CFLAGS])
				AC_SUBST([URIPARSER_LIBS], [$URIPARSER_LIBS])
			])

	AS_IF([test "x$URIPARSER_LIBS" = "x"], [
		if test -x "$PKG_CONFIG"
		then
			AX_PKG_CHECK_MODULES([URIPARSER], [liburiparser >= 0.9.0], [], [], [AC_MSG_ERROR([the required package uriparser is not installed])])
		else
		    AC_REQUIRE([AC_CANONICAL_HOST])

			AS_CASE([${host_cpu}],
				[x86_64],[libsubdirs="lib64 libx32 lib lib64"],
				[ppc64|powerpc64|s390x|sparc64|aarch64|ppc64le|powerpc64le|riscv64],[libsubdirs="lib64 lib lib64"],
				[libsubdirs="lib"]
			)

			for _AX_URIPARSER_path in /usr /usr/local /opt /opt/local ; do
				if test -d "$_AX_URIPARSER_path/include/uriparser" && test -r "$_AX_URIPARSER_path/include/uriparser" ; then

					for libsubdir in $search_libsubdirs ; do
						if ls "$_AX_URIPARSER_path/$libsubdir/liburiparser"* >/dev/null 2>&1 ; then break; fi
					done
					URIPARSER_LDFLAGS="-L$_AX_URIPARSER_path/$libsubdir"
					URIPARSER_CFLAGS="-I$_AX_URIPARSER_path/include"
					break;
				fi
			done

			save_LDFLAGS=$LDFLAGS; LDFLAGS="$LDFLAGS $URIPARSER_LDFLAGS"
			save_CPPFLAGS=$CPPFLAGS; CPPFLAGS="$CPPFLAGS $URIPARSER_CFLAGS"

			AC_CHECK_HEADER(
				[uriparser/Uri.h],
				[],
				[AC_MSG_ERROR([
Can't find the uriparser header uriparser/Uri.h. Make sure that uriparser
is installed, and either use the --with-uriparser option or install pkg-config.])])

			AX_CHECK_LIBRARY([URIPARSER], [uriparser/Uri.h], [uriparser],
				[ LIBS="-luriparser $LIBS" ],
				[AC_MSG_ERROR([uriparser not found])])

			LDFLAGS=$save_LDFLAGS
			CPPFLAGS=$save_CPPFLAGS
		fi
	])
])