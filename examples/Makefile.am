# 
# Copyright (C) 2023  CismonX <admin@cismon.net>
# 
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty, provided the copyright notice and
# this notice are preserved. This file is offered as-is, without any warranty.
# 

include_HEADERS = 
lib_LTLIBRARIES = 
dist_man3_MANS  = 


if BUILD_ARIF_RIME

  include_HEADERS += arif_rime.h
  lib_LTLIBRARIES += libarifrime.la
  dist_man3_MANS  += arif_rime.3

  libarifrime_la_CPPFLAGS = $(RIME_CFLAGS) -I$(top_srcdir)/include
  libarifrime_la_LIBADD   = $(RIME_LIBS)
  libarifrime_la_SOURCES  = arif_rime.c

endif  # BUILD_ARIF_RIME
