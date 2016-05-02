#include <string.h>

#include "runes.h"

void runes_term_init(RunesTerm *t, RunesLoop *loop, int argc, char *argv[])
{
    memset((void *)t, 0, sizeof(*t));

    runes_config_init(t, argc, argv);
    runes_display_init(t);

    runes_window_backend_create_window(t, argc, argv);
    runes_pty_backend_spawn_subprocess(t);

    vt100_screen_init(&t->scr);
    vt100_screen_set_scrollback_length(&t->scr, t->config.scrollback_length);
    runes_display_set_window_size(t);

    t->loop = loop;
    runes_window_backend_init_loop(t, loop);
    runes_pty_backend_init_loop(t, loop);
}

void runes_term_cleanup(RunesTerm *t)
{
    runes_config_cleanup(t);
    runes_display_cleanup(t);
    vt100_screen_cleanup(&t->scr);
    runes_window_backend_cleanup(t);
    runes_pty_backend_cleanup(t);
}
