/**
 * arif/src/arif_rl.c - ARIF Readline frontend
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

#include "arif_rl.h"

#ifdef HAVE_READLINE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>

#include "arif_defs.h"

char **
arif_rl_complete(
    struct arif_ctx *ctx,
    char const      *text,
    int              start,
    int              end
) {
    // skip right next to the last non-ASCII character
    int skip;
    for (skip = end - start; skip >= 0; --skip) {
        unsigned char ch = text[skip];
        if (0 != ch >> 7) {
            break;
        }
    }
    start += ++skip;

    rl_attempted_completion_over  = 1;
    rl_completion_suppress_append = 1;
    rl_completion_suppress_quote  = 1;

    end = arif_query(ctx, rl_line_buffer, start, end - start);
    if (end <= 0) {
        return NULL;
    }

    struct arif_cand const *cand;
    int num = arif_fetch(ctx, &cand);
    if (num == 0) {
        return NULL;
    }

    char **comp_list = malloc(sizeof(char *) * (num + 2));
    assert(comp_list != NULL);

    for (int idx = 1; idx <= num; ++idx, ++cand) {
        char const *line;
        int         line_len;
        if (cand->transform != NULL) {
            line     = cand->transform;
            line_len = cand->transform_len;
        } else {
            line     = text + skip;
            line_len = end;
        }

        int len = line_len - cand->replace_len + cand->len + skip;
        char *match = malloc(sizeof(char) * (len + 1));
        assert(match != NULL);

        memcpy(match, text, skip);
        memcpy(match + skip, cand->text, cand->len);
        memcpy(match + skip + cand->len, line + cand->replace_len,
                len - cand->len - skip);
        match[len] = '\0';
        comp_list[idx] = match;
    }
    comp_list[num + 1] = NULL;

    // Readline inserts completion matches by common prefix.
    // What we want:
    //  - if there is only one candidate, insert it
    //  - if there are multiple candidates, insert nothing
    if (num == 1) {
        comp_list[0] = comp_list[1];
        comp_list[1] = NULL;
    } else {
        char *match = malloc(sizeof(char));
        assert(match != NULL);
        match[0] = '\0';
        comp_list[0] = match;
    }

    return comp_list;
}

void
arif_rl_display(
    struct arif_ctx  *ctx,
    char            **ARIF_UNUSED_ARG(matches),
    int               num,
    int               max_len
) {
    char **disp_list = malloc(sizeof(char *) * (num + 2));
    assert(NULL != disp_list);

    struct arif_cand const *cand;
    assert(num == arif_fetch(ctx, &cand));

    disp_list[0] = "";
    for (int idx = 1; idx <= num; ++idx, ++cand) {
        int disp_len = cand->display_len;
        if (disp_len > max_len) {
            max_len = disp_len;
        }

        char *disp = malloc(sizeof(char) * (disp_len + 1));
        assert(disp != NULL);
        disp_list[idx] = memcpy(disp, cand->display, disp_len);
        disp[disp_len] = '\0';
    }
    disp_list[num + 1] = NULL;

    rl_display_match_list(disp_list, num, max_len);

    for (int i = 1; i <= num; ++i) {
        free(disp_list[i]);
    }
    free(disp_list);

    rl_forced_update_display();
}

#endif  // defined(HAVE_READLINE)
