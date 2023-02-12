/**
 * arif/include/arify.h - ARIF preload library
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

#ifndef ARIFY_H_
#define ARIFY_H_

#include "arif.h"

#ifdef ARIF_DEBUG
#  define arify_debug_printf(fmt, ...)  arify_log_printf(fmt, "D", __VA_ARGS__)
#else
#  define arify_debug_printf(fmt, ...)
#endif

#define arify_err_printf(fmt, ...)  arify_log_printf(fmt, "E", __VA_ARGS__)

typedef void (arify_frontend_finalize_func) (
    void *frontend_data
);

typedef int (arify_frontend_init_func) (
    struct arif_ctx  *ctx,
    void            **frontend_data_ptr
);

struct arify_frontend {
    arify_frontend_init_func     *init;
    arify_frontend_finalize_func *finalize;
};

void
arify_log_printf (
    char const *fmt,
    char const *level,
    ...
);

struct arif_engine const *
arify_next_engine (
    void **engine_data_ptr
);

#endif  // !defined(ARIFY_H_)
