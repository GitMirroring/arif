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

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

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

ARIF_ARG_ENABLE([arify], [yes], [the arify program])
AM_CONDITIONAL([BUILD_ARIFY], [test x${enable_arify} != xno])

ARIF_ARG_ENABLE([rl-loop], [no], [the rl-loop program], [], [
    AS_VAR_IF([with_readline], [no], [
        AC_MSG_ERROR(m4_normalize([
            The rl-loop program could not be built,
            since mandatory dependency GNU Readline is not configured.
        ]))
    ])
])
AM_CONDITIONAL([BUILD_RL_LOOP], [test x${enable_rl_loop} != xno])

# -- Checks for compiler builtins and attributes --

AX_GCC_BUILTIN([__builtin_unreachable])
AX_GCC_FUNC_ATTRIBUTE([constructor])
AX_GCC_FUNC_ATTRIBUTE([destructor])
AX_GCC_VAR_ATTRIBUTE([unused])

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
