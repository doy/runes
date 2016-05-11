#include <stdlib.h>
#include <string.h>

#include "runes.h"
#include "term.h"

#include "config.h"
#include "display.h"
#include "pty-unix.h"
#include "window-xlib.h"

RunesTerm *runes_term_new(int argc, char *argv[], RunesWindowBackend *wb)
{
    RunesTerm *t;
    int width, height;

    t = calloc(1, sizeof(RunesTerm));

    t->config = runes_config_new(argc, argv);
    t->display = runes_display_new(t->config->font_name);
    t->w = runes_window_new(wb);
    t->pty = runes_pty_new();
    t->scr = vt100_screen_new(t->config->default_cols, t->config->default_rows);

    vt100_screen_set_scrollback_length(t->scr, t->config->scrollback_length);
    runes_window_create_window(t, argc, argv);
    runes_pty_spawn_subprocess(t);
    runes_display_set_context(t, t->w->backend_cr);

    runes_window_get_size(t, &width, &height);
    runes_term_set_window_size(t, width, height);

    return t;
}

void runes_term_register_with_loop(RunesTerm *t, RunesLoop *loop)
{
    t->loop = loop;
    runes_window_init_loop(t, loop);
    runes_pty_init_loop(t, loop);
}

void runes_term_set_window_size(RunesTerm *t, int xpixel, int ypixel)
{
    int row = ypixel / t->display->fonty, col = xpixel / t->display->fontx;

    runes_pty_set_window_size(t, row, col, xpixel, ypixel);
    vt100_screen_set_window_size(t->scr, row, col);
}

void runes_term_refcnt_inc(RunesTerm *t)
{
    t->refcnt++;
}

void runes_term_refcnt_dec(RunesTerm *t)
{
    t->refcnt--;
    if (t->refcnt <= 0) {
        runes_term_delete(t);
    }
}

void runes_term_delete(RunesTerm *t)
{
    vt100_screen_delete(t->scr);
    runes_pty_delete(t->pty);
    runes_window_delete(t->w);
    runes_display_delete(t->display);
    runes_config_delete(t->config);

    free(t);
}
