#
# Copyright (C) 2023  CismonX <admin@cismon.net>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty, provided the copyright notice and
# this notice are preserved. This file is offered as-is, without any warranty.
#

AUTOMAKE_OPTIONS = dejagnu

check_HEADERS     = arif_dummy_engine.h
check_PROGRAMS    = arif-test
check_LTLIBRARIES = libarif_dummy.la

arif_test_CPPFLAGS = -I$(top_srcdir)/src
arif_test_LDADD    = $(top_builddir)/src/libarif.la
arif_test_SOURCES  = arif_test.c arif_dummy_engine.c

libarif_dummy_la_CPPFLAGS = -I$(top_srcdir)/src
libarif_dummy_la_SOURCES  = arif_dummy_engine.c

EXTRA_DIST = lib/arif.exp arif/000-query.exp arif/001-select-page.exp  \
             arif/002-fetch.exp
RUNTESTFLAGS = ARIF_TEST_BIN=$(builddir)/arif-test
