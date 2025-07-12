#
# Copyright (C) 2023  CismonX <admin@cismon.net>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty, provided the copyright notice and
# this notice are preserved. This file is offered as-is, without any warranty.
#

bin_PROGRAMS       =
include_HEADERS    =
noinst_HEADERS     =
pkglib_LTLIBRARIES =
dist_man1_MANS     =
dist_man3_MANS     =

if ARIF_RIME
  include_HEADERS    += arif_rime.h
  noinst_HEADERS     += arif_rime_workaround.h
  pkglib_LTLIBRARIES += rime.la
  dist_man3_MANS     += arif_rime.3

  rime_la_CPPFLAGS = $(RIME_CFLAGS) -I$(top_srcdir)/include
  rime_la_LDFLAGS  = -module
  rime_la_LIBADD   = $(RIME_LIBS)
  rime_la_SOURCES  = arif_rime.c arif_rime_workaround.cc
endif  # ARIF_RIME

if RL_LOOP
  bin_PROGRAMS   += rl-loop
  dist_man1_MANS += rl-loop.1

  rl_loop_CPPFLAGS = $(READLINE_CFLAGS)
  rl_loop_LDADD    = $(READLINE_LIBS)
  rl_loop_SOURCES  = rl_loop.c
endif  # RL_LOOP
