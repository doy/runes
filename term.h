#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

struct runes_term {
    RunesWindow *w;
    /* RunesBuffer *buf; */

    cairo_surface_t *surface;
    cairo_t *cr;

    uv_loop_t *loop;
};

RunesTerm *runes_term_create();
void runes_term_destroy(RunesTerm *t);

#endif
