AC_INIT([sph], [1.0], [santiago.pivetta@gmail.com])

AC_REVISION([$Revision$])

AC_CONFIG_FILES([Makefile])

AC_LANG([C++])

AC_DEFUN([AX_CXX_CHECK_LIB],
[m4_ifval([$3], , [AH_CHECK_LIB([$1])])dnl
AS_LITERAL_IF([$1],
          [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1_$2])],
          [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1''_$2])])dnl
AC_CACHE_CHECK([for $2 in -l$1], ac_Lib,
[ac_check_lib_save_LIBS=$LIBS
LIBS="-l$1 $5 $LIBS"
case "$2"
in *::*::*\(*)
AC_LINK_IFELSE([AC_LANG_PROGRAM([
 namespace `echo "$2" | sed -e "s/::.*//"`
 { class `echo "$2" | sed -e "s/.*::\\(.*\\)::.*/\\1/" -e "s/(.*//"`
   { public: int `echo "$2" | sed -e "s/.*:://" -e "/(/!s/..*/&amp;()/"`;
   };
 }
],[`echo "$2" | sed  -e "s/(.*//" -e "s/\\(.*\\)::\\(.*\\)/((\\1*)(0))-&gt;\\2/g"`()])],
           [AS_VAR_SET(ac_Lib, yes)],
           [AS_VAR_SET(ac_Lib, no)])
;; *::*::*)
AC_LINK_IFELSE([AC_LANG_PROGRAM([
 namespace `echo "$2" | sed -e "s/::.*//"`
 { namespace `echo "$2" | sed -e "s/.*::\\(.*\\)::.*/\\1/"`
   { class `echo "$2" | sed -e "s/.*:://"`
      { public: `echo "$2" | sed -e "s/.*:://"` ();
      };
   }
 }
],[new $2()])],
           [AS_VAR_SET(ac_Lib, yes)],
           [AS_VAR_SET(ac_Lib, no)])
;; *::*\(*)
AC_LINK_IFELSE([AC_LANG_PROGRAM([
 class `echo "$2" | sed -e "s/\\(.*\\)::.*/\\1/" -e "s/(.*//"`
   { public: int `echo "$2" | sed -e "s/.*:://" -e "/(/!s/..*/&amp;()/"`;
   };
],[`echo "$2" | sed  -e "s/(.*//" -e "s/\\(.*\\)::\\(.*\\)/((\\1*)(0))-&gt;\\2/g"`()])],
           [AS_VAR_SET(ac_Lib, yes)],
           [AS_VAR_SET(ac_Lib, no)])
;; *::*)
AC_LINK_IFELSE([AC_LANG_PROGRAM([
 namespace `echo "$2" | sed -e "s/::.*//"`
 { class `echo "$2" | sed -e "s/.*:://"`
   { public: `echo "$2" | sed -e "s/.*:://"` ();
   };
 }
],[new $2()])],
           [AS_VAR_SET(ac_Lib, yes)],
           [AS_VAR_SET(ac_Lib, no)])
;; *)
AC_LINK_IFELSE([AC_LANG_CALL([], [$2])],
           [AS_VAR_SET(ac_Lib, yes)],
           [AS_VAR_SET(ac_Lib, no)])
;; esac
LIBS=$ac_check_lib_save_LIBS])
AS_IF([test AS_VAR_GET(ac_Lib) = yes],
      [m4_default([$3], [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_LIB$1))
  LIBS="-l$1 $LIBS"
])],
      [$4])dnl
AS_VAR_POPDEF([ac_Lib])dnl
])# AC_CHECK_LIB


AC_CHECK_LIB(OpenCL, clCreateContext, [], AC_MSG_ERROR(OpenCL not present!))
AC_CHECK_LIB(pthread, pthread_create, [], AC_MSG_ERROR(pthread not present!))
AX_CXX_CHECK_LIB(clogs, clogs::RadixsortProblem, [], AC_MSG_ERROR(clogs not present!))

AC_ARG_WITH([qt-install-path],
    [AS_HELP_STRING([--with-qt-install-path], [location of the Qt installation path])],
    [QT_INSTALL_PATH="$withval"],
    AC_MSG_ERROR(Qt installation path not defined))
AC_SUBST([QT_INSTALL_PATH])

AC_ARG_WITH([cl-include-path],
    [AS_HELP_STRING([--with-cl-include-path], [location of the OpenCL headers])],
    [CL_INCLUDE_PATH="$withval"],
    AC_MSG_ERROR(OpenCL include path not defined))
AC_SUBST([CL_INCLUDE_PATH])

AC_ARG_WITH([bt-install-path],
    [AS_HELP_STRING([--with-bt-install-path], [location of the Bullets Physics installation path])],
    [BT_INSTALL_PATH="$withval"],
    AC_MSG_ERROR(Bullets physics installation path not defined))
AC_SUBST([BT_INSTALL_PATH])

AC_OUTPUT
