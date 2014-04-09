#include <stdlib.h>

#include "runes.h"

void runes_term_init(RunesTerm *t, int argc, char *argv[])
{
    t->loop = uv_default_loop();

    runes_window_backend_init(t, argc, argv);
    t->cr = cairo_create(runes_window_backend_surface_create(t));

    runes_window_backend_loop_init(t, t->loop);
}

void runes_term_cleanup(RunesTerm *t)
{
    cairo_destroy(t->cr);
    runes_window_backend_cleanup(t);
}
