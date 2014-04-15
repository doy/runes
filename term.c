#include <stdlib.h>

#include "runes.h"

void runes_term_init(RunesTerm *t, int argc, char *argv[])
{
    /* doing most of the pty initialization right at the beginning, because
     * libuv will set up a bunch of state (including potentially things like
     * spawning threads) when that is initialized, and i'm not really sure how
     * that interacts with forking */
    runes_pty_backend_init(t);
    runes_window_backend_init(t);

    runes_display_init(t);
    t->loop = uv_default_loop();

    runes_pty_backend_post_init(t);
    runes_window_backend_post_init(t, argc, argv);
    runes_display_post_init(t);
}

void runes_term_cleanup(RunesTerm *t)
{
    cairo_destroy(t->cr);
    runes_window_backend_cleanup(t);
    runes_pty_backend_cleanup(t);
}
