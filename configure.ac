AC_INIT([pdata], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AC_ARG_VAR(CHIP, [Target chip to build for])
AS_IF([test "x$CHIP" == "x"], [CHIP="p10"])

AC_SUBST([CHIP])

case "$CHIP" in
	"p9") pchip="p9n" ;;
	"p10") pchip="p10" ;;
	*) pchip="unk" ;;
esac
AC_DEFINE_UNQUOTED([PCHIP], ["$pchip"], [Power chip type])

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
AC_PROG_CXX

if test x"$ARCH" != "xarm" ; then

AC_PATH_PROG([DTC], [dtc])
if test x"$ac_cv_path_DTC" = x ; then
	AC_MSG_ERROR([Command 'dtc' not found.])
fi
AC_SUBST([DTC])

fi

FDT=1
AC_CHECK_LIB([fdt], [fdt_create], [LIBS="-lfdt $LIBS"])
if test x"$ac_cv_lib_fdt_fdt_create" != "xyes" ; then
	AC_MSG_WARN([FDT library not found, attribute tool will not built])
	FDT=0
fi

AM_CONDITIONAL([BUILD_TOOLS], [test $FDT -eq 1])
AM_CONDITIONAL([BUILD_DTB], [test x"$ARCH" != "xarm"])

AC_ARG_ENABLE([gen_attrsinfo],
	AS_HELP_STRING([--enable-gen_attrsinfo],
		       [Generate attributes API based on dynamic device tree]))
AM_CONDITIONAL([BUILD_ATTR_API], [test "x$enable_gen_attrsinfo" = "xyes"])

if test "x$enable_gen_attrsinfo" = "xyes" ; then
	if test $FDT -eq 0 ; then
		AC_MSG_ERROR([FDT library not fond])
	fi
	AC_CHECK_LIB([pdbg], [pdbg_targets_init], [LIBS="-lpdbg $LIBS"])
	if test x"$ac_cv_lib_pdbg_pdbg_targets_init" != "xyes" ; then
		AC_MSG_ERROR([PDBG library not found])
	fi

	AX_CXX_COMPILE_STDCXX_11([noext])
fi

AC_ARG_ENABLE([gen_dynamicdt],
	AS_HELP_STRING([--enable-gen-dynamicdt],
		       [Generate dynamic device tree]))
AM_CONDITIONAL([BUILD_DYNAMIC_DT], [test x"$enable_gen_dynamicdt" = "xyes"])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([libdt-api/libdt-api.pc])

AC_OUTPUT
