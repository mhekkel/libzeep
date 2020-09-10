#
# SYNOPSIS
#
#   AX_ZEEP_XML
#
# DESCRIPTION
#
#   Test for xml library from the libzeep libraries. The macro requires
#   a preceding call to AX_ZEEP_BASE.
#
#   This macro calls:
#
#     AC_SUBST(ZEEP_XML_LIB)
#
#   And sets:
#
#     HAVE_ZEEP_XML
#
# LICENSE
#
#   Copyright (c) 2020 Maarten L. Hekkelman (maarten@hekkelman.com)
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

AC_DEFUN([AX_ZEEP_BASE],
[
AC_ARG_WITH([zeep],
  [AS_HELP_STRING([--with-zeep@<:@=ARG@:>@],
    [use libzeep library from a standard location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@ ])],
    [
     AS_CASE([$withval],
       [no],[want_zeep="no";_AX_ZEEP_path=""],
       [yes],[want_zeep="yes";_AX_ZEEP_path=""],
       [want_zeep="yes";_AX_ZEEP_path="$withval"])
    ],
    [want_zeep="yes"])
])

AC_DEFUN([AX_ZEEP_XML],
[
	AC_ARG_WITH([zeep-xml],
	AS_HELP_STRING([--with-zeep-xml@<:@=special-lib@:>@],
                   [use the xml library from libzeep - it is possible to specify a certain library for the linker
                        e.g. --with-zeep-xml=zeep-xml-gcc-mt-d ]),
        [
        if test "$withval" = "no"; then
			want_zeep="no"
        elif test "$withval" = "yes"; then
            want_zeep="yes"
            ax_zeep_user_xml_lib=""
        else
		    want_zeep="yes"
			ax_zeep_user_xml_lib="$withval"
		fi
        ],
        [want_zeep="yes"]
	)

	if test "x$want_zeep" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS -I ${_AX_ZEEP_path}/include"
		export CPPFLAGS

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS -L ${_AX_ZEEP_path}/lib"
		export LDFLAGS

        AC_CACHE_CHECK(whether the zeep::xml library is available,
					   ax_cv_zeep_xml,
        [AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <zeep/xml/node.hpp>
												]],
                                   [[zeep::xml::element n("test"); return 0;]])],
                   ax_cv_zeep_xml=yes, ax_cv_zeep_xml=no)
         AC_LANG_POP([C++])
		])
		if test "x$ax_cv_zeep_xml" = "xyes"; then
			AC_DEFINE(HAVE_ZEEP_XML,,[define if the zeep::xml library is available])
            ZEEPLIBDIR="${_AX_ZEEP_path}/lib"

            if test "x$ax_zeep_user_xml_lib" = "x"; then
                for libextension in `ls $ZEEPLIBDIR/libzeep-xml*.so* $ZEEPLIBDIR/libzeep-xml*.dylib* $ZEEPLIBDIR/libzeep-xml*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^lib\(zeep-xml.*\)\.so.*$;\1;' -e 's;^lib\(zeep-xml.*\)\.dylib.*;\1;' -e 's;^lib\(zeep-xml.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [ZEEP_XML_LIB="-l$ax_lib"; AC_SUBST(ZEEP_XML_LIB) link_zeep_xml="yes"; break],
                                 [link_zeep_xml="no"])
				done
                if test "x$link_zeep_xml" != "xyes"; then
                for libextension in `ls $ZEEPLIBDIR/zeep-xml*.dll* $ZEEPLIBDIR/zeep-xml*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^\(zeep-xml.*\)\.dll.*$;\1;' -e 's;^\(zeep-xml.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [ZEEP_XML_LIB="-l$ax_lib"; AC_SUBST(ZEEP_XML_LIB) link_zeep_xml="yes"; break],
                                 [link_zeep_xml="no"])
				done
                fi
            else
               for ax_lib in $ax_zeep_user_xml_lib zeep-xml; do
				      AC_CHECK_LIB($ax_lib, main,
                                   [ZEEP_XML_LIB="-l$ax_lib"; AC_SUBST(ZEEP_XML_LIB) link_zeep_xml="yes"; break],
                                   [link_zeep_xml="no"])
               done
            fi

            if test "x$ax_lib" = "x"; then
                AC_MSG_ERROR(Could not find a version of the zeep::xml library!)
            fi
			if test "x$link_zeep_xml" != "xyes"; then
				AC_MSG_ERROR(Could not link against $ax_lib !)
			fi
		fi

		CPPFLAGS="$CPPFLAGS_SAVED"
		LDFLAGS="$LDFLAGS_SAVED"
		LIBS="$LIBS $ZEEP_XML_LIB"
	fi
])

