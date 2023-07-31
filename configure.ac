dnl
dnl  Copyright (C) 2023  CismonX <admin@cismon.net>
dnl
dnl  Copying and distribution of this file, with or without modification,
dnl  are permitted in any medium without royalty,
dnl  provided the copyright notice and this notice are preserved.
dnl  This file is offered as-is, without any warranty.
dnl

AC_PREREQ([2.69])
AC_INIT([arif], [0.1.0], [arif-devel@nongnu.org])
AC_CONFIG_SRCDIR([src/arif.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
PKG_PROG_PKG_CONFIG([0.27])
LT_PREREQ([2.4.2])
LT_INIT([dlopen])

# -- Checks for programs --

AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_SED

# -- Checks for libraries --

AC_CHECK_LIB([dl], [dlopen], [
    AS_VAR_SET([DL_LIBS], [-ldl])
])

ARIF_CHECK_PKG([readline], [>= 6.0], [GNU Readline], [], [
    AC_DEFINE([HAVE_READLINE], [1], [Define to 1 if you have GNU Readline.])
])
AM_CONDITIONAL([BUILD_ARIF_READLINE], [test x${with_readline} != xno])

ARIF_CHECK_PKG([rime], [>= 1.6], [Rime Input Method Engine], [
    AS_VAR_SET([with_rime], [no])
], [
    AC_DEFINE([HAVE_RIME], [1],
            [Define to 1 if you have the Rime Input Method Engine.])
])
AM_CONDITIONAL([BUILD_ARIF_RIME], [test x${with_rime} != xno])

# -- Checks for features --

ARIF_ARG_ENABLE([arif-debug], [no], [debugging features for ARIF], [], [
    AC_DEFINE([ARIF_DEBUG], [1],
            [Define to 1 if ARIF debugging is enabled])
    AS_VAR_SET([enable_debug], [yes])
])
AM_CONDITIONAL([ENABLE_DEBUG], [test x${enable_debug} = xyes])

ARIF_ARG_ENABLE([arify], [yes], [the arify command line tool], [], [
    AS_VAR_IF([with_readline], [no], [
        AC_MSG_WARN(m4_normalize([
            The arify command line tool will not be built,
            as the ARIF library is configured without GNU Readline.
        ]))
        AS_VAR_SET([enable_arify], [no])
    ])
])
AM_CONDITIONAL([BUILD_ARIFY], [test x${enable_arify} != xno])

# -- Checks for compiler builtins and attributes --

ARIF_TRY_APPLY([AX_GCC_FUNC_ATTRIBUTE],
        [constructor], [destructor], [noreturn])
ARIF_TRY_APPLY([AX_GCC_VAR_ATTRIBUTE],
        [unused])

# -- Checks for header files --

AC_CHECK_HEADERS([fcntl.h strings.h unistd.h])

# -- Checks for typedefs, structures, and compiler characteristics --

AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# --  Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit memchr strcasecmp])

# -- Output --

PKG_INSTALLDIR()
AC_CONFIG_FILES([
    Makefile
    doc/Makefile
    examples/Makefile
    include/Makefile
    src/Makefile
    tests/Makefile
    arif.pc
])
AC_OUTPUT
