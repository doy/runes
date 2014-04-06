#include <cairo.h>
#include <stdlib.h>

#include "runes.h"

RunesTerm *runes_term_create()
{
    RunesTerm *t;

    t = malloc(sizeof(RunesTerm));

    t->w       = runes_window_create();
    t->surface = runes_surface_create(t->w);
    t->cr      = cairo_create(t->surface);
    t->loop    = runes_loop_create(t);

    return t;
}

void runes_term_destroy(RunesTerm *t)
{
    cairo_destroy(t->cr);
    cairo_surface_destroy(t->surface);
    runes_window_destroy(t->w);

    free(t);
}
