AC_INIT([libzeep], 5.0, [maarten@hekkelman.com])

dnl Switch to a C++ compiler, and check if it works.
AC_LANG(C++)
AX_CXX_COMPILE_STDCXX_17([noext])

AC_CONFIG_SRCDIR([lib-http/src/connection.cpp])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config/m4])

AC_PREFIX_DEFAULT(/usr/local)

AC_PROG_INSTALL
AC_ENABLE_STATIC
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AC_PATH_PROG([SHELL], [sh])

AC_ARG_VAR([DEBUG], [Build a debug version of the library])

AC_ARG_VAR([MRC], [Specify a location for the mrc executable])

dnl We really want to use mrc
if test "x$MRC" = "x"; then
	AC_PATH_PROG([MRC], [mrc])
fi

if test "x$MRC" = "x"; then
	AC_MSG_WARN([mrc not found, building without support for resources.])
fi

AX_PTHREAD(
	[
		LIBS="$PTHREAD_LIBS $LIBS"
		CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
	]
)

AC_CHECK_HEADER([filesystem], [], [AC_MSG_ERROR([The file <filesystem> is missing, perhaps you should install a more recent libstdc++ implementation.])])

dnl check if we need stdc++fs as library
AC_TRY_LINK(
	[#include <filesystem>],
	[(void)std::filesystem::current_path();],
	[],
	[
		LIBS="$LIBS -lstdc++fs"

		AC_TRY_LINK(
			[#include <filesystem>],
			[(void)std::filesystem::current_path();],
			[],
			[
				AC_MSG_ERROR([Could not link filesystem])
			]
		)
	]
)

AX_BOOST_BASE([1.65.1], [], [
	AC_MSG_ERROR([Sorry, your boost is not found or not up-to-date.])
])
AX_BOOST_ASIO
AX_BOOST_PROGRAM_OPTIONS

BUILD_DIR="$abs_top_builddir"

dnl Process Makefile.in to create Makefile
AC_OUTPUT([GNUmakefile
		   lib-xml/GNUmakefile
		   lib-json/GNUmakefile
		   lib-http/GNUmakefile
		   examples/GNUmakefile
		   lib-xml/libzeep-xml.pc
		   lib-json/libzeep-json.pc
		   lib-http/libzeep-http.pc])
