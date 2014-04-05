#ifndef _RUNES_H
#define _RUNES_H

#include <cairo.h>

#include "xlib.h"

typedef struct {
    RunesWindow *w;
    /* RunesBuffer *buf; */

    cairo_surface_t *surface;
    cairo_t *cr;
} RunesTerm;

RunesTerm *runes_term_create();
void runes_term_flush(RunesTerm *t);
void runes_term_destroy(RunesTerm *t);

#endif
