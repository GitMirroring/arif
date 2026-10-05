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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "arif_common.h"

static struct {
    char const           *basic_quote_chars;
    char const           *word_break_chars;
    rl_completion_func_t *comp_func;
    rl_compdisp_func_t   *disp_func;
    int                   ignore_duplicates;
    int                   query_items;
    int                   sort_matches;
} old_rl_vars;

char **
arif_rl_complete (
    struct arif_ctx *ctx,
    char const      *text,
    int              start,
    int              end
) {
    // skip right next to the last non-ASCII character
    int skip;
    for (skip = end - start - 1; skip >= 0; --skip) {
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

    char **comp_list = xmalloc(sizeof(char *) * (num + 2));

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

        int len = line_len - cand->replace_len + cand->text_len + skip;
        char *match = xmalloc(sizeof(char) * (len + 1));

        match[len] = '\0';
        comp_list[idx] = memcpy(match, text, skip);

        int offset = skip;
        memcpy(match + offset, line, cand->replace_start);

        offset += cand->replace_start;
        memcpy(match + offset, cand->text, cand->text_len);

        offset += cand->text_len;
        memcpy(match + offset,
                line + cand->replace_start + cand->replace_len,
                len - offset);
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
        char *match = xmalloc(sizeof(char));
        match[0] = '\0';
        comp_list[0] = match;

        // This hack ensures that candidates are always displayed,
        // even without `show-all-if-ambiguous` on.
        if (rl_completion_type == TAB) {
            arif_rl_display(ctx, NULL, num, 0);
        }
    }

    return comp_list;
}

void
arif_rl_disable (void)
{
    rl_basic_quote_characters          = old_rl_vars.basic_quote_chars;
    rl_completer_word_break_characters = old_rl_vars.word_break_chars;
    rl_attempted_completion_function   = old_rl_vars.comp_func;
    rl_completion_display_matches_hook = old_rl_vars.disp_func;
    rl_completion_query_items          = old_rl_vars.query_items;
    rl_ignore_completion_duplicates    = old_rl_vars.ignore_duplicates;
    rl_sort_completion_matches         = old_rl_vars.sort_matches;
}

void
arif_rl_display (
    struct arif_ctx  *ctx,
    char            **ARIF_UNUSED_ARG(matches),
    int               num,
    int               max_len
) {
    char **disp_list = xmalloc(sizeof(char *) * (num + 2));

    struct arif_cand const *cand;
    assert(num == arif_fetch(ctx, &cand));

    disp_list[0] = "";
    for (int idx = 1; idx <= num; ++idx, ++cand) {
        int disp_len = cand->display_len;
        if (disp_len > max_len) {
            max_len = disp_len;
        }

        char *disp = xmalloc(sizeof(char) * (disp_len + 1));
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

void
arif_rl_enable (
    rl_completion_func_t *complete_func,
    rl_compdisp_func_t   *display_func,
    char const           *word_break_chars
) {
    old_rl_vars.basic_quote_chars = rl_basic_quote_characters;
    old_rl_vars.word_break_chars  = rl_completer_word_break_characters;
    old_rl_vars.comp_func         = rl_attempted_completion_function;
    old_rl_vars.disp_func         = rl_completion_display_matches_hook;
    old_rl_vars.query_items       = rl_completion_query_items;
    old_rl_vars.ignore_duplicates = rl_ignore_completion_duplicates;
    old_rl_vars.sort_matches      = rl_sort_completion_matches;

    // This is necessary, since rl_complete_internal() appends these characters
    // even with rl_completion_suppress_quote set to non-zero
    rl_basic_quote_characters          = NULL;
    rl_completer_word_break_characters = word_break_chars;
    rl_attempted_completion_function   = complete_func;
    rl_completion_display_matches_hook = display_func;
    rl_completion_query_items          = 0;
    rl_ignore_completion_duplicates    = 0;
    rl_sort_completion_matches         = 0;
}
