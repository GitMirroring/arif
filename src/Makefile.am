# 
# Copyright (C) 2023  CismonX <admin@cismon.net>
# 
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty, provided the copyright notice and
# this notice are preserved. This file is offered as-is, without any warranty.
# 

bin_PROGRAMS    = 
lib_LTLIBRARIES = libarif.la

ARIF_CPPFLAGS_ = -I$(top_srcdir)/include

libarif_la_CPPFLAGS = $(READLINE_CFLAGS) $(ARIF_CPPFLAGS_)
libarif_la_LIBADD   = $(READLINE_LIBS)
libarif_la_SOURCES  = arif.c


if BUILD_ARIF_READLINE

  libarif_la_SOURCES += arif_rl.c

endif  # BUILD_ARIF_READLINE

if BUILD_ARIFY

  bin_PROGRAMS    += arify
  lib_LTLIBRARIES += libarify.la

  arify_CPPFLAGS = $(ARIF_CPPFLAGS_) -DARIFY_LIBDIR=$(libdir)
  arify_LDADD    = libarify.la
  arify_SOURCES  = arify_cli.c

  libarify_la_CPPFLAGS = $(ARIF_CPPFLAGS_)
  libarify_la_LIBADD   = $(DL_LIBS) libarif.la
  libarify_la_SOURCES  = arify.c

if BUILD_ARIF_READLINE

  libarify_la_SOURCES += arify_rl.c

endif  # BUILD_ARIF_READLINE

endif  # BUILD_ARIFY
