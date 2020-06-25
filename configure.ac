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

AX_BOOST_BASE([1.73])
AX_BOOST_ASIO
AX_BOOST_PROGRAM_OPTIONS

dnl Process Makefile.in to create Makefile
AC_OUTPUT([GNUmakefile
		   lib-xml/GNUmakefile
		   lib-json/GNUmakefile
		   lib-http/GNUmakefile
		   examples/GNUmakefile
		   lib-xml/libzeep-xml.pc
		   lib-json/libzeep-json.pc
		   lib-http/libzeep-http.pc])
