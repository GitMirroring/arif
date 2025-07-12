/**
 * arif/src/arif_rime_workaround.cc
 * ----
 *
 * Copyright (C) 2023  CismonX <admin@cismon.net>
 *
 * This file is part of ARIF, Another Readline Input Framework.
 *
 * ARIF is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ARIF is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARIF.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "arif_rime_workaround.h"

// It's better practice to just `#include <glog/logging.h>` here instead.
// However, we don't want to make glog our dependency just for the sake of
// this workaround.

namespace fLB {
    bool FLAGS_alsologtostderr;
}

namespace fLI {
    int FLAGS_stderrthreshold;
}

void
arif_rime_workaround_glog_nostderr (void)
{
    // Rime sets this to true during init
    fLB::FLAGS_alsologtostderr = false;
    // default value is ERROR (2) instead of FATAL
    fLI::FLAGS_stderrthreshold = 3;
}
