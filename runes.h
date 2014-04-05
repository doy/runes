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
void runes_prepare_input(RunesTerm *t);
void runes_read_key(RunesTerm *t, char **buf, size_t *len);
void runes_term_destroy(RunesTerm *t);

#endif
