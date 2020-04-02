AC_INIT([pdata], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

case "$host" in
	arm*-*-*) ARCH="arm" ;;
	*) ARCH=$(uname -m) ;;
esac
AC_SUBST([ARCH])

case "$ARCH" in
	x86_64|ppc64le|arm)
		ENDIAN_FLAG=-DHAVE_LITTLE_ENDIAN
		;;
	ppc64)
		ENDIAN_FLAG=-DHAVE_BIG_ENDIAN
		;;
	*)
		AC_MSG_ERROR([Unknown architecture])
		;;
esac
AC_SUBST([ENDIAN_FLAG])

AC_PROG_CC
AC_PROG_LIBTOOL

if test x"$ARCH" != "xarm" ; then

AC_PATH_PROG([DTC], [dtc])
if test x"$ac_cv_path_DTC" = x ; then
	AC_MSG_ERROR([Command 'dtc' not found.])
fi
AC_SUBST([DTC])

fi

PDBG=1
AC_CHECK_LIB([pdbg], [pdbg_targets_init], [LIBS="-lpdbg -lfdt $LIBS"], [], [-lfdt])
if test x"$ac_cv_lib_pdbg_pdbg_targets_init" != "xyes" ; then
	AC_MSG_WARN([PDBG library not found, attribute tool will not built])
	PDBG=0
fi

AM_CONDITIONAL([BUILD_TOOLS], [test x"$ARCH" != "xarm" -a $PDBG -eq 1])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
