dnl
dnl  Copyright (C) 2023  CismonX <admin@cismon.net>
dnl  
dnl  Copying and distribution of this file, with or without modification,
dnl  are permitted in any medium without royalty,
dnl  provided the copyright notice and this notice are preserved.
dnl  This file is offered as-is, without any warranty.
dnl

dnl
dnl  ARIF_ARG_ENABLE(feature, default-value, description, [extra-msg],
dnl                  [action-if-enabled], [action-if-disabled])
dnl  
dnl  Wrapper for AC_ARG_ENABLE.
dnl
AC_DEFUN([ARIF_ARG_ENABLE], [
    m4_pushdef([arg_action_], m4_if([$2], [no], [enable], [disable]))
    m4_pushdef([enable_var_], [enable_]m4_translit([$1], [-+.], [___]))
    AC_MSG_CHECKING(m4_normalize([if $3 is enabled]))
    AC_ARG_ENABLE([$1], m4_normalize([
        AS_HELP_STRING([--]arg_action_[-$1], arg_action_ [$3 $4])
    ]), [], [
        AS_VAR_SET(enable_var_, [$2])
    ])
    AS_VAR_IF(enable_var_, [no], [
        $6
        AC_MSG_RESULT([no])
    ], [
        $5
        AC_MSG_RESULT([yes])
    ])
    m4_popdef([arg_action_])
    m4_popdef([enable_var_])
])

dnl
dnl  ARIF_CHECK_PKG(package, version, package-name, [action-if-not-given],
dnl                 [action-if-found], [action-if-without])
dnl  
dnl  Checks if a package exists with `pkg-config', and provides option for
dnl  the configure script to specify the package's custom install location.
dnl
AC_DEFUN([ARIF_CHECK_PKG], [
    m4_pushdef([with_var_], [with_]m4_translit([$1], [-+.], [___]))
    AC_ARG_WITH([$1], m4_normalize([
        AS_HELP_STRING(
		[--with-$1[[=PKGCONFIGDIR]]], [pkg-config search path for $3])
    ]), [], [$4])
    AS_VAR_IF(with_var_, [no], [$6], [
        AS_VAR_SET([SAVED_PKG_CONFIG_PATH_], ["${PKG_CONFIG_PATH}"])
        AS_VAR_IF(with_var_, [yes], [], [
            AS_VAR_SET([PKG_CONFIG_PATH], ["${with_$1}:${PKG_CONFIG_PATH}"])
        ])
        export PKG_CONFIG_PATH
        PKG_CHECK_MODULES(m4_toupper([$1]), [$1 $2], [$5])
        AS_VAR_SET([PKG_CONFIG_PATH], ["${SAVED_PKG_CONFIG_PATH_}"])
    ])
    m4_popdef([with_var_])
])

dnl
dnl  ARIF_TRY_APPLY(macro-name, ...)
dnl  
dnl  If `macro-name' is defined, for each remaining argument,
dnl  applies the macro to that argument.
dnl
AC_DEFUN([ARIF_TRY_APPLY], [
    m4_ifdef([$1], [
        m4_foreach([val_], [m4_shift($@)], [$1(val_)])
    ])
])
