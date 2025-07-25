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
LT_INIT([disable-static dlopen])

# -- Checks for programs --

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# -- Checks for features --

EX_FEAT([arify],         [Y E], [the arify program])
EX_FEAT([rlexec],        [  E], [the rlexec program])
EX_FEAT([arif-readline], [YDE], [Readline features for ARIF])
EX_FEAT([arif-rime],     [  E], [RIME integration for ARIF])
EX_FEAT([arif-debug],    [ D ], [debugging features for ARIF])

# -- Checks for libraries --

EX_DEP([readline], [>= 6.0], [GNU Readline library],
        [rlexec], [arif-readline])

EX_DEP([rime], [>= 1.6], [Rime Input Method Engine], [arif-rime])

# -- Output --

PKG_INSTALLDIR()
AC_CONFIG_FILES([
    Makefile
    doc/Makefile
    src/Makefile src/arif.pc
    tests/Makefile
])
AC_OUTPUT
