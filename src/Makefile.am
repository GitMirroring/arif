#
# Copyright (C) 2023  CismonX <admin@cismon.net>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty, provided the copyright notice and
# this notice are preserved. This file is offered as-is, without any warranty.
#

noinst_HEADERS     = arif_common.h arify.h
include_HEADERS    = arif.h
bin_PROGRAMS       =
lib_LTLIBRARIES    = libarif.la
pkglib_LTLIBRARIES =
pkgconfig_DATA     = arif.pc

libarif_la_CPPFLAGS = $(READLINE_CFLAGS)
libarif_la_LIBADD   = $(READLINE_LIBS)
libarif_la_SOURCES  = arif.c

if ARIF_READLINE
  noinst_HEADERS  += arify_rl.h
  include_HEADERS += arif_rl.h

  libarif_la_SOURCES += arif_rl.c
endif  # ARIF_READLINE

if ARIFY
  ARIFY_CPPFLAGS_ = -DARIF_LIBDIR="\"$(libdir)\""

  bin_PROGRAMS    += arify
  lib_LTLIBRARIES += libarify.la

  arify_CPPFLAGS = $(ARIFY_CPPFLAGS_)
  arify_SOURCES  = arify_cli.c

  libarify_la_CPPFLAGS = $(READLINE_CFLAGS) $(ARIFY_CPPFLAGS_)
  libarify_la_LIBADD   = $(READLINE_LIBS) $(DL_LIBS) libarif.la
  libarify_la_SOURCES  = arify.c

if ARIF_READLINE
  libarify_la_SOURCES += arify_rl.c
endif  # ARIF_READLINE
endif  # ARIFY

if ARIF_RIME
  include_HEADERS    += arif_rime.h
  noinst_HEADERS     += arif_rime_quirks.h
  pkglib_LTLIBRARIES += rime.la

  rime_la_CPPFLAGS = $(RIME_CFLAGS)
  rime_la_LDFLAGS  = -module
  rime_la_LIBADD   = $(RIME_LIBS)
  rime_la_SOURCES  = arif_rime.c arif_rime_quirks.cc
endif  # ARIF_RIME

if RL_LOOP
  bin_PROGRAMS += rl-loop

  rl_loop_CPPFLAGS = $(READLINE_CFLAGS)
  rl_loop_LDADD    = $(READLINE_LIBS)
  rl_loop_SOURCES  = rl_loop.c
endif  # RL_LOOP
