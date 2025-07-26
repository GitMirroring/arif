#
# Copyright (C) 2023  CismonX <admin@cismon.net>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty, provided the copyright notice and
# this notice are preserved. This file is offered as-is, without any warranty.
#

dist_man1_MANS =
dist_man3_MANS = arif_ctx_create.3 arif_ctx_destroy.3 arif_fetch.3  \
                 arif_query.3 arif_select_page.3 arif_set_engine.3
dist_man7_MANS = arif.7

if ARIF_READLINE
  dist_man3_MANS += arif_rl_complete.3 arif_rl_display.3
endif  # ARIF_READLINE

if ARIFY
  dist_man1_MANS += arify.1
endif  # ARIFY

if ARIF_RIME
  dist_man3_MANS += arif_rime.3
endif  # ARIF_RIME

if RLEXEC
  dist_man1_MANS += rlexec.1
endif  # RLEXEC

EXTRA_DIST = fdl.txt
