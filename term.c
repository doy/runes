#include "runes.h"

void runes_term_init(RunesTerm *t, int argc, char *argv[])
{
    runes_config_init(t, argc, argv);

    /* doing most of the pty initialization right at the beginning, because
     * libuv will set up a bunch of state (including potentially things like
     * spawning threads) when that is initialized, and i'm not really sure how
     * that interacts with forking */
    runes_pty_backend_spawn_subprocess(t);

    runes_display_init(t);
    runes_window_backend_create_window(t, argc, argv);

    runes_display_set_window_size(t);

    /* have to initialize these here instead of in display_init because they
     * depend on the window size being set */
    t->scroll_top = 0;
    t->scroll_bottom = t->rows - 1;

    t->loop = uv_default_loop();
    runes_window_backend_start_loop(t);
    runes_pty_backend_start_loop(t);
}

void runes_term_cleanup(RunesTerm *t)
{
    runes_display_cleanup(t);
    runes_window_backend_cleanup(t);
    runes_pty_backend_cleanup(t);
}