AC_DEFUN([AX_ZEEP_JSON],
[
	AC_ARG_WITH([zeep-json],
	AS_HELP_STRING([--with-zeep-json@<:@=special-lib@:>@],
                   [use the json library from libzeep - it is possible to specify a certain library for the linker
                        e.g. --with-zeep-json=zeep-json-gcc-mt-d ]),
        [
        if test "$withval" = "no"; then
			want_zeep="no"
        elif test "$withval" = "yes"; then
            want_zeep="yes"
            ax_zeep_user_json_lib=""
        else
		    want_zeep="yes"
			ax_zeep_user_json_lib="$withval"
		fi
        ],
        [want_zeep="yes"]
	)

	if test "x$want_zeep" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS -I ${_AX_ZEEP_path}/include"
		export CPPFLAGS

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS -L ${_AX_ZEEP_path}/lib"
		export LDFLAGS

        AC_CACHE_CHECK(whether the zeep::json library is available,
					   ax_cv_zeep_json,
        [AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <zeep/json/element.hpp>
												]],
                                   [[zeep::json::element n; return 0;]])],
                   ax_cv_zeep_json=yes, ax_cv_zeep_json=no)
         AC_LANG_POP([C++])
		])
		if test "x$ax_cv_zeep_json" = "xyes"; then
			AC_DEFINE(HAVE_ZEEP_JSON,,[define if the zeep::json library is available])
            ZEEPLIBDIR="${_AX_ZEEP_path}/lib"

            if test "x$ax_zeep_user_json_lib" = "x"; then
                for libextension in `ls $ZEEPLIBDIR/libzeep-json*.so* $ZEEPLIBDIR/libzeep-json*.dylib* $ZEEPLIBDIR/libzeep-json*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^lib\(zeep-json.*\)\.so.*$;\1;' -e 's;^lib\(zeep-json.*\)\.dylib.*;\1;' -e 's;^lib\(zeep-json.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [ZEEP_JSON_LIB="-l$ax_lib"; AC_SUBST(ZEEP_JSON_LIB) link_zeep_json="yes"; break],
                                 [link_zeep_json="no"])
				done
                if test "x$link_zeep_json" != "xyes"; then
                for libextension in `ls $ZEEPLIBDIR/zeep-json*.dll* $ZEEPLIBDIR/zeep-json*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^\(zeep-json.*\)\.dll.*$;\1;' -e 's;^\(zeep-json.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [ZEEP_JSON_LIB="-l$ax_lib"; AC_SUBST(ZEEP_JSON_LIB) link_zeep_json="yes"; break],
                                 [link_zeep_json="no"])
				done
                fi
            else
               for ax_lib in $ax_zeep_user_json_lib zeep-json; do
				      AC_CHECK_LIB($ax_lib, main,
                                   [ZEEP_JSON_LIB="-l$ax_lib"; AC_SUBST(ZEEP_JSON_LIB) link_zeep_json="yes"; break],
                                   [link_zeep_json="no"])
               done
            fi

            if test "x$ax_lib" = "x"; then
                AC_MSG_ERROR(Could not find a version of the zeep::json library!)
            fi
			if test "x$link_zeep_json" != "xyes"; then
				AC_MSG_ERROR(Could not link against $ax_lib !)
			fi
		fi

		CPPFLAGS="$CPPFLAGS_SAVED"
		LDFLAGS="$LDFLAGS_SAVED"
		LIBS="$ZEEP_JSON_LIB $LIBS"
	fi
])

