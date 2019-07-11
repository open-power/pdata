AC_INIT([pdata], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

case "$host" in
	arm*-*-*) ARCH="arm" ;;
	*) ARCH=$(arch) ;;
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

AC_CHECK_LIB([pdbg], [pdbg_targets_init], [LIBS="-lpdbg -lfdt $LIBS"], [], [-lfdt])
if test x"$ac_cv_lib_pdbg_pdbg_targets_init" != "xyes" ; then
	AC_MSG_ERROR([PDBG library not found])
fi

if test x"$ARCH" != "xarm" ; then
	AC_CHECK_LIB([gdbm], [gdbm_open])
	if test x"$ac_cv_lib_gdbm_gdbm_open" != "xyes" ; then
		AC_MSG_ERROR([GDBM library not found])
	fi
fi
AM_CONDITIONAL([BUILD_TOOLS], [test x"$ARCH" != "xarm"])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
