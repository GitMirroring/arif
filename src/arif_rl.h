/**
 * arif/src/arif_rl.h - ARIF Readline frontend
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

#ifndef ARIF_RL_H_
#define ARIF_RL_H_

#include <stdio.h>
#include <readline/readline.h>

#include "arif.h"

char **
arif_rl_complete (
    struct arif_ctx *ctx,
    char const      *text,
    int              start,
    int              end
);

void
arif_rl_disable (void);

void
arif_rl_display (
    struct arif_ctx  *ctx,
    char            **matches,
    int               num,
    int               max_len
);

void
arif_rl_enable (
    rl_completion_func_t *complete_func,
    rl_compdisp_func_t   *display_func,
    char const           *word_break_chars
);

#endif  // !defined(ARIF_RL_H_)