AC_DEFUN([AX_ZEEP_HTTP],
[
	AC_ARG_WITH([zeep-http],
	AS_HELP_STRING([--with-zeep-http@<:@=special-lib@:>@],
                   [use the http library from libzeep - it is possible to specify a certain library for the linker
                        e.g. --with-zeep-http=zeep-http-gcc-mt-d ]),
        [
        if test "$withval" = "no"; then
			want_zeep="no"
        elif test "$withval" = "yes"; then
            want_zeep="yes"
            ax_zeep_user_http_lib=""
        else
		    want_zeep="yes"
			ax_zeep_user_http_lib="$withval"
		fi
        ],
        [want_zeep="yes"]
	)

	if test "x$want_zeep" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS -I ${_AX_ZEEP_path}/include"
		export CPPFLAGS

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS -L ${_AX_ZEEP_path}/lib"
		export LDFLAGS

        AC_CACHE_CHECK(whether the zeep::http library is available,
					   ax_cv_zeep_http,
        [AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <zeep/crypto.hpp>
												]],
                                   [[zeep::encode_base64(""); return 0;]])],
                   ax_cv_zeep_http=yes, ax_cv_zeep_http=no)
         AC_LANG_POP([C++])
		])
		if test "x$ax_cv_zeep_http" = "xyes"; then
			AC_DEFINE(HAVE_ZEEP_HTTP,,[define if the zeep::http library is available])
            ZEEPLIBDIR="${_AX_ZEEP_path}/lib"

            if test "x$ax_zeep_user_http_lib" = "x"; then
                for libextension in `ls $ZEEPLIBDIR/libzeep-http*.so* $ZEEPLIBDIR/libzeep-http*.dylib* $ZEEPLIBDIR/libzeep-http*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^lib\(zeep-http.*\)\.so.*$;\1;' -e 's;^lib\(zeep-http.*\)\.dylib.*;\1;' -e 's;^lib\(zeep-http.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [ZEEP_HTTP_LIB="-l$ax_lib"; AC_SUBST(ZEEP_HTTP_LIB) link_zeep_http="yes"; break],
                                 [link_zeep_http="no"])
				done
                if test "x$link_zeep_http" != "xyes"; then
                for libextension in `ls $ZEEPLIBDIR/zeep-http*.dll* $ZEEPLIBDIR/zeep-http*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^\(zeep-http.*\)\.dll.*$;\1;' -e 's;^\(zeep-http.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [ZEEP_HTTP_LIB="-l$ax_lib"; AC_SUBST(ZEEP_HTTP_LIB) link_zeep_http="yes"; break],
                                 [link_zeep_http="no"])
				done
                fi
            else
               for ax_lib in $ax_zeep_user_http_lib zeep-http; do
				      AC_CHECK_LIB($ax_lib, main,
                                   [ZEEP_HTTP_LIB="-l$ax_lib"; AC_SUBST(ZEEP_HTTP_LIB) link_zeep_http="yes"; break],
                                   [link_zeep_http="no"])
               done
            fi

            if test "x$ax_lib" = "x"; then
                AC_MSG_ERROR(Could not find a version of the zeep::http library!)
            fi
			if test "x$link_zeep_http" != "xyes"; then
				AC_MSG_ERROR(Could not link against $ax_lib !)
			fi
		fi

		CPPFLAGS="$CPPFLAGS_SAVED"
		LDFLAGS="$LDFLAGS_SAVED"
		LIBS="$ZEEP_HTTP_LIB $LIBS"
	fi
])



