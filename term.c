#include <stdlib.h>

#include "runes.h"

void runes_term_init(RunesTerm *t, int argc, char *argv[])
{
    int x, y;

    /* doing most of the pty initialization right at the beginning, because
     * libuv will set up a bunch of state (including potentially things like
     * spawning threads) when that is initialized, and i'm not really sure how
     * that interacts with forking */
    runes_pty_backend_init(t);

    t->loop = uv_default_loop();

    runes_pty_backend_loop_init(t);

    runes_window_backend_init(t, argc, argv);
    t->backend_cr = cairo_create(runes_window_backend_surface_create(t));
    runes_window_backend_get_size(t, &x, &y);
    t->cr = cairo_create(cairo_surface_create_similar_image(cairo_get_target(t->backend_cr), CAIRO_FORMAT_RGB24, x, y));
}

void runes_term_cleanup(RunesTerm *t)
{
    cairo_destroy(t->cr);
    runes_window_backend_cleanup(t);
    runes_pty_backend_cleanup(t);
}
