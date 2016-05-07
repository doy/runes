#include <string.h>

#include "runes.h"

void runes_term_init(RunesTerm *t, RunesLoop *loop, int argc, char *argv[])
{
    int width, height;

    memset((void *)t, 0, sizeof(*t));

    runes_config_init(t, argc, argv);
    runes_display_init(t);

    runes_window_backend_create_window(t, argc, argv);
    runes_pty_backend_spawn_subprocess(t);

    vt100_screen_init(&t->scr);
    vt100_screen_set_scrollback_length(&t->scr, t->config.scrollback_length);

    runes_window_backend_get_size(t, &width, &height);
    runes_term_set_window_size(t, width, height);

    t->loop = loop;
    runes_window_backend_init_loop(t, loop);
    runes_pty_backend_init_loop(t, loop);
}

void runes_term_set_window_size(RunesTerm *t, int xpixel, int ypixel)
{
    int row = ypixel / t->display.fonty, col = xpixel / t->display.fontx;

    runes_display_set_window_size(t, xpixel, ypixel);
    runes_pty_backend_set_window_size(t, row, col, xpixel, ypixel);
    vt100_screen_set_window_size(&t->scr, row, col);
}

void runes_term_cleanup(RunesTerm *t)
{
    runes_config_cleanup(t);
    runes_display_cleanup(t);
    vt100_screen_cleanup(&t->scr);
    runes_window_backend_cleanup(t);
    runes_pty_backend_cleanup(t);
}
