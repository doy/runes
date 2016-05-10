#include <stdlib.h>
#include <string.h>

#include "runes.h"
#include "term.h"

#include "config.h"
#include "display.h"
#include "pty-unix.h"
#include "window-xlib.h"

static void runes_term_init_loop(RunesTerm *t, RunesLoop *loop);

void runes_term_init(RunesTerm *t, RunesLoop *loop, int argc, char *argv[])
{
    int width, height;

    t->config = calloc(1, sizeof(RunesConfig));
    runes_config_init(t->config, argc, argv);

    t->display = calloc(1, sizeof(RunesDisplay));
    runes_display_init(t->display, t->config->font_name);

    t->w = calloc(1, sizeof(RunesWindowBackend));
    runes_window_backend_init(t->w);

    t->pty = calloc(1, sizeof(RunesPtyBackend));
    runes_pty_backend_init(t->pty);

    t->scr = calloc(1, sizeof(VT100Screen));
    vt100_screen_init(t->scr);

    vt100_screen_set_scrollback_length(t->scr, t->config->scrollback_length);
    runes_window_backend_create_window(t, argc, argv);
    runes_pty_backend_spawn_subprocess(t);
    runes_display_set_context(t, t->w->backend_cr);

    runes_window_backend_get_size(t, &width, &height);
    runes_term_set_window_size(t, width, height);

    t->loop = loop;
    runes_term_init_loop(t, loop);
}

void runes_term_refcnt_inc(RunesTerm *t)
{
    t->refcnt++;
}

void runes_term_refcnt_dec(RunesTerm *t)
{
    t->refcnt--;
    if (t->refcnt <= 0) {
        runes_term_cleanup(t);
        free(t);
    }
}

void runes_term_set_window_size(RunesTerm *t, int xpixel, int ypixel)
{
    int row = ypixel / t->display->fonty, col = xpixel / t->display->fontx;

    runes_pty_backend_set_window_size(t, row, col, xpixel, ypixel);
    vt100_screen_set_window_size(t->scr, row, col);
}

void runes_term_cleanup(RunesTerm *t)
{
    vt100_screen_cleanup(t->scr);
    free(t->scr);

    runes_display_cleanup(t->display);
    free(t->display);

    runes_config_cleanup(t->config);
    free(t->config);
}

static void runes_term_init_loop(RunesTerm *t, RunesLoop *loop)
{
    runes_window_backend_init_loop(t, loop);
    runes_pty_backend_init_loop(t, loop);
}
