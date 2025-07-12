#
# Copyright (C) 2023  CismonX <admin@cismon.net>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty, provided the copyright notice and
# this notice are preserved. This file is offered as-is, without any warranty.
#

info_TEXINFOS  = arif.texi
dist_man1_MANS =
dist_man3_MANS = arif_ctx_create.3 arif_ctx_destroy.3 arif_fetch.3  \
                 arif_query.3 arif_select_page.3 arif_set_engine.3

arif_TEXINFOS = fdl.texi

if ARIF_READLINE
  dist_man1_MANS += arify.1
  dist_man3_MANS += arif_rl_complete.3 arif_rl_display.3
endif  # ARIF_READLINE
