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

    char *font_name;
    double font_size;
    char font_italic;
    char font_bold;
    char font_underline;

    char show_cursor;

    cairo_pattern_t *colors[8];

    int row;
    int col;
};

void runes_term_init(RunesTerm *t, int argc, char *argv[]);
void runes_term_cleanup(RunesTerm *t);

#endif
