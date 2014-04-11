#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

struct runes_term {
    RunesWindowBackend w;
    RunesPtyBackend pty;

    cairo_t *cr;
    cairo_t *backend_cr;
    uv_loop_t *loop;

    cairo_pattern_t *bgcolor;
    cairo_pattern_t *fgcolor;
    cairo_pattern_t *cursorcolor;
    cairo_scaled_font_t *font;

    int row;
    int col;
};

void runes_term_init(RunesTerm *t, int argc, char *argv[]);
void runes_term_cleanup(RunesTerm *t);

#endif
