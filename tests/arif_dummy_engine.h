/**
 * arif/tests/arif_dummy_engine.h
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

#ifndef ARIF_DUMMY_ENGINE_H_
#define ARIF_DUMMY_ENGINE_H_

#include <stdbool.h>

#include "arif.h"

struct arif_dummy_engine_opts {
    bool gen_all_candidates;
};

extern struct arif_engine const arif_dummy_engine;

#endif  // !defined(ARIF_DUMMY_ENGINE_H_)
